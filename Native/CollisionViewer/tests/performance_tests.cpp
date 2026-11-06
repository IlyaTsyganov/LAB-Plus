#include "../src/vulkan_layer.cpp"
#include <future>

static void expect(bool value,const char* message) {
    if(!value) { fprintf(stderr,"FAIL: %s\n",message); exit(1); }
}

static std::promise<void>* presentEntered;
static std::shared_future<void> releasePresent;
static std::atomic<unsigned> recordedViewports{0};
static VKAPI_ATTR VkResult VKAPI_CALL stalledPresent(VkQueue,const VkPresentInfoKHR*) {
    presentEntered->set_value();
    releasePresent.wait();
    return VK_SUBOPTIMAL_KHR;
}
static VKAPI_ATTR void VKAPI_CALL recordViewport(VkCommandBuffer,uint32_t,uint32_t,const VkViewport*) {
    ++recordedViewports;
}
static VKAPI_ATTR VkResult VKAPI_CALL submitNextFrame(VkQueue,uint32_t,const VkSubmitInfo*,VkFence) { return VK_SUCCESS; }

static void cacheBoundaries() {
    RuntimeScene reusedScene;
    auto geometry=std::make_shared<Geometry>(); geometry->kind=1;
    geometry->triangles={{1,2,3},{4,5,6},{7,8,9}}; geometry->lines={{1,2,3},{4,5,6}};
    reusedScene.meshes.resize(1); auto& mesh=reusedScene.meshes[0]; mesh.kind=1; mesh.geometry=geometry;
    std::vector<Candidate> visible{{&mesh,0,0}};
    WorldVertexCache cache;
    auto firstOwner=std::shared_ptr<const RuntimeScene>(&reusedScene,[](const RuntimeScene*) {});
    cache.begin(firstOwner,visible);
    auto positions=cache.positions(mesh);
    expect(positions&&positions->size()==5&&(*positions)[0].x==1,"immutable world positions are cached without color/style state");
    const auto* originalPositions=positions;
    auto trim=std::make_shared<ProjectileTrim>(); trim->meshes.push_back(mesh); trim->meshes.back().kind=3;
    visible.push_back({&trim->meshes[0],0,0}); cache.begin(firstOwner,visible,trim);
    expect(cache.positions(mesh)==originalPositions&&cache.positions(trim->meshes[0]),"publishing a trim batch retains the original world's warm positions");
    auto nextTrim=std::make_shared<ProjectileTrim>(*trim); nextTrim->meshes[0].matrix[3]=20;
    visible[1].mesh=&nextTrim->meshes[0]; cache.begin(firstOwner,visible,nextTrim);
    expect(cache.positions(mesh)==originalPositions&&cache.positions(nextTrim->meshes[0])->at(0).x==21,"a subsequent batch refreshes only replacement positions");
    visible.resize(1);
    firstOwner.reset();
    mesh.matrix[3]=10;
    // Deliberately reuse exactly the same RuntimeScene and Mesh addresses with a
    // distinct lifetime, as a heap allocator can after a map change.
    auto secondOwner=std::shared_ptr<const RuntimeScene>(&reusedScene,[](const RuntimeScene*) {});
    cache.begin(secondOwner,visible); positions=cache.positions(mesh);
    expect(positions&&(*positions)[0].x==11,"new ownership invalidates a reused scene and mesh address");
    Mesh dynamic=mesh; dynamic.kind=0;
    expect(!cache.positions(dynamic),"live entities outside the immutable world vector never use cached positions");
    auto huge=std::make_shared<Geometry>(); huge->kind=1; huge->triangles.resize(WorldVertexCache::limit/sizeof(Vec3)+1);
    mesh.geometry=huge;
    auto thirdOwner=std::shared_ptr<const RuntimeScene>(&reusedScene,[](const RuntimeScene*) {});
    cache.begin(thirdOwner,visible);
    expect(!cache.positions(mesh),"an oversized mesh uses direct transforms without a vertex budget or oversized cache allocation");
    huge->triangles.resize(WorldVertexCache::limit/(2*sizeof(Vec3))+1);
    reusedScene.meshes.resize(2); reusedScene.meshes[0].geometry=huge; reusedScene.meshes[1].geometry=huge;
    visible={{&reusedScene.meshes[0],0,0},{&reusedScene.meshes[1],0,0}};
    auto fourthOwner=std::shared_ptr<const RuntimeScene>(&reusedScene,[](const RuntimeScene*) {});
    cache.begin(fourthOwner,visible);
    expect(!cache.positions(reusedScene.meshes[0])&&!cache.positions(reusedScene.meshes[1]),"combined oversized visible set bypasses the cache instead of thrashing");
    cache.begin({},visible);
    expect(!cache.positions(reusedScene.meshes[0]),"loading or absent world disables and clears the cache");
}

