#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <psapi.h>
#include <algorithm>
#include <cwctype>

static bool loadedLibrary(const wchar_t* fragment) {
    HMODULE modules[1024]{};DWORD bytes=0;
    if(!K32EnumProcessModules(GetCurrentProcess(),modules,sizeof(modules),&bytes))return false;
    for(size_t i=0;i<std::min(size_t(bytes/sizeof(HMODULE)),size_t(1024));++i){
        wchar_t file[32768]{},expanded[32768]{};
        if(!GetModuleFileNameW(modules[i],file,32768))continue;
        std::wstring name=GetLongPathNameW(file,expanded,32768)?expanded:file;
        std::transform(name.begin(),name.end(),name.begin(),[](wchar_t c){return std::towlower(c);});
        if(name.find(fragment)!=std::wstring::npos)return true;
    }
    return false;
}

int wmain(int argc, wchar_t** argv) {
    bool joint=argc==3&&wcscmp(argv[2],L"--joint")==0;
    bool absent=argc==3&&wcscmp(argv[2],L"--absent")==0;
    if (argc != 2 && !joint && !absent) {
        std::wcerr << L"Usage: LayerLoaderProbe <layer-runtime-directory>\n";
        return 2;
    }

    HMODULE loader = LoadLibraryW(L"vulkan-1.dll");
    if (!loader) {
        std::cerr << "Could not load the Windows Vulkan loader.\n";
        return 3;
    }

    auto getInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(loader, "vkGetInstanceProcAddr"));
    auto createInstance = getInstanceProcAddr
        ? reinterpret_cast<PFN_vkCreateInstance>(getInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"))
        : nullptr;
    if (!createInstance) {
        std::cerr << "The Vulkan loader did not expose vkCreateInstance.\n";
        return 4;
    }

    VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application.pApplicationName = "LAB Plus layer loader probe";
    application.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &application;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = createInstance(&createInfo, nullptr, &instance);
    bool collisionLoaded=loadedLibrary(L"labplus-layer-"),lightingLoaded=loadedLibrary(L"labplus-lighting-");
    if (instance) {
        auto destroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(getInstanceProcAddr(instance, "vkDestroyInstance"));
        if (destroyInstance) destroyInstance(instance, nullptr);
    }
    if(absent) {
        if(result!=VK_SUCCESS||collisionLoaded||lightingLoaded) {
            std::cerr<<"External launch unexpectedly loaded LAB Plus. vkCreateInstance="<<result<<" collision="<<collisionLoaded<<" lighting="<<lightingLoaded<<"\n";
            return 5;
        }
        std::cout<<"PASS: external Vulkan process loaded neither LAB Plus layer.\n";
        return 0;
    }

    const std::filesystem::path logPath = std::filesystem::path(argv[1]) / L"VulkanViewer.log";
    std::ifstream logFile(logPath);
    std::string log((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
    if (result!=VK_SUCCESS || log.find("Collision Vulkan layer loaded.") == std::string::npos || !collisionLoaded || (joint&&!lightingLoaded)) {
        std::cerr << "Layer loading failed. vkCreateInstance=" << result << " collision=" << collisionLoaded << " lighting=" << lightingLoaded << "\n";
        return 5;
    }

    std::cout << "PASS: Vulkan Loader called the LAB Plus layer. vkCreateInstance result=" << result << "\n";
    if(joint)std::cout << "PASS: both collision and controllable lighting DLLs were loaded by the real Vulkan loader in the same process.\n";
    return 0;
}
