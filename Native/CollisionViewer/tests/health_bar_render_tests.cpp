#include "../src/vulkan_layer.cpp"
#include <limits>
static void requireHealthBar(bool condition,const char* text) {
    if(!condition) { fprintf(stderr,"FAIL: %s\n",text); exit(1); }
}
static bool healthBarNear(float left,float right,float tolerance=.0001f) { return std::abs(left-right)<tolerance; }
static size_t healthBarColorVertices(const EnemyHealthBarGeometry& geometry,float red,float green,float blue) {
    return static_cast<size_t>(std::count_if(geometry.triangles.begin(),geometry.triangles.end(),[&](const Vertex& vertex) {
        return healthBarNear(vertex.color[0],red)&&healthBarNear(vertex.color[1],green)&&healthBarNear(vertex.color[2],blue);
    }));
}
static void saveHealthBarPreview(const EnemyHealthBarGeometry& geometry,const GameCamera& camera,const std::filesystem::path& path) {
    constexpr int width=480,height=240;
    std::vector<unsigned char> pixels(width*height*3,37);
    const auto& atlas=eternalUi2AtlasPixels();
    auto projected=[&](Vec3 point) {
        float w=camera.matrix[12]*point.x+camera.matrix[13]*point.y+camera.matrix[14]*point.z+camera.matrix[15];
        float x=(camera.matrix[0]*point.x+camera.matrix[1]*point.y+camera.matrix[2]*point.z+camera.matrix[3])/w;
        float y=(camera.matrix[4]*point.x+camera.matrix[5]*point.y+camera.matrix[6]*point.z+camera.matrix[7])/w;
        return std::array<float,2>{(x+1)*width*.5f,(1-y)*height*.5f};
    };
    auto coverage=[&](float u,float v) {
        const float x=u*kEternalUi2AtlasWidth-.5f,y=v*kEternalUi2AtlasHeight-.5f;
        const int left=static_cast<int>(std::floor(x)),top=static_cast<int>(std::floor(y));
        const float fx=x-left,fy=y-top;
        auto read=[&](int column,int row) { return atlas[std::clamp(row,0,int(kEternalUi2AtlasHeight)-1)*kEternalUi2AtlasWidth+std::clamp(column,0,int(kEternalUi2AtlasWidth)-1)]/255.f; };
        return (read(left,top)*(1-fx)+read(left+1,top)*fx)*(1-fy)+(read(left,top+1)*(1-fx)+read(left+1,top+1)*fx)*fy;
    };
    // These billboards project to axis-aligned rectangles. Rasterize each
    // logical quad once, avoiding shared-edge double alpha on its diagonal.
    for(size_t index=0;index+5<geometry.triangles.size();index+=6) {
        const auto lower=projected(geometry.triangles[index].position),upper=projected(geometry.triangles[index+2].position);
        const float left=lower[0],right=upper[0],top=upper[1],bottom=lower[1];
        for(int y=std::max(0,int(std::floor(top)));y<=std::min(height-1,int(std::ceil(bottom)));++y)
            for(int x=std::max(0,int(std::floor(left)));x<=std::min(width-1,int(std::ceil(right)));++x) {
                const float px=static_cast<float>(x)+.5f,py=static_cast<float>(y)+.5f;
                if(px<left||px>=right||py<top||py>=bottom)continue;
                const auto& vertex=geometry.triangles[index];float alpha=vertex.color[3];
                if(vertex.texcoord[0]>=0) {
                    float u=vertex.texcoord[0]+(px-left)/(right-left)*(geometry.triangles[index+2].texcoord[0]-vertex.texcoord[0]);
                    float v=geometry.triangles[index+2].texcoord[1]+(py-top)/(bottom-top)*(vertex.texcoord[1]-geometry.triangles[index+2].texcoord[1]);
                    alpha*=coverage(u,v);
                }
                for(size_t channel=0;channel<3;++channel) {
                    auto& destination=pixels[(y*width+x)*3+channel];
                    destination=static_cast<unsigned char>(std::lround(std::clamp(vertex.color[channel]*255*alpha+destination*(1-alpha),0.f,255.f)));
                }
            }
    }
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path,std::ios::binary); output<<"P6\n"<<width<<' '<<height<<"\n255\n"; output.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());
    requireHealthBar(bool(output),"projected renderer preview saved");
}
static int benchmarkHealthBarFont(bool maximum=false) {
    GameCamera camera{}; camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    LevelIdentity level;level.map=1;level.world=2;level.name="benchmark";
    for(unsigned count:{3u,50u,200u}) {
        InvisibleNumbersSnapshot snapshot;snapshot.phase="live";snapshot.level=level;snapshot.captured=1000;
        for(unsigned index=0;index<count;++index) {
            InvisibleEnemyRow row;row.entity={0x10000+index*32,1,0x20000+index*32,index,1,"idAI2"};
            row.positionKnown=row.maximumKnown=true;row.health=index%3==0?200:index%3==1?100:25;row.maximum=200;
            row.top={static_cast<float>(int(index%11)-5)*.5f,12,0};
            row.staggerKnown=row.staggerActive=index%3==2;row.staggerSeconds=2.5;row.staggerDuration=maximum?999960LL*5:0;snapshot.barEnemies.push_back(row);
        }
        EnemyHealthBarGeometry geometry;
        auto pose=[](auto& row,Vec3& top) {top=row.top;return true;};
        for(unsigned frame=0;frame<10;++frame)populateEnemyHealthBars(geometry,camera,snapshot,level,1000,40,1080,pose);
        std::vector<double> timings;
        for(unsigned frame=0;frame<100;++frame) {
            auto begin=std::chrono::steady_clock::now();
            populateEnemyHealthBars(geometry,camera,snapshot,level,1000,40,1080,pose);
            timings.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count());
        }
        std::sort(timings.begin(),timings.end());
        printf("bars=%u vertices=%zu median_us=%.3f p95_us=%.3f\n",count,geometry.triangles.size(),timings[50],timings[95]);
    }
    return 0;
}
int main(int argc,char** argv) {
    if(argc==2&&std::string(argv[1])=="--benchmark")return benchmarkHealthBarFont();
    if(argc==2&&std::string(argv[1])=="--benchmark-max")return benchmarkHealthBarFont(true);
    requireHealthBar(std::string(kEternalUi2SourceSha256)=="da163693bafcbabbb720b2c28d9f08a655f736a9fb5a17b238f3d9b798583838","font atlas identifies the exact bundled EternalUi2Regular.ttf");
    requireHealthBar(kEternalUi2FontSize==18&&eternalUi2TextWidth("200/200")==66,"font layout uses the real Eternal UI 2 metrics");
    requireHealthBar(eternalUi2TextWidth("A\n200/200")==66&&eternalUi2TextWidth("\xc2\xb0")==eternalUi2TextWidth("?"),"newlines and unsupported UTF-8 keep measurement consistent");
    size_t coverageCount=0;for(const auto& run:kEternalUi2CoverageRuns)coverageCount+=run.count;
    requireHealthBar(coverageCount==kEternalUi2AtlasWidth*kEternalUi2AtlasHeight,"font coverage stream is exactly bounded");
    for(uint32_t code=33;code<=126;++code) {
        const auto& glyph=eternalUi2Glyph(code);
        requireHealthBar(glyph.width&&glyph.height&&glyph.x+glyph.width<=kEternalUi2AtlasWidth&&glyph.y+glyph.height<=kEternalUi2AtlasHeight,"all printable ASCII glyph UVs stay inside the real font atlas");
    }
    unsigned glyphs=0;
    appendEternalUi2GlyphQuads("200/200",0,18,1,{1,1,1,.5f},[&](float left,float top,float right,float bottom,float u0,float v0,float u1,float v1,const auto& color) {
        requireHealthBar(right>left&&bottom>top&&u0>=0&&v0>=0&&u1<=1&&v1<=1&&color[3]==.5f,"glyph quads preserve straight-alpha color and valid UVs");++glyphs;
    });
    requireHealthBar(glyphs==7,"font emits one textured quad per glyph");
    requireHealthBar(enemyStaggerBarText(2.5,5)=="GK 2.50 / 5.00 s"&&enemyStaggerBarText(2.5,0)=="GK 2.50s","GK maximum duration is shown only when validated");
    requireHealthBar(!overlayDepthTestEnabled(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,true)&&!overlayDepthWriteEnabled(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,true),"health panels use painter order without self-depth rejection");

    LevelIdentity level; level.map=0x20000; level.world=0x30000; level.name="arena";
    InvisibleNumbersSnapshot snapshot; snapshot.level=level; snapshot.phase="live"; snapshot.captured=1000;
    InvisibleEnemyRow row; row.entity={0x40000,level.map,0x50000,1,2,"idAI2"};
    row.positionKnown=row.maximumKnown=true; row.health=100; row.maximum=200;
    row.top={100,100,100}; snapshot.barEnemies.push_back(row);
    GameCamera camera{}; camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    unsigned validations=0;
    auto pose=[&](InvisibleEnemyRow&,Vec3& top) { ++validations; top={1,4,2}; return true; };
    auto geometry=buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,pose);
    requireHealthBar(geometry.bars==1&&geometry.lines.empty()&&geometry.triangles.size()==102,"bars and real-font glyphs use exactly one quad per glyph and shadow");
    for(size_t index=0;index<geometry.triangles.size();++index) {
        const auto& vertex=geometry.triangles[index];
        requireHealthBar(index<18?(vertex.texcoord[0]==-1&&vertex.texcoord[1]==-1):
            (vertex.texcoord[0]>=0&&vertex.texcoord[0]<=1&&vertex.texcoord[1]>=0&&vertex.texcoord[1]<=1),"solid panels bypass the atlas while every font vertex has bounded UVs");
    }
    requireHealthBar(geometry.triangles.size()%6==0,"each bar and glyph run is a complete solid quad");
    requireHealthBar(healthBarNear(geometry.triangles[0].position.y,4)&&geometry.triangles[0].position.z>2.5f,"panel follows fresh pose and moves above the previous head margin");
    requireHealthBar(healthBarNear(geometry.triangles[13].position.x,1),"half health fills exactly half the background rectangle");
    requireHealthBar(healthBarColorVertices(geometry,1,1,.06f)==6,"half health is a solid bright-yellow fill");
    requireHealthBar(healthBarColorVertices(geometry,1,1,1)==42,"current and maximum HP emit readable white glyph geometry");
    requireHealthBar(enemyHealthBarText(100,200)=="100/200"&&enemyHealthBarText(12.5,200)=="12.5/200","HP label preserves actual current and maximum values");
    requireHealthBar(enemyStaggerBarText(1.25)=="GK 1.25s"&&enemyStaggerBarText(-1).empty()&&enemyStaggerBarText(std::numeric_limits<double>::quiet_NaN()).empty(),"stagger countdown is finite, explicit and never guessed");
    const auto healthy=enemyHealthBarColor(1),low=enemyHealthBarColor(.1);
    requireHealthBar(healthy[0]==0&&healthy[1]==1&&low[0]==1&&healthBarNear(low[1],.2f),"fill changes from green to red with remaining percentage");

    const size_t ordinaryVertices=geometry.triangles.size();
    snapshot.barEnemies[0].staggerKnown=snapshot.barEnemies[0].staggerActive=true;
    snapshot.barEnemies[0].staggerSeconds=1.25;
    geometry=buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,pose);
    requireHealthBar(geometry.triangles.size()>ordinaryVertices&&healthBarColorVertices(geometry,1,.8f,.18f)>0,"known active stagger adds a separate countdown label above HP");
    snapshot.barEnemies[0].staggerKnown=false;
    geometry=buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,pose);
    requireHealthBar(geometry.triangles.size()==ordinaryVertices,"unknown stagger cannot produce a timer or active frame");
    snapshot.barEnemies[0].staggerActive=false;

    geometry=buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,[](InvisibleEnemyRow& refreshed,Vec3& top) {
        top={1,4,2}; refreshed.health=50; refreshed.staggerKnown=refreshed.staggerActive=true; refreshed.staggerSeconds=.5; return true;
    });
    requireHealthBar(healthBarColorVertices(geometry,1,.5f,.06f)==6&&healthBarColorVertices(geometry,1,.8f,.18f)>0,"one validation pass can refresh both actual health and stagger before drawing");

    snapshot.barEnemies.push_back(snapshot.barEnemies[0]); validations=0;
    requireHealthBar(buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,pose).bars==1&&validations==1,"one world bar per validated entity even with duplicate membership");
    snapshot.barEnemies.resize(1);
    requireHealthBar(buildEnemyHealthBars(camera,snapshot,level,1251,40,1080,pose).triangles.empty(),"stale data draws no health panel");
    auto other=level; other.world++;
    requireHealthBar(buildEnemyHealthBars(camera,snapshot,other,1000,40,1080,pose).triangles.empty(),"old-world bars disappear");
    requireHealthBar(buildEnemyHealthBars(camera,snapshot,level,1000,40,0,pose).triangles.empty(),"missing viewport data cannot create invalid vertices");
    requireHealthBar(buildEnemyHealthBars(camera,snapshot,level,1000,1,1080,pose).triangles.empty(),"distant enemies do not generate glyph meshes");
    requireHealthBar(buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,[](auto&,Vec3&) { return false; }).triangles.empty(),"failed identity/death validation hides the complete panel");
    for(double value:{0.,-1.,std::numeric_limits<double>::quiet_NaN()}) {
        snapshot.barEnemies[0].health=value;
        requireHealthBar(buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,pose).triangles.empty(),"dead or unreadable HP cannot produce a panel");
    }
    snapshot.barEnemies[0].health=100;
    for(float depth:{5.f,20.f}) for(uint32_t height:{720u,1080u,2160u}) {
        auto sized=buildEnemyHealthBars(camera,snapshot,level,1000,40,height,[&](auto&,Vec3& top) { top={0,depth,2}; return true; });
        requireHealthBar(sized.bars==1,"panel remains visible at ordinary distances and display scales");
        float minimum=FLT_MAX,maximum=-FLT_MAX;
        for(size_t i=0;i<6;++i) { minimum=std::min(minimum,sized.triangles[i].position.x); maximum=std::max(maximum,sized.triangles[i].position.x); }
        float projectedWidth=(maximum-minimum)*static_cast<float>(height)/(2*depth);
        requireHealthBar(healthBarNear(projectedWidth,99.f,.001f),"bar pixel width stays readable across depth and viewport changes");
    }
    geometry=buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,pose);
    const size_t capacity=geometry.triangles.capacity();
    populateEnemyHealthBars(geometry,camera,snapshot,level,1000,40,1080,pose);
    requireHealthBar(geometry.triangles.capacity()==capacity&&geometry.bars==1,"repeated frames retain the geometry allocation");
    populateEnemyHealthBars(geometry,camera,snapshot,level,2000,40,1080,pose);
    requireHealthBar(geometry.triangles.empty()&&geometry.triangles.capacity()==capacity,"stale frames clear visible geometry while retaining reusable capacity");

    auto preview=snapshot; preview.barEnemies.clear();
    for(unsigned i=0;i<3;++i) {
        auto example=row; example.entity.address+=i*32; example.entity.id+=i;
        example.health=i==0?200:i==1?100:25; example.top={static_cast<float>(int(i)*13-13),10,0};
        example.staggerKnown=example.staggerActive=i==2; example.staggerSeconds=2.5; example.staggerDuration=999960LL*5; preview.barEnemies.push_back(example);
    }
    GameCamera previewCamera=camera; previewCamera.matrix[0]=.5f;
    const auto image=buildEnemyHealthBars(previewCamera,preview,level,1000,40,240,[](auto& current,Vec3& top) { top=current.top; return true; });
    requireHealthBar(image.bars==3,"preview includes full, half and low health panels");
    saveHealthBarPreview(image,previewCamera,"build/health-bar-render-preview.ppm");
    puts("PASS: solid health fill, green/yellow/red colors, current/max numerals, stagger countdown, fresh pose/state, projection sizing and safety gates.");
}