static void geometryBenchmark() {
    constexpr unsigned meshCount=1137;
    auto world=std::make_shared<RuntimeScene>();
    auto geometry=std::make_shared<Geometry>(); geometry->kind=1;
    // Similar vertex load to the recorded 5.1 Fortress view (1,137 world
    // instances, roughly 66k triangles and 92k wire segments), not a game FPS test.
    for(unsigned i=0;i<58;++i) {
        float offset=float(i)/1000.f;
        geometry->triangles.push_back({-.2f+offset,-.2f,.2f});
        geometry->triangles.push_back({.2f+offset,-.2f,.2f});
        geometry->triangles.push_back({offset,.2f,.2f});
    }
    for(unsigned i=0;i<80;++i) {
        float offset=float(i)/1000.f;
        geometry->lines.push_back({-.2f+offset,-.2f,.2f}); geometry->lines.push_back({.2f+offset,-.2f,.2f});
    }
    world->meshes.resize(meshCount); world->order.resize(meshCount); std::iota(world->order.begin(),world->order.end(),0u);
    for(unsigned i=0;i<meshCount;++i) {
        auto& mesh=world->meshes[i]; mesh.kind=1; mesh.geometry=geometry;
        const float bounds[]{-.5f,-.5f,0,.5f,.5f,.8f}; memcpy(mesh.bounds,bounds,sizeof(bounds));
        mesh.matrix[0]=.83f; mesh.matrix[1]=-.23f; mesh.matrix[4]=.23f; mesh.matrix[5]=.83f;
        mesh.matrix[3]=float(i%17)/1000.f; mesh.matrix[7]=float(i%19)/1000.f;
    }
    buildSpatialNodeFor(world->meshes,world->nodes,world->order,0,meshCount);
    runtimeRendering=true; runtimeWorld=world; runtimeEntities.reset();
    showCollision=true; showTriggers=showWalls=inspectVolumes=showResearchPath=showVolumeHistory=false; collisionMode=1; radius=40;
    volumeAppearances[0].opacity=.5f;
    GameCamera camera{}; camera.matrix[0]=camera.matrix[5]=camera.matrix[10]=camera.matrix[15]=1;
    std::vector<Vertex> output;
    const auto fill=collectVertices(camera,output);
    expect(fill==meshCount*58*3&&output.size()==meshCount*(58*3+80*2),"all Fortress-size fixture geometry remains present");
    std::vector<Vertex> reference;
    for(const auto& mesh:world->meshes) {
        const auto style=volumeStyle(mesh,false,GetTickCount64());
        for(Vec3 position:geometry->triangles) { Vertex vertex{}; vertex.position=mesh.world(position); memcpy(vertex.color,style.fill.data(),16); reference.push_back(vertex); }
    }
    for(const auto& mesh:world->meshes) {
        const auto style=volumeStyle(mesh,false,GetTickCount64());
        for(Vec3 position:geometry->lines) { Vertex vertex{}; vertex.position=mesh.world(position); memcpy(vertex.color,style.line.data(),16); reference.push_back(vertex); }
    }
    expect(output.size()==reference.size()&&memcmp(output.data(),reference.data(),output.size()*sizeof(Vertex))==0,"prepared geometry matches direct transforms and colors byte for byte");
    for(unsigned warmup=0;warmup<3;++warmup) collectVertices(camera,output);
    auto start=std::chrono::steady_clock::now(); constexpr unsigned iterations=100;
    for(unsigned i=0;i<iterations;++i) collectVertices(camera,output);
    double milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/iterations;
    printf("Fortress-size CPU preparation: %u world meshes, %zu vertices, %.3fms/frame (%u iterations); direct-transform output verified\n",meshCount,output.size(),milliseconds,iterations);
    expect(memcmp(output.data(),reference.data(),output.size()*sizeof(Vertex))==0,"warm-cache output remains byte identical to direct transforms");
    volumeAppearances[0].color={.1f,.7f,.3f}; volumeAppearances[0].opacity=.25f;
    collectVertices(camera,output);
    expect(output[0].color[0]==.1f&&output[0].color[1]==.7f&&output[0].color[2]==.3f&&output[0].color[3]==.25f&&
           output[fill].color[0]==.1f&&output[fill].color[1]==.7f&&output[fill].color[3]==.65f,"warm cached positions retain live fill/outline colors and opacity");
    auto entities=std::make_shared<RuntimeEntities>(); Mesh trigger=world->meshes[0]; trigger.kind=0; entities->meshes.push_back(trigger);
    runtimeEntities=entities; showTriggers=true;
    collectVertices(camera,output);
    const float initial=output[fill].position.x;
    entities->meshes[0].matrix[3]+=.1f;
    collectVertices(camera,output);
    expect(fabsf(output[fill].position.x-initial-.1f)<.00001f,"live trigger transforms stay fresh beside a warm world cache");
    runtimeEntities.reset();
    runtimeWorld.reset(); runtimeRendering=false;
}

