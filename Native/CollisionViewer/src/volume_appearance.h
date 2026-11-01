#pragma once

struct VolumeAppearance {
    std::array<float,3> color;
    float opacity;
};
static std::array<VolumeAppearance,4> volumeAppearances{{
    {{38.f/255,204.f/255,1.f},.09f},
    {{64.f/255,242.f/255,31.f/255},.09f},
    {{1.f,.45f,.05f},.09f},
    {{1.f,217.f/255,26.f/255},.20f}
}};
static void readVolumeAppearances(const std::wstring& path) {
    const wchar_t* colors[]={L"WorldColor",L"TriggerColor",L"WallColor",L"InspectorColor"};
    const wchar_t* opacities[]={L"WorldFillOpacity",L"TriggerFillOpacity",L"WallFillOpacity",L"InspectorFillOpacity"};
    for(size_t i=0;i<volumeAppearances.size();++i) {
        wchar_t value[32]{};
        GetPrivateProfileStringW(L"Viewer",colors[i],L"",value,32,path.c_str());
        auto hex=[](wchar_t c)->int { if(c>=L'0'&&c<=L'9')return c-L'0';if(c>=L'a'&&c<=L'f')return c-L'a'+10;if(c>=L'A'&&c<=L'F')return c-L'A'+10;return -1; };
        bool valid=wcslen(value)==7&&value[0]==L'#';
        for(size_t j=1;valid&&j<7;++j)valid=hex(value[j])>=0;
        if(valid) for(size_t j=0;j<3;++j)volumeAppearances[i].color[j]=float(hex(value[j*2+1])*16+hex(value[j*2+2]))/255.f;
        GetPrivateProfileStringW(L"Viewer",opacities[i],L"",value,32,path.c_str());
        if(value[0]) {
            wchar_t* end=nullptr;long opacity=wcstol(value,&end,10);
            if(end!=value&&!*end)volumeAppearances[i].opacity=float(std::clamp(opacity,0L,100L))/100.f;
        }
    }
}
