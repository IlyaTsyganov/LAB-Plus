#include "../src/vulkan_layer.cpp"
static void check(bool condition,const char* text){if(!condition){fprintf(stderr,"FAIL: %s\n",text);exit(1);}}
int main(){
    wchar_t temporaryFolder[MAX_PATH]{},file[MAX_PATH]{};
    check(GetTempPathW(MAX_PATH,temporaryFolder)&&GetTempFileNameW(temporaryFolder,L"lpa",0,file),"temporary settings");
    const wchar_t* colors[]={L"WorldColor",L"TriggerColor",L"WallColor",L"InspectorColor"};
    const wchar_t* opacity[]={L"WorldFillOpacity",L"TriggerFillOpacity",L"WallFillOpacity",L"InspectorFillOpacity"};
    const wchar_t* colorValues[]={L"#123456",L"#3b6f9d",L"#d3a527",L"#b629e1"};
    const std::array<float,3> expectedColors[]{{18.f/255,52.f/255,86.f/255},{59.f/255,111.f/255,157.f/255},{211.f/255,165.f/255,39.f/255},{182.f/255,41.f/255,225.f/255}};
    const wchar_t* values[]={L"0",L"37",L"100",L"64"};
    const float expectedOpacity[]{0,.37f,1,.64f};
    for(size_t i=0;i<4;++i){WritePrivateProfileStringW(L"Viewer",colors[i],colorValues[i],file);WritePrivateProfileStringW(L"Viewer",opacity[i],values[i],file);}
    readVolumeAppearances(file);
    Mesh mesh{};mesh.volumeState=EntityStatus::Enabled;
    // Settings/UI order is World, Trigger, Wall, Inspector. Mesh kinds use
    // Trigger=0, World=1, Wall=2; distinct colors AND opacity catch either swap.
    const size_t appearanceForKind[]{1,0,2};
    for(uint32_t kind=0;kind<3;++kind){
        mesh.kind=kind;const size_t appearance=appearanceForKind[kind];auto style=volumeStyle(mesh,false,0);
        for(int c=0;c<3;++c){check(style.line[c]==style.fill[c],"one RGB color for contour and fill");check(style.fill[c]==expectedColors[appearance][c],"each geometry category uses its own UI color");}
        check(style.fill[3]==expectedOpacity[appearance],"each geometry category uses its own UI fill opacity");
        auto selected=volumeStyle(mesh,true,0);
        for(int c=0;c<3;++c)check(selected.fill[c]==expectedColors[3][c]&&selected.line[c]==expectedColors[3][c],"inspector selection overrides each category with its independent color");
        check(selected.fill[3]==.64f&&selected.line[3]==1.f,"inspector uses independent fill opacity");
    }
    {
        GameCamera camera{};camera.matrix[0]=camera.matrix[5]=camera.matrix[10]=camera.matrix[15]=1;
        meshes.clear();liveWalls.meshes.clear();spatialNodes.clear();spatialOrder.clear();
        for(uint32_t kind=0;kind<3;++kind){
            auto geometry=std::make_shared<Geometry>();geometry->kind=kind;
            geometry->triangles={{-.2f,-.2f,.2f},{.2f,-.2f,.2f},{0,.2f,.2f}};geometry->lines={{-.2f,-.2f,.2f},{.2f,-.2f,.2f}};
            Mesh fixture{};fixture.kind=kind;fixture.geometry=geometry;
            const float bounds[]{-.2f,-.2f,.2f,.2f,.2f,.2f};memcpy(fixture.bounds,bounds,sizeof(bounds));
            if(kind<2)meshes.push_back(fixture);else liveWalls.meshes.push_back(fixture);
        }
        spatialOrder={0,1};buildSpatialNode(0,2);showTriggers=showCollision=showWalls=true;collisionMode=1;
        std::vector<Vertex> output;const auto fill=collectVertices(camera,output);
        check(fill==6&&output.size()==12,"zero-opacity world keeps contours while trigger and wall fills remain present");
        const size_t categoryAtVertex[]{1,1,1,2,2,2,0,0,1,1,2,2};
        for(size_t i=0;i<output.size();++i){
            const size_t category=categoryAtVertex[i];
            for(int c=0;c<3;++c)check(output[i].color[c]==expectedColors[category][c],"draw vertices retain the color of their geometry category");
            if(i<fill)check(output[i].color[3]==expectedOpacity[category],"filled draw vertices retain their category opacity");
        }
    }
    check(volumeAppearances[0].opacity==0&&volumeAppearances[1].opacity==.37f&&volumeAppearances[2].opacity==1,"opacity endpoints and independent values");
    WritePrivateProfileStringW(L"Viewer",L"WorldColor",L"#oopsxx",file);WritePrivateProfileStringW(L"Viewer",L"WorldFillOpacity",L"200",file);readVolumeAppearances(file);
    check(volumeAppearances[0].color[0]==18.f/255&&volumeAppearances[0].opacity==1,"invalid color preserved and opacity clamped");
    DeleteFileW(file);puts("PASS: distinct World/Trigger/Wall/Inspector colors and opacity map to the correct mesh kinds and draw vertices; line/fill RGB match and zero-opacity world retains contours.");
}
