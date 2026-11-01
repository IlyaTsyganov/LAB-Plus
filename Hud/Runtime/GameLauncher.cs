using System.Diagnostics;
using System.Text.Json;
using System.Runtime.InteropServices;
using System.Text;
namespace LabPlus.Hud.Runtime;
internal static class GameLauncher
{
    private static readonly object ControlGate=new();
    internal static string PrepareLighting(SettingsStore store)
    {
        string folder=Path.Combine(store.DirectoryPath,"lighting");Directory.CreateDirectory(folder);
        string source=Path.Combine(AppContext.BaseDirectory,"Native/Lighting/labplus-lighting.dll");if(!File.Exists(source))throw new FileNotFoundException("The LAB lighting library is missing. Extract the complete application archive.",source);
        string hash=Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(File.ReadAllBytes(source)))[..16].ToLowerInvariant();string library=Path.Combine(folder,"labplus-lighting-"+hash+".dll");if(!File.Exists(library))File.Copy(source,library);
        var manifest=new{file_format_version="1.1.2",layer=new{name="VK_LAYER_LABPLUS_lighting",type="GLOBAL",library_path=LoaderPath(library),api_version="1.1.0",implementation_version="1",description="LAB lighting barrier correction",functions=new{vkNegotiateLoaderLayerInterfaceVersion="labplus_hud_negotiate_layer_interface_version"}}};
        File.WriteAllText(Path.Combine(folder,"labplus-lighting.json"),JsonSerializer.Serialize(manifest,SettingsStore.Json));WriteLightingControl(store);return LoaderPath(folder);
    }
    internal static bool WasLaunchedByLab(Process game,SettingsStore store)
    {
        return GameLaunchHistory.Matches(game,store);
    }
    internal static string LoaderPath(string path){var buffer=new StringBuilder(32768);uint count=GetShortPathName(path,buffer,(uint)buffer.Capacity);return count>0&&count<buffer.Capacity?buffer.ToString():path;}
    internal static bool LightingActive(int pid){try{using var status=EventWaitHandle.OpenExisting("Local\\LABPLUS.Lighting.Active."+pid);return status.WaitOne(0);}catch(WaitHandleCannotBeOpenedException){return false;}catch(UnauthorizedAccessException){return false;}}
    [DllImport("kernel32.dll",EntryPoint="GetShortPathNameW",CharSet=CharSet.Unicode)]private static extern uint GetShortPathName(string path,StringBuilder output,uint capacity);
    internal static void WriteLightingControl(SettingsStore store,bool running=true){lock(ControlGate){using var owner=Process.GetCurrentProcess();string path=Path.Combine(store.DirectoryPath,"lighting.ini");File.WriteAllText(path+".tmp","[Lighting]\nEnabled="+(running&&store.Snapshot().LightingFix&&global::LabPlus.Game.LightingFixPolicy.CanUse(store.Snapshot().GamePath)?"1":"0")+"\nOwnerPid="+owner.Id+"\nOwnerStarted="+owner.StartTime.ToUniversalTime().ToFileTimeUtc()+"\n");File.Move(path+".tmp",path,true);}}
    internal static ProcessStartInfo CreateStartInfo(LabSettings settings,string dataDirectory,string? lightingFolder=null)
    {
        string path=Path.GetFullPath(settings.GamePath);if(!File.Exists(path)||!Path.GetFileName(path).Equals("DOOMEternalx64vk.exe",StringComparison.OrdinalIgnoreCase))throw new ArgumentException("Select an existing DOOMEternalx64vk.exe.");
        var info=new ProcessStartInfo(path){UseShellExecute=false,WorkingDirectory=Path.GetDirectoryName(path)!};info.ArgumentList.Add("+com_skipIntroVideo");info.ArgumentList.Add("1");info.Environment["LABPLUS_HUD_DATA_DIRECTORY"]=dataDirectory;bool lighting=settings.LightingFix&&global::LabPlus.Game.LightingFixPolicy.CanUse(settings.GamePath);info.Environment["LABPLUS_HUD_LIGHTING_FIX"]=lighting?"1":"0";
        if(lighting){if(lightingFolder is null)throw new InvalidOperationException("Lighting runtime is not prepared.");info.Environment.TryGetValue("VK_INSTANCE_LAYERS",out string? oldLayers);info.Environment["VK_INSTANCE_LAYERS"]=string.Join(';',(oldLayers??"").Split(';',StringSplitOptions.RemoveEmptyEntries).Append("VK_LAYER_LABPLUS_lighting").Distinct());info.Environment.TryGetValue("VK_LAYER_PATH",out string? oldPath);info.Environment["VK_LAYER_PATH"]=string.IsNullOrEmpty(oldPath)?lightingFolder:lightingFolder+";"+oldPath;info.Environment["SteamAppId"]="782330";}
        info.Environment["LABPLUS_LAUNCHED"]="1";
        info.Environment["SteamAppId"]="782330";
        return info;
    }
    internal static void Start(SettingsStore store,Func<string,global::LabPlus.Game.GameFirewallResult>? inspect=null,Func<ProcessStartInfo,Process?>? start=null)
    {
        var s=store.Snapshot();
        s.GamePath=global::LabPlus.Game.GameLauncher.ValidateGamePath(s.GamePath);
        inspect??=global::LabPlus.Game.GameFirewall.Inspect;
        global::LabPlus.Game.GameFirewall.RequireBlocked(inspect(s.GamePath));
        string? folder=s.LightingFix&&global::LabPlus.Game.LightingFixPolicy.CanUse(s.GamePath)?PrepareLighting(store):null;
        var info=CreateStartInfo(s,store.DirectoryPath,folder);
        global::LabPlus.Game.GameFirewall.RequireBlocked(inspect(info.FileName));
        using var game=(start??Process.Start)(info)??throw new IOException("Game process did not start.");
        try{GameLaunchHistory.Record(game,s.GamePath,store);}catch(Exception e){Log.Warning(e,"Could not record LAB game launch");}
    }
}
