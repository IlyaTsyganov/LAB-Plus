using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;
using LabPlus.Hud.Memory;
using LabPlus.Hud.Overlay;
using LabPlus.Hud.Runtime;

// Explicit opt-in, read-only acceptance checks against LAB's existing adapters.
internal static class LiveGameChecks
{
    internal static int InspectMetrics()
    {
        using var game=Process.GetProcessesByName("DOOMEternalx64vk").Single();
        IntPtr image=game.MainModule!.BaseAddress;
        var layout=NativeGameLayout.Resolve(game.MainModule.FileName);
        var metrics=layout.Metrics??throw new Exception(layout.MetricsFailure);
        using var reader=new ReadOnlyMemory(game.Id);
        string Text(IntPtr at)=>Encoding.UTF8.GetString(reader.Read(at,128)).Split('\0')[0];
        Console.WriteLine($"READ-ONLY PID {game.Id}; fps={Text(image+metrics.FpsFormat)}; frameTime={Text(image+metrics.FpsFormat+8)}; last={Text(image+metrics.LastFormat)}");
        Console.WriteLine("Detected hardware labels: "+string.Join(" | ",LabPlus.Game.HardwareNames.Current));
        byte[] entry=reader.Read(image+metrics.Function,16);
        if(!entry.AsSpan(0,6).SequenceEqual(Convert.FromHexString("FF2500000000"))) {Console.WriteLine("No metrics adapter installed");return 1;}
        IntPtr adapter=new(BitConverter.ToInt64(entry,6)),state=adapter+4096;
        Console.WriteLine($"Magic={Text(adapter+0x700)}; lease={BitConverter.ToInt64(reader.Read(state,8))}; now={Environment.TickCount64}; DESRU={BitConverter.ToInt32(reader.Read(state+8,4))}; Hide={BitConverter.ToInt32(reader.Read(state+12,4))}; indicator={Text(state+64)}");
        return 0;
    }
    private sealed class ReadOnlyMemory : IDisposable
    {
        private readonly IntPtr _handle;
        internal ReadOnlyMemory(int pid) { _handle=OpenProcess(0x1010,false,pid);if(_handle==IntPtr.Zero)throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error()); }
        internal byte[] Read(IntPtr at,int length) { var bytes=new byte[length];if(!ReadProcessMemory(_handle,at,bytes,(UIntPtr)length,out var got)||got.ToUInt64()!=(ulong)length)throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());return bytes; }
        public void Dispose()=>CloseHandle(_handle);
        [DllImport("kernel32.dll",SetLastError=true)]private static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
        [DllImport("kernel32.dll",SetLastError=true)]private static extern bool ReadProcessMemory(IntPtr process,IntPtr at,byte[] bytes,UIntPtr size,out UIntPtr read);
        [DllImport("kernel32.dll")]private static extern bool CloseHandle(IntPtr handle);
    }
    internal static int InspectPlayerCheats()
    {
        using var game=Process.GetProcessesByName("DOOMEternalx64vk").Single();
        var path=game.MainModule!.FileName;
        var layout=NativeGameLayout.Resolve(path);
        var flags=layout.PlayerCheats??throw new NotSupportedException(layout.PlayerCheatsFailure);
        var reader=new GameReader(game,GameProfile.Resolve(path));
        IntPtr player=reader.Player;var current=new PlayerCheatMonitor(game,flags).Read(reader.Active?player:IntPtr.Zero);
        Console.WriteLine($"READ-ONLY PID {game.Id}; active={reader.Active}; player=0x{player.ToInt64():x}; {flags}; noclip={current.Noclip}, notarget={current.Notarget}, god={current.God}");
        return 0;
    }

    internal static int InspectCvarSession()
    {
        using var game = Process.GetProcessesByName("DOOMEternalx64vk").Single();
        var layout = NativeGameLayout.Resolve(game.MainModule!.FileName);
        var monitor = new ProtectedCvarMonitor(game,layout.Cvars ?? throw new Exception(layout.CvarsFailure));
        IntPtr image = game.MainModule.BaseAddress, slot = image + layout.HudSlot;
        slot = HudChain.UnwrapHud(game,slot,game.ReadPointer(slot));
        byte[] head = game.ReadBytes(game.ReadPointer(slot),20) ?? throw new Exception("Cannot read HUD");
        if(!head.AsSpan(0,12).SequenceEqual(Convert.FromHexString("5356574883EC40488BF248BB"))) throw new Exception("Unknown HUD hook");
        IntPtr state = new(BitConverter.ToInt64(head,12));
        var session = new CheatCvarSession(game,state,monitor);
        var baseline = (IReadOnlyList<CvarDifference>)typeof(CheatCvarSession).GetField("_baseline",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance)!.GetValue(session)!;
        var current = monitor.Read(); var displayed = session.DisplayChanges(current);
        Console.WriteLine($"READ-ONLY PID {game.Id}: active={session.Active}, baseline={baseline.Count}, nondefault={current.Count}, filtered={displayed.Count}; layout={layout.Cvars}");
        foreach(var item in baseline) Console.WriteLine($"BASELINE {item.Name}={item.Value} reset={item.Default} flags=0x{item.Flags:x}");
        foreach(var item in displayed) Console.WriteLine($"DISPLAY {item.Name}={item.Value} reset={item.Default} flags=0x{item.Flags:x}");
        var executableType = typeof(NativeGameLayout).GetNestedType("Executable",System.Reflection.BindingFlags.NonPublic)!;
        using(var executable = (IDisposable)Activator.CreateInstance(executableType,System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic,null,[game.MainModule.FileName],null)!) {
            var enumValue = executableType.GetMethod("EnumValue",System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic)!;
            foreach(string name in new[]{"CVAR_EXPOSE","CVAR_LOCAL_SETTING","CVAR_ARCHIVE","CVAR_CHEAT","CVAR_NOCHEAT","CVAR_RESOURCE_DEPENDENCY_GREATER_THAN","CVAR_RESOURCE_DEPENDENCY_LESS_THAN"})
                Console.WriteLine($"ENUM {name}=0x{(int)enumValue.Invoke(executable,["cvarFlags_t",name])!:x}");
        }
        var cvarLayout = layout.Cvars!;
        IntPtr system = game.ReadPointer(image+cvarLayout.SystemGlobal), list = game.ReadPointer(system+8);
        byte[] registry = game.ReadBytes(list,16)!; IntPtr array = new(BitConverter.ToInt64(registry));
        int count = BitConverter.ToInt32(registry,8);
        if(count is <1 or >65536) throw new Exception("Invalid registry");
        byte[] pointers = game.ReadBytes(array,count*8)!;
        for(int i=0;i<count;i++) {
            IntPtr data = game.ReadPointer(new IntPtr(BitConverter.ToInt64(pointers,i*8)));
            byte[]? bytes = game.ReadBytes(data,cvarLayout.DataSize); if(bytes is null) continue;
            string name = game.ReadString(new IntPtr(BitConverter.ToInt64(bytes,cvarLayout.Name)),ReadStringType.UTF8,256);
            if(!(name.Contains("viewAngleimportance",StringComparison.OrdinalIgnoreCase)||name.Contains("skill",StringComparison.OrdinalIgnoreCase)||name.Contains("difficulty",StringComparison.OrdinalIgnoreCase)||name.StartsWith("s_volume")||name=="g_infiniteAmmo"||name=="timescale")) continue;
            string value = game.ReadString(new IntPtr(BitConverter.ToInt64(bytes,cvarLayout.ValueString)),ReadStringType.UTF8,256);
            Console.WriteLine($"REGISTERED {name}={value} flags=0x{BitConverter.ToInt32(bytes,cvarLayout.Flags):x}");
        }
        return 0;
    }

    internal static int BenchmarkCvars()
    {
        var settings = new SettingsStore().Snapshot();
        using var game = Process.GetProcessesByName("DOOMEternalx64vk").Single(p => p.MainModule!.FileName.Equals(settings.GamePath,StringComparison.OrdinalIgnoreCase));
        var layout = NativeGameLayout.Resolve(settings.GamePath);
        var cvars = layout.Cvars ?? throw new NotSupportedException(layout.CvarsFailure);
        var monitor = new ProtectedCvarMonitor(game,cvars);
        var watch = Stopwatch.StartNew(); var differences = monitor.Read(); watch.Stop();
        double initial = watch.Elapsed.TotalMilliseconds;
        watch.Restart();for(int i=0;i<30;i++) differences = monitor.Read();watch.Stop();
        Console.WriteLine($"READ-ONLY: PID {game.Id}; initial cvar registry scan {initial:F2} ms; cached scans average {watch.Elapsed.TotalMilliseconds/30:F2} ms; {differences.Count} nondefault protected cvars.");
        foreach(var difference in differences) Console.WriteLine(difference.Name + " = " + difference.Value + " (default: " + difference.Default + ")");
        return 0;
    }

    internal static int Run(bool active,bool restart=false)
    {
        var store=new SettingsStore();var settings=store.Snapshot();
        using var game=Process.GetProcessesByName("DOOMEternalx64vk").Single(p=>p.MainModule!.FileName.Equals(settings.GamePath,StringComparison.OrdinalIgnoreCase));
        var layout=NativeGameLayout.Resolve(settings.GamePath);var profile=GameProfile.Resolve(settings.GamePath);var reader=new GameReader(game,profile);
        IntPtr image=game.MainModule!.BaseAddress;
        byte[] Read(IntPtr address,int size)=>game.ReadBytes(address,size)??throw new IOException("Cannot read existing LAB adapter");
        var entry=Read(image+layout.Metrics!.Function,16);
        if(!entry.AsSpan(0,6).SequenceEqual(Convert.FromHexString("FF2500000000")))throw new Exception("LAB metrics adapter is not installed");
        IntPtr metrics=new(BitConverter.ToInt64(entry,6));
        if(Encoding.ASCII.GetString(Read(metrics+0x700,8))!=PerformanceMetrics.Magic)throw new Exception("Unknown metrics adapter");
        long lease=game.ReadValue<long>(metrics+4096);string indicator=Encoding.ASCII.GetString(Read(metrics+4096+64,64)).Split('\0')[0];
        string expected=restart?PerformanceMetrics.RestartText:LabIndicator.Format(settings);
        if(active?(lease<=Environment.TickCount64||indicator!=expected):lease>Environment.TickCount64)throw new Exception("Unexpected metrics lease or indicator: "+indicator);
        if(active){
            IntPtr hudSlot=image+layout.HudSlot;hudSlot=HudChain.UnwrapHud(game,hudSlot,game.ReadPointer(hudSlot));
            IntPtr render=game.ReadPointer(hudSlot);var head=Read(render,20);
            bool ours=head.AsSpan(0,12).SequenceEqual(Convert.FromHexString("5356574883EC40488BF248BB"));
            if(!ours&&!restart)throw new Exception("Unknown LAB HUD renderer");
            if(ours){
                IntPtr hudState=new(BitConverter.ToInt64(head,12));
                if(restart){
                    if(game.ReadValue<int>(hudState)>0||game.ReadValue<int>(hudState+LowHealthTintCode.LeaseOffset)>0)
                        throw new Exception("Normal HUD/tint features are active in restart mode");
                }else{
                    if(Encoding.ASCII.GetString(Read(hudState+48,8))!=NativeSpeedometerCode.Magic||game.ReadValue<int>(hudState+4)!=NativeSpeedometerCode.Commands.Length)throw new Exception("New LAB console command registration is not ready");
                    string lastName=Encoding.ASCII.GetString(Read(hudState+NativeSpeedometerCode.NamesOffset+8*256,64)).Split('\0')[0];
                    if(lastName!="labplus_speedometer_verticalsign")throw new Exception("Vertical sign command is missing");
                }
            }
        }
        var modbot=layout.ModbotInput??throw new Exception(layout.ModbotInputFailure);
        IntPtr callback=game.ReadPointer(image+modbot.MenuVtable+modbot.ShowSlot*8),state=callback+4096;
        bool guard=(game.ReadBytes(state,8) is byte[] magic)&&Encoding.ASCII.GetString(magic)==NativeModbotInputCode.Magic;
        if(!guard&&!restart)throw new Exception("LAB Modbot adapter is not installed");
        int enabled=guard?game.ReadValue<int>(state+NativeModbotInputCode.Enabled):0;
        long inputLease=guard?game.ReadValue<long>(state+NativeModbotInputCode.Lease):0;
        bool expectedInput=active&&!restart&&settings.ModbotFix&&reader.MaxHz is >0 and <1000;
        if((enabled!=0)!=expectedInput||(active&&!restart&&inputLease<=Environment.TickCount64))throw new Exception("Unexpected Modbot activation/lease");
        if(restart&&(!File.ReadAllText(Path.Combine(store.DirectoryPath,"lighting.ini")).Contains("Enabled=0")||game.ReadValue<int>(metrics+4096+12)!=0))
            throw new Exception("Normal lighting or driver filtering is active in restart mode");
        Console.WriteLine($"PASS: game {profile.Name}, PID {game.Id}; {(active?indicator:"stopped")}; normal features {(restart?"disabled":"available")}");
        return 0;
    }
}
