#include "../src/vulkan_layer.cpp"
static void expect(bool value,const char* message) { if(!value) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
static std::shared_ptr<Geometry> cube() {
    auto g=std::make_shared<Geometry>(); g->kind=1; g->convexSolid=true;
    Vec3 v[8]; for(unsigned i=0;i<8;++i) v[i]={i&1?1.f:-1.f,i&2?1.f:-1.f,i&4?1.f:-1.f};
    const unsigned faces[][4]{{0,2,3,1},{4,5,7,6},{0,1,5,4},{2,6,7,3},{0,4,6,2},{1,3,7,5}};
    for(const auto& f:faces) for(unsigned i=1;i<3;++i) for(unsigned k:{f[0],f[i],f[i+1]}) g->triangles.push_back(v[k]);
    return g;
}
static Mesh instance(std::shared_ptr<const Geometry> geometry,unsigned kind) {
    Mesh m{}; m.geometry=std::move(geometry); m.kind=kind;
    for(int k=0;k<3;++k) { m.bounds[k]=FLT_MAX; m.bounds[k+3]=-FLT_MAX; }
    auto include=[&](Vec3 p) { p=m.world(p); float xyz[]{p.x,p.y,p.z}; for(int k=0;k<3;++k) { m.bounds[k]=std::min(m.bounds[k],xyz[k]); m.bounds[k+3]=std::max(m.bounds[k+3],xyz[k]); } };
    for(Vec3 p:m.geometry->triangles) include(p); for(Vec3 p:m.geometry->lines) include(p);
    return m;
}
static void index(RuntimeScene& scene) {
    scene.nodes.clear(); scene.order.resize(scene.meshes.size()); std::iota(scene.order.begin(),scene.order.end(),0u);
    buildSpatialNodeFor(scene.meshes,scene.nodes,scene.order,0,uint32_t(scene.meshes.size()));
}
static double area(const Geometry& g) {
    using namespace projectile_trim; double total=0;
    for(size_t i=0;i<g.triangles.size();i+=3) { auto n=cross(point(g.triangles[i+1])-point(g.triangles[i]),point(g.triangles[i+2])-point(g.triangles[i])); total+=sqrt(dot(n,n))*.5; }
    return total;
}
int main() {
    SetEnvironmentVariableW(L"LABPLUS_SCENE_TEST",L"1");
    SetEnvironmentVariableW(L"LABPLUS_TEST_COLLISION_MODE",L"1");
    using namespace projectile_trim;
    {
        std::vector<unsigned char> data(0x200);
        auto put=[&](size_t at,const auto& value) { memcpy(data.data()+at,&value,sizeof(value)); };
        put(0x42,uint16_t(8)); put(0x44,uint16_t(0x70-0x42)); put(0x50,uint16_t(6)); put(0x52,uint16_t(0xf0-0x50));
        put(0x54,uint16_t(6)); put(0x56,uint16_t(0x150-0x54)); put(0x58,uint16_t(24)); put(0x5a,uint16_t(0x168-0x58));
        for(unsigned i=0;i<8;++i) { std::array<float,4> p{i&1?1.f:-1.f,i&2?1.f:-1.f,i&4?1.f:-1.f,0}; put(0x70+i*16,p); }
        const std::array<float,4> planes[]{{0,0,-1,-1},{0,0,1,-1},{0,-1,0,-1},{0,1,0,-1},{-1,0,0,-1},{1,0,0,-1}};
        memcpy(data.data()+0xf0,planes,sizeof(planes));
        const unsigned char corners[]{0,2,3,1,4,5,7,6,0,1,5,4,2,6,7,3,0,4,6,2,1,3,7,5}; memcpy(data.data()+0x168,corners,sizeof(corners));
        for(unsigned i=0;i<6;++i) { put(0x150+i*4,uint16_t(i*4)); data[0x150+i*4+2]=4; }
        RuntimeSceneBuilder builder; auto address=reinterpret_cast<uintptr_t>(data.data());
        expect(builder.nativePlanes(address,data.data()).size()==6,"native relative plane arrays agree with face corners");
        put(0x50,uint16_t(5)); expect(builder.nativePlanes(address,data.data()).empty(),"mismatched plane/face counts keep the mesh fallback"); put(0x50,uint16_t(6));
        put(0x52,uint16_t(8)); expect(builder.nativePlanes(address,data.data()).empty(),"overlapping native arrays are rejected"); put(0x52,uint16_t(0xf0-0x50));
        put(0xf0+12,1.f); expect(builder.nativePlanes(address,data.data()).empty(),"unrelated plane equations cannot pass the native face certificate");
    }
    Budget budget; budget.cancelled=[] { return false; };
    auto solid=cube(); expect(closedConvex(*solid,budget).faces.size()==6,"closed native convex box has six planes");
    auto nativeCube=std::make_shared<Geometry>(*solid);
    nativeCube->solidPlanes={{0,0,-1,-1},{0,0,1,-1},{0,-1,0,-1},{0,1,0,-1},{-1,0,0,-1},{1,0,0,-1}};
    for(auto& p:nativeCube->triangles) if(p.x==1&&p.y==1&&p.z==1) p.x-=5e-7f;
    auto withoutPlanes=*nativeCube; withoutPlanes.solidPlanes.clear();
    expect(closedConvex(withoutPlanes,budget).faces.empty(),"a tiny warped fan can fail inferred support planes");
    auto nativeHull=closedConvex(*nativeCube,budget);
    expect(nativeHull.native.size()==6,"native planes retain a validated convex solid despite triangulation noise");
    Mesh nativeInstance=instance(nativeCube,1); nativeInstance.matrix[0]=-2; nativeInstance.matrix[5]=.5f; nativeInstance.matrix[3]=10;
    auto transformed=worldPlanes(nativeHull,nativeInstance);
    expect(transformed.size()==6,"native planes support reflected nonuniform transforms");
    for(const auto& p:transformed) expect(p.distance({10,0,0})<0,"inverse-transpose planes preserve the transformed interior");
    expect(std::any_of(transformed.begin(),transformed.end(),[](const Plane& p) { return p.distance({12.1,0,0})>0; }),"transformed native volume never expands past its exterior");
    auto invalidPlanes=*nativeCube; invalidPlanes.solidPlanes[0][0]=100;
    expect(closedConvex(invalidPlanes,budget).native.empty(),"invalid native plane normals never replace mesh fallback");
    invalidPlanes=*nativeCube; invalidPlanes.convexSolid=false;
    expect(closedConvex(invalidPlanes,budget).native.empty(),"native-looking plane data without verified player-blocking classification is rejected");
    auto solidInstance=instance(solid,1);
    auto planes=worldPlanes(closedConvex(*solid,budget),solidInstance);
    projectile_trim::Polygon disjointTriangle{{2,0,0},{0,2,0},{2,2,0}};
    std::vector<projectile_trim::Polygon> disjoint{disjointTriangle};
    expect(subtract(disjoint,planes,budget)&&disjoint.size()==1&&disjoint[0].size()==3&&memcmp(disjoint[0].data(),disjointTriangle.data(),3*sizeof(Point))==0,"a disjoint triangle crossing individual face planes retains its exact original triangulation");
    auto open=*solid; open.triangles.resize(open.triangles.size()-3);
    expect(closedConvex(open,budget).faces.empty(),"open meshes cannot hide projectile collision");
    auto compressed=*solid; compressed.convexSolid=false;
    expect(closedConvex(compressed,budget).faces.empty(),"compressed triangle surfaces are not assumed solid even if they resemble a box");
    auto concave=*solid; for(auto& p:concave.triangles) if(p.x==1&&p.y==1&&p.z==1) p={0,0,0};
    expect(closedConvex(concave,budget).faces.empty(),"non-convex or degenerate solid fails validation");
    auto shots=std::make_shared<Geometry>(); shots->kind=3;
    for(unsigned i=0;i<20;++i) {
        shots->triangles.insert(shots->triangles.end(),{{-.1f,0,0},{.1f,0,0},{0,.1f,0}});
        shots->lines.insert(shots->lines.end(),{{-.1f,0,0},{.1f,0,0}});
    }
    shots->triangles.insert(shots->triangles.end(),{{-2,0,0},{2,0,0},{0,2,0}});
    shots->lines.insert(shots->lines.end(),{{-2,0,0},{2,0,0}});
    auto scene=std::make_shared<RuntimeScene>(); scene->meshes={instance(solid,1),instance(shots,3)}; index(*scene);
    auto result=buildProjectileTrim(*scene);
    expect(result&&result->changed==1&&result->replacement[1]>=0,"partial overlap produces a cached replacement");
    auto reduced=*result->meshes[size_t(result->replacement[1])].geometry;
    expect(area(reduced)>2&&area(reduced)<2.005,"clipping preserves the analytic exterior area of a triangle crossing a box, with conservative inset");
    double length=0;
    for(size_t i=0;i<reduced.lines.size();i+=2) { auto delta=point(reduced.lines[i+1])-point(reduced.lines[i]); length+=sqrt(dot(delta,delta)); }
    expect(length>2&&length<2.005,"line segments crossing a volume retain both exterior parts");
    expect(scene->meshes[1].geometry==shots&&shots->triangles.size()==63,"raw scene geometry remains untouched");
    auto wireOnly=std::make_shared<Geometry>(*shots);
    wireOnly->triangles.assign(shots->triangles.end()-3,shots->triangles.end());
    scene->meshes[1]=instance(wireOnly,3); index(*scene); auto wires=buildProjectileTrim(*scene);
    expect(wires->replacement[1]>=0,"useful wire clipping survives an expensive fill triangulation");
    const auto& wireGeometry=*wires->meshes[size_t(wires->replacement[1])].geometry;
    expect(wireGeometry.triangles.size()==wireOnly->triangles.size()&&wireGeometry.lines.size()<wireOnly->lines.size()&&area(wireGeometry)==area(*wireOnly),"fill fallback preserves the original while wire geometry improves without growth");
    auto shortened=std::make_shared<Geometry>(); shortened->kind=3;
    shortened->triangles={{-3,-3,0},{-2,-3,0},{-3,-2,0}}; shortened->lines={{-.5f,0,0},{2,0,0}};
    scene->meshes[1]=instance(shortened,3); index(*scene); auto sameCount=buildProjectileTrim(*scene);
    expect(sameCount->replacement[1]>=0&&sameCount->lines==1,"shortening a segment is retained even when primitive count is unchanged");
    const auto& shortenedLine=sameCount->meshes[size_t(sameCount->replacement[1])].geometry->lines;
    expect(shortenedLine.size()==2&&shortenedLine[0].x>.99f&&shortenedLine[1].x==2,"equal-count clipping preserves the exterior endpoint and removes the interior portion");
    auto inside=std::make_shared<Geometry>(); inside->kind=3; inside->triangles.assign(shots->triangles.begin(),shots->triangles.begin()+3);
    scene->meshes[1]=instance(inside,3); index(*scene); result=buildProjectileTrim(*scene);
    expect(result->replacement[1]==-2&&result->removed==1,"fully covered mesh is removed");
    auto boundary=std::make_shared<Geometry>(*inside); for(auto& p:boundary->triangles) p.z=1;
    scene->meshes[1]=instance(boundary,3); index(*scene); result=buildProjectileTrim(*scene);
    expect(result->replacement[1]==-1,"coincident boundaries are preserved conservatively");
    auto outside=std::make_shared<Geometry>(*inside); for(auto& p:outside->triangles) { p.x+=1.2f; p.y+=1.2f; }
    scene->meshes[0].matrix[0]=scene->meshes[0].matrix[5]=.70710678f; scene->meshes[0].matrix[1]=-.70710678f; scene->meshes[0].matrix[4]=.70710678f;
    // Expanded bounds deliberately admit the test triangle to broad-phase.
    for(int k=0;k<3;++k) { scene->meshes[0].bounds[k]=-2; scene->meshes[0].bounds[k+3]=2; }
    scene->meshes[1]=instance(outside,3); index(*scene); result=buildProjectileTrim(*scene);
    expect(result->replacement[1]==-1,"overlap with a rotated solid's AABB cannot erase exterior geometry");
    scene->meshes[0].matrix[0]=-2; scene->meshes[0].matrix[1]=0; scene->meshes[0].matrix[4]=0; scene->meshes[0].matrix[5]=.5f;
    scene->meshes[1]=instance(inside,3); index(*scene); result=buildProjectileTrim(*scene);
    expect(result->replacement[1]==-2,"reflected non-uniform transforms preserve convex containment");
    expect(!buildProjectileTrim(*scene,[] { return true; }),"cancelled level work cannot publish a result");
    scene->meshes={instance(solid,1),instance(shots,3)}; index(*scene); runtimeWorld=scene; runtimeTrim=buildProjectileTrim(*scene);
    runtimeRendering=true; enabled=true; showShotCollision=true; showCollision=showTriggers=showWalls=inspectVolumes=false; radius=40;
    GameCamera camera{}; camera.matrix[0]=camera.matrix[5]=camera.matrix[10]=camera.matrix[15]=.2f;
    std::vector<Candidate> visible; trimShotCollision=false; gatherVisible(camera,visible);
    expect(visible.size()==1&&visible[0].mesh==&scene->meshes[1],"disabled experiment selects exact original projectile mesh");
    trimShotCollision=true; gatherVisible(camera,visible);
    expect(visible.size()==1&&visible[0].mesh==&runtimeTrim->meshes[0],"enabled experiment selects the reduced immutable mesh");
    WorldVertexCache cache; cache.begin(scene,visible,runtimeTrim); auto positions=cache.positions(*visible[0].mesh);
    expect(positions&&positions->size()==reduced.triangles.size()+reduced.lines.size(),"replacement meshes participate in the bounded world vertex cache");
    auto entities=std::make_shared<RuntimeEntities>(); entities->meshes.push_back(instance(inside,3)); runtimeEntities=entities; gatherVisible(camera,visible);
    expect(visible.size()==2&&visible[1].mesh==&entities->meshes[0],"moving and live entity geometry is never subtracted against a stale world");
    runtimeEntities.reset();
    auto benchmark=std::make_shared<RuntimeScene>();
    for(unsigned i=0;i<512;++i) for(unsigned kind:{1u,3u}) {
        auto mesh=instance(kind==1?solid:shots,kind);
        const float x=float(i%32)*4-64,y=float(i/32)*4-32;
        mesh.matrix[3]=x; mesh.matrix[7]=y;
        mesh.bounds[0]+=x; mesh.bounds[3]+=x; mesh.bounds[1]+=y; mesh.bounds[4]+=y;
        benchmark->meshes.push_back(std::move(mesh));
    }
    index(*benchmark); runtimeWorld=benchmark; runtimeTrim=buildProjectileTrim(*benchmark); radius=200;
    camera.matrix[0]=camera.matrix[5]=camera.matrix[10]=.01f; camera.matrix[15]=1;
    Limits smallBudget; smallBudget.work=4096;
    auto prioritized=buildProjectileTrim(*benchmark,[]{return false;},smallBudget,&camera,5);
    expect(prioritized->limited&&prioritized->replacement[545]!=-1&&prioritized->replacement[1]==-1,"bounded processing starts with visible nearby geometry and preserves distant originals when its budget expires");
    GameCamera moved=camera; moved.position[0]=-64; moved.position[1]=-32;
    expect(pendingProjectileTrimInView(*benchmark,*prioritized,moved,5),"a newly visited region discovers unfinished projectile meshes");
    auto continued=buildProjectileTrim(*benchmark,[]{return false;},{},&moved,5,prioritized.get());
    expect(continued->replacement[1]!=-1&&continued->reasons[1]!=ProjectileTrim::Pending&&prioritized->replacement[1]==-1,"background continuation processes the new region without mutating the published snapshot");
    expect(continued->replacement[545]==prioritized->replacement[545]&&continued->originalTriangles==prioritized->originalTriangles&&continued->triangles<=prioritized->triangles&&continued->lines<=prioritized->lines,"continuation retains previous reductions and does not double-count originals");
    RuntimeScene other=*benchmark;
    Limits noTime; noTime.time=std::chrono::milliseconds(0);
    auto foreign=buildProjectileTrim(other,[]{return false;},noTime,&camera,5,continued.get());
    expect(foreign->replacement[545]==-1,"a same-sized different world cannot reuse earlier clipping");
    std::vector<Vertex> output; double elapsed[2]{}; size_t vertices[2]{};
    for(unsigned mode=0;mode<2;++mode) {
        trimShotCollision=mode!=0;
        for(unsigned i=0;i<5;++i) collectVertices(camera,output);
        auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<80;++i) collectVertices(camera,output);
        elapsed[mode]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/80;
        vertices[mode]=output.size();
    }
    expect(vertices[1]<vertices[0]&&!runtimeTrim->limited,"overlap-heavy benchmark reduces work without hitting the build budget");
    printf("Synthetic overlap fixture (not game FPS): raw=%zu vertices %.3fms/frame; trimmed=%zu vertices %.3fms/frame; background build %.2fms\n",vertices[0],elapsed[0],vertices[1],elapsed[1],runtimeTrim->milliseconds);
    trimShotCollision=false; collectVertices(camera,output);
    expect(output.size()==vertices[0],"switching back restores original vertex count after a warm reduced cache");
    runtimeTrim.reset(); runtimeWorld.reset(); runtimeRendering=false;
    puts("PASS: conservative partial triangle/line subtraction, closed/convex validation, transformed solids, raw fallback, cancellation, A/B selection, cache and live-entity isolation.");
}