int main(int argc,char** argv) {
    const bool baseline=argc==2&&strcmp(argv[1],"--baseline")==0;
    SetEnvironmentVariableW(L"LABPLUS_SMOKE_TEST",L"1");
    SetEnvironmentVariableW(L"LABPLUS_SCENE_TEST",L"1");
    SetEnvironmentVariableW(L"LABPLUS_TEST_COLLISION_MODE",L"1");
    SetEnvironmentVariableW(L"LABPLUS_DATA_DIRECTORY",L"C:\\nonexistent-labplus-performance-fixture");
    void* dispatch=reinterpret_cast<void*>(uintptr_t(12345));
    VkQueue queue=reinterpret_cast<VkQueue>(&dispatch);
    VkCommandBuffer command=reinterpret_cast<VkCommandBuffer>(&dispatch);
    auto device=std::make_unique<Device>();
    auto* data=device.get(); data->QueuePresentKHR=stalledPresent; data->CmdSetViewport=recordViewport; data->QueueSubmit=submitNextFrame;
    data->families.resize(1); data->families[0].queueFlags=VK_QUEUE_COMPUTE_BIT;
    devices[dispatch]=std::move(device);
    VkSwapchainKHR chain=reinterpret_cast<VkSwapchainKHR>(uintptr_t(1)); uint32_t image=0;
    VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR}; info.swapchainCount=1; info.pSwapchains=&chain; info.pImageIndices=&image;
    auto swap=std::make_unique<Swapchain>(); swap->device=data; swaps[chain]=std::move(swap);
    for(unsigned test=0;test<3;++test) {
        data->depth.submitted=42;
        enabled=test!=0;
        queues.clear(); if(test==2) queues[queue]={data,0};
        std::promise<void> entered,released;
        presentEntered=&entered; releasePresent=released.get_future().share();
        auto present=std::async(std::launch::async,[&] { return vkQueuePresentKHR(queue,&info); });
        expect(entered.get_future().wait_for(std::chrono::seconds(2))==std::future_status::ready,"driver presentation was entered");
        auto started=std::chrono::steady_clock::now();
        auto record=std::async(std::launch::async,[&] { VkViewport viewport{0,0,1920,1080,0,1}; vkCmdSetViewport(command,0,1,&viewport); expect(vkQueueSubmit(queue,0,nullptr,VK_NULL_HANDLE)==VK_SUCCESS,"next-frame submission reaches the driver"); });
        // The fake driver waits on a gate, so the result does not depend on GPU,
        // VSync mode or scheduling a short sleep precisely. Recording must finish
        // before presentation is released, rather than simply look fast afterward.
        const bool completed=record.wait_for(std::chrono::milliseconds(100))==std::future_status::ready;
        double blocked=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        released.set_value(); record.get();
        expect(present.get()==VK_SUBOPTIMAL_KHR,"driver presentation status is preserved");
        expect(data->depth.previousPresent==42&&data->depth.submitted==43,"presentation depth boundary excludes concurrently submitted next-frame work");
        expect(completed||baseline,"command recording cannot wait behind a blocked driver presentation");
        printf("Present path %u: recording completed while driver blocked=%s; recording wait=%.3fms\n",test,completed?"yes":"no",blocked);
    }
    expect(recordedViewports==3,"all recording commands reached the driver");
    queues.clear(); swaps.clear(); devices.clear();
    geometryBenchmark();
    cacheBoundaries();
    puts(baseline?"BASELINE: presentation-lock measurement completed.":"PASS: concurrent recording/submission during driver presentation; immutable world-cache byte-equivalence, live colors/entities, reused owners and oversized-view fallback.");
}
