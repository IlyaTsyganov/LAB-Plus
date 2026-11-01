using System.Diagnostics;
using LabPlus.Hud.Runtime;

internal static class FirewallLaunchTests
{
    internal static void Run(Action<bool, string> check)
    {
        string root = Path.Combine(Path.GetTempPath(), "labplus-firewall-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            string exe = Path.Combine(root, "DOOMEternalx64vk.exe");
            File.WriteAllBytes(exe, [1, 2, 3]); // Non-executable; tests inject the process starter.
            var store = new SettingsStore(Path.Combine(root, "hud"));
            store.Update(s => { s.GamePath = exe; s.LightingFix = true; return true; });
            int started = 0, inspections = 0;
            try
            {
                GameLauncher.Start(store, path => { check(path == exe, "HUD launch inspects the exact executable"); return new(false, "hud-blocked"); }, _ =>
                { started++; return Process.GetCurrentProcess(); });
                throw new Exception("HUD launcher bypassed firewall gate");
            }
            catch (InvalidOperationException ex) when (ex.Message == "hud-blocked") { }
            check(started == 0 && !Directory.Exists(Path.Combine(store.DirectoryPath, "lighting")), "HUD firewall rejection occurs before lighting preparation or Process.Start");
            store.Update(s => { s.LightingFix = false; return true; });
            try
            {
                GameLauncher.Start(store, _ => ++inspections == 1 ? new(true, "protected") : new(false, "hud-rule-disabled"), _ =>
                { started++; return Process.GetCurrentProcess(); });
                throw new Exception("HUD launcher used a stale firewall result");
            }
            catch (InvalidOperationException ex) when (ex.Message == "hud-rule-disabled") { }
            check(inspections == 2 && started == 0, "HUD launcher rechecks the rule with all optional layers disabled");
            inspections = 0;
            GameLauncher.Start(store, _ => { inspections++; return new(true, "protected"); }, info =>
            { check(info.FileName == exe && !info.UseShellExecute, "HUD starter receives the protected direct executable"); started++; return Process.GetCurrentProcess(); });
            check(inspections == 2 && started == 1, "HUD launcher starts only after two successful current firewall checks");
        }
        finally { Directory.Delete(root, true); }
    }
}
