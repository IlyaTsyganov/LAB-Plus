using System.Diagnostics;
using LabPlus.Game;

internal static class SteamStartupChecks
{
    internal static void Run(string root)
    {
        static void Check(bool value, string message) { if (!value) throw new Exception(message); }
        string directory = Path.Combine(root, "steam-startup"); Directory.CreateDirectory(directory);
        string executable = Path.Combine(directory, "steam.exe"); File.WriteAllText(executable, "fixture");
        int starts = 0, waits = 0; bool ready = true;
        Task Run(bool running) => SteamClientLauncher.EnsureRunningAsync(() => running, () => ready, () => executable,
            info => { Check(info.FileName == executable && !info.UseShellExecute && info.ArgumentList.SequenceEqual(new[] { "-silent" }), "Steam launch must not launch the game through Steam"); starts++; },
            _ => { waits++; ready = true; return Task.CompletedTask; });
        Run(true).GetAwaiter().GetResult(); Check(starts == 0 && waits == 0, "Ready Steam must not be restarted");
        ready = false; Run(false).GetAwaiter().GetResult();
        Check(starts == 1 && waits == 1, "Steam must be started once and awaited before the game");
        ready = false; Run(true).GetAwaiter().GetResult();
        Check(starts == 1 && waits == 2, "An initializing Steam process must be awaited without another client launch");
        try {
            SteamClientLauncher.EnsureRunningAsync(() => true, () => false, () => executable, _ => throw new Exception("Duplicate Steam"),
                _ => Task.CompletedTask).GetAwaiter().GetResult();
            throw new Exception("Steam readiness timeout was ignored");
        } catch (IOException) { }
        using var cancellation = new CancellationTokenSource();
        try {
            SteamClientLauncher.EnsureRunningAsync(() => true, () => false, () => executable, _ => throw new Exception("Duplicate Steam"),
                _ => { cancellation.Cancel(); return Task.CompletedTask; }, cancellation: cancellation.Token).GetAwaiter().GetResult();
            throw new Exception("Canceled Steam startup continued");
        } catch (OperationCanceledException) { }
        string[] keys = ["LABPLUS_LAUNCHED", "VK_INSTANCE_LAYERS", "SteamAppId"];
        var saved = keys.ToDictionary(key => key, Environment.GetEnvironmentVariable);
        try {
            foreach (string key in keys) Environment.SetEnvironmentVariable(key, "fixture");
            var info = SteamClientLauncher.CreateStartInfo(executable);
            Check(keys.All(key => !info.Environment.ContainsKey(key)) && keys.All(key => Environment.GetEnvironmentVariable(key) == "fixture"),
                "Steam must not inherit game-only settings or mutate the parent environment");
        } finally { foreach (var item in saved) Environment.SetEnvironmentVariable(item.Key, item.Value); }
        Console.WriteLine("PASS: Steam startup ordering, already-running client, readiness wait, timeout/cancellation and isolated launch environment.");
    }
}
