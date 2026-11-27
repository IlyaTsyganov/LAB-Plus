using System.Text.Json;
using LabPlus.Hud.Runtime;
using LabPlus.Hud.Overlay;

internal static class BooleanToggleChecks
{
    internal static void Run(Action<bool, string> check)
    {
        var custom=new LabSettings{TintOpacity=37.5,VerticalShowSign=false}; custom.Current.X=999;
        foreach(int command in Enumerable.Range(0,NativeSpeedometerCode.Commands.Length)) {
            string text=ConsoleControls.Help(custom,command); var lines=text.TrimEnd('\n').Split('\n');
            check(lines.Length==2&&lines[0].StartsWith("\""+NativeSpeedometerCode.Commands[command]+"\" is:\"")&&
                lines[0].Contains("\"^7 default:\"")&&lines[0].EndsWith("\"")&&lines[1].StartsWith("^7"),
                "All HUD queries use the exact two-line engine layout: "+command);
        }
        check(ConsoleControls.Help(custom,4).StartsWith("\"labplus_lowhptintopacity\" is:\"37.5\"^7 default:\"100\"\n"),"Current opacity and factory default share the first line");
        check(ConsoleControls.Help(custom,8).StartsWith("\"labplus_speedometer_sign_vertical\" is:\"0\"^7 default:\"1\"\n"),"Changed sign preference does not become the default");
        check(ConsoleControls.Help(custom,1).Contains("is:\"current=999")&&ConsoleControls.Help(custom,1).Contains("default:\"current=160"),"Per-meter coordinates keep independent current and default values");
        foreach (var (command, read) in new (int, Func<LabSettings, bool>)[] {
            (0, s => s.SpeedometersEnabled), (5, s => s.Landing.Enabled),
            (7, s => s.Vertical.Enabled), (8, s => s.VerticalShowSign) })
        {
            var settings = new LabSettings();
            bool before = read(settings);
            check(ConsoleControls.Apply(settings, command, ["toggle"]) && read(settings) != before
                && ConsoleControls.Apply(settings, command, ["toggle"]) && read(settings) == before,
                "Binary HUD control toggles both ways: " + command);
            check(ConsoleControls.Help(settings, command).Contains("<0|1>")&&!ConsoleControls.Help(settings, command).Contains("toggle"), "Binary HUD help describes values without suggesting a postfix toggle argument: " + command);
        }
        foreach (string target in new[] { "current", "landing", "vertical" })
        {
            var settings = new LabSettings();
            check(ConsoleControls.Apply(settings, 0, ["toggle", target]) && !settings.Meter(target).Enabled
                && !settings.SpeedometersEnabled && new[] { "current", "landing", "vertical" }.Where(x => x != target).All(x => settings.Meter(x).Enabled),
                "Targeted HUD toggle changes only its meter: " + target);
        }
        var master = new LabSettings(); master.Landing.Enabled = master.Vertical.Enabled = false;
        check(ConsoleControls.Apply(master, 0, ["toggle"]) && master.SpeedometersEnabled && master.Current.Enabled
            && !master.Landing.Enabled && !master.Vertical.Enabled, "Master toggle preserves secondary choices");
        foreach (var (command, arguments) in new (int, string[])[] {
            (1, ["toggle"]), (2, ["toggle"]), (3, ["toggle"]), (4, ["toggle"]), (6, ["toggle"]),
            (0, ["toggle", "unknown"]), (0, ["toggle", "current;quit"]), (0, ["toggle", "current", "extra"]),
            (5, ["toggle", "current"]), (7, ["toggle", "vertical"]), (8, ["toggle", "vertical"]) })
        {
            var settings = new LabSettings(); string before = JsonSerializer.Serialize(settings);
            check(!ConsoleControls.Apply(settings, command, arguments) && JsonSerializer.Serialize(settings) == before,
                "Invalid toggle preserves HUD preferences: " + command);
        }
        string directory = Path.Combine(Path.GetTempPath(), "labplus-toggle-test-" + Guid.NewGuid().ToString("N"));
        try
        {
            var store = new SettingsStore(directory);
            check(store.Update(s => ConsoleControls.Apply(s, 8, ["toggle"])) && !new SettingsStore(directory).Snapshot().VerticalShowSign,
                "Toggle persists across HUD restart");
            check(File.ReadAllLines(store.ConsoleSettingsPath).Contains("VerticalShowSign=0"), "HUD cvar snapshot follows the saved sign preference");
            check(store.Update(s => ConsoleControls.Apply(s, 8, ["toggle"])) && new SettingsStore(directory).Snapshot().VerticalShowSign,
                "Second toggle restores and persists the current value");
            foreach (var (command, key) in new[] { (0, "SpeedometersEnabled"), (5, "LandingEnabled"), (7, "VerticalEnabled"), (8, "VerticalShowSign") })
                foreach (string value in new[] { "0", "1" })
                    check(store.Update(s => ConsoleControls.Apply(s, command, [value])) && File.ReadAllLines(store.ConsoleSettingsPath).Contains(key + "=" + value),
                        "Native cvar transport updates the matching persisted HUD setting: " + key);
        }
        finally { Directory.Delete(directory, true); }
    }
}
