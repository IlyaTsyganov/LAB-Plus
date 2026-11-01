#include "../src/vulkan_layer.cpp"
#include <future>
static void expect(bool valid,const char* message) { if(!valid) { fprintf(stderr,"FAIL: %s\n",message);exit(1); } }
int main() {
    const auto suffix=std::to_wstring(GetCurrentProcessId());
    const auto eventName=L"Local\\LABPlus.Test.Active."+suffix,ownerName=L"Local\\LABPlus.Test.Owner."+suffix;
    HANDLE event=CreateEventW(nullptr,TRUE,TRUE,eventName.c_str()),owner=CreateMutexW(nullptr,FALSE,ownerName.c_str());
    expect(event&&owner,"isolated lifecycle objects created");
    expect(!labAppAvailable(eventName.c_str(),ownerName.c_str()),"signaled presence alone cannot enable commands or graphics");
    std::promise<void> ready,finish;auto done=finish.get_future();
    std::thread active([&] { expect(WaitForSingleObject(owner,0)==WAIT_OBJECT_0,"owner acquired");ready.set_value();done.wait(); });
    ready.get_future().wait();
    expect(labAppAvailable(eventName.c_str(),ownerName.c_str()),"live owner plus server enables functionality");
    HANDLE stale=OpenEventW(SYNCHRONIZE,FALSE,eventName.c_str());
    ResetEvent(event);CloseHandle(event);
    expect(!labAppAvailable(eventName.c_str(),ownerName.c_str()),"shutdown resets presence even while a reader holds a handle");
    event=CreateEventW(nullptr,TRUE,FALSE,eventName.c_str());SetEvent(event);
    finish.set_value();active.join();
    expect(!labAppAvailable(eventName.c_str(),ownerName.c_str()),"abandoned owner disables everything despite stale signaled presence");
    std::promise<void> restarted,stop;auto stopFuture=stop.get_future();
    std::thread next([&] { auto status=WaitForSingleObject(owner,0);expect(status==WAIT_OBJECT_0||status==WAIT_ABANDONED,"restart acquires previous owner's mutex");restarted.set_value();stopFuture.wait();ReleaseMutex(owner); });
    restarted.get_future().wait();expect(labAppAvailable(eventName.c_str(),ownerName.c_str()),"restarting the companion restores availability");
    stop.set_value();next.join();CloseHandle(stale);CloseHandle(event);CloseHandle(owner);

    wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
    const auto directory=std::filesystem::path(temp)/(L"labplus-lifetime-test-"+suffix);std::filesystem::create_directories(directory);
    SetEnvironmentVariableW(L"LABPLUS_DATA_DIRECTORY",directory.c_str());
    const auto ini=(directory/L"viewer.ini").wstring();
    for(const auto* key:{L"Enabled",L"InspectVolumes",L"ShowResearchPath",L"RecordResearch",L"ShowPlayerCollider",L"ShowMotionPaths",L"ShowSupportHud",L"ShowActiveEncountersHud",L"ShowEnemyHealthHud",L"ShowEnemyHealthBars",L"ShowGameTimersHud",L"ShowSpeedrunTechsHud"})
        WritePrivateProfileStringW(L"Viewer",key,L"1",ini.c_str());
    expect(applyCompanionAvailability(true)&&nativeOverlayNeeded(),"running companion restores its saved selections");
    expect(!applyCompanionAvailability(false)&&!nativeOverlayNeeded(),"shutdown removes all collision and telemetry overlays");
    expect(!enabled&&!inspectVolumes&&!recordResearch&&!showResearchPath&&!showPlayerCollider&&!showMotionPaths,"shutdown stops inspector, recording and motion visuals");
    expect(GetPrivateProfileIntW(L"Viewer",L"Enabled",0,ini.c_str())==1,"suspension preserves preferences");
    expect(applyCompanionAvailability(true)&&enabled&&showEnemyHealthBars&&showSpeedrunTechsHud,"next launch resumes saved features immediately");
    applyCompanionAvailability(false);std::filesystem::remove(ini);std::filesystem::remove(directory);
    puts("PASS: live owner, retained presence handles, abandoned ownership, restart, all overlay suspension and preference recovery.");
}
