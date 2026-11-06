using LabPlus.Configuration;
using LabPlus.Game;
using System.Diagnostics;

internal static class FirewallSourceChecks
{
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    internal static void Run(string temporary)
    {
        string exe = Path.Combine(temporary, "firewall", "DOOMEternalx64vk.exe");
        var block = new GameFirewallRule("arbitrary rule name", exe, true, 2, 0, int.MaxValue);
        var state = new GameFirewallState(4, 7, false, true, [block]);
        Check(GameFirewall.Evaluate(exe, state with { Rules=[] }).Message == "Launch blocked: enable firewall rule for the selected DOOM Eternal version(e.g. via DESRU).", "Missing-rule message does not match the user-facing text");
        Check(GameFirewall.Evaluate(exe.ToUpperInvariant(), state).Blocked, "Exact executable case normalization failed");
        foreach (var invalid in new[] { block with { Enabled = false }, block with { Direction = 1 }, block with { Action = 1 },
            block with { Application = Path.Combine(temporary, "other", "DOOMEternalx64vk.exe") }, block with { Profiles = 2 }, block with { Protocol = 6 },
            block with { RemoteAddresses = "1.2.3.4" }, block with { LocalAddresses = "192.168.0.0/24" }, block with { InterfaceTypes = "Lan" },
            block with { Service = "SomeService" }, block with { LimitedInterfaces = true }, block with { RestrictedIdentity = true } })
            Check(!GameFirewall.Evaluate(exe, state with { Rules = [invalid] }).Blocked, "Restricted/ineffective rule accepted: " + invalid);
        foreach (var invalid in new[] { state with { Rules = [] }, state with { ActiveProfiles = 0 }, state with { ActiveProfiles = 8 },
            state with { EnabledProfiles = 3 }, state with { ExcludedInterfaces = true }, state with { LocalPolicyEffective = false }, state with { ServicesRunning = false } })
            Check(!GameFirewall.Evaluate(exe, invalid).Blocked, "Unprotected firewall policy accepted");
        Check(!GameFirewall.Evaluate(exe, state with { ActiveProfiles = 6, Rules = [block with { Profiles = 2 }] }).Blocked, "Second active profile was ignored");
        Check(GameFirewall.Evaluate(exe, state with { ActiveProfiles = 6, Rules = [block with { Profiles = 2 }, block with { Profiles = 4 }] }).Blocked, "Separate profile blocks were ignored");
        Directory.CreateDirectory(Path.GetDirectoryName(exe)!); File.WriteAllBytes(exe, [1, 2, 3]);
        var store = new SettingsStore(Path.Combine(temporary, "firewall-settings"));
        foreach (bool viewer in new[] { false, true })
        {
            bool configured = false, started = false;
            var launcher = new GameLauncher(store, path => { Check(path == exe, "Checked a different launch target"); return new(false, "test-blocked"); }, _ => { started = true; return Process.GetCurrentProcess(); }, (_, _) => Task.CompletedTask);
            try { launcher.Start(new() { GameExecutablePath = exe, CollisionViewerEnabled = viewer }, _ => configured = true); throw new Exception("Unprotected game launch allowed"); }
            catch (InvalidOperationException ex) when (ex.Message == "test-blocked") { }
            Check(!configured && !started && !Directory.Exists(launcher.RuntimeDirectory), "Launch preparation or process start happened before firewall gate");
        }
        string layerFixture = Path.Combine(AppContext.BaseDirectory, "Native", "CollisionViewer", "bin", "labplus-layer.dll");
        Directory.CreateDirectory(Path.GetDirectoryName(layerFixture)!);
        if (!File.Exists(layerFixture)) File.WriteAllBytes(layerFixture, [1, 2, 3]);
        int checks = 0, starts = 0;
        var protectedLauncher = new GameLauncher(store, path => { Check(path == exe, "Rechecked a different target"); checks++; return new(true, "protected"); }, info =>
        { Check(info.FileName == exe && !info.UseShellExecute, "Started a different process from the verified executable"); Check(info.Environment["LABPLUS_LAUNCHED"] == "1" && info.Environment["SteamAppId"] == "782330", "Child launch identity must survive Steam initialization even without the viewer"); Check(info.Environment["VK_INSTANCE_LAYERS"]!.Contains("VK_LAYER_LABPLUS_collision"), "Saves bridge must load with every overlay disabled"); starts++; return Process.GetCurrentProcess(); }, (_, _) => Task.CompletedTask);
        using (protectedLauncher.Start(new() { GameExecutablePath = exe, CollisionViewerEnabled = false })) { }
        Check(checks == 2 && starts == 1, "Protected launch must inspect the rule again immediately before the process starts");
        var steamReady = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        int asyncStarts = 0, asyncChecks = 0;
        var waitingLauncher = new GameLauncher(store, _ => { asyncChecks++; return new(true, "protected"); },
            _ => { asyncStarts++; return Process.GetCurrentProcess(); }, (_, _) => steamReady.Task);
        var waiting = waitingLauncher.StartAsync(new() { GameExecutablePath = exe, CollisionViewerEnabled = false });
        Check(asyncStarts == 0 && asyncChecks == 1 && !waiting.IsCompleted, "Game launch must wait for Steam readiness");
        steamReady.SetResult(); using (waiting.GetAwaiter().GetResult()) { }
        Check(asyncStarts == 1 && asyncChecks == 2, "The final firewall check must follow Steam startup");
        ProcessStartInfo? captured = null;
        var delayedMutation = new GameLauncher(store, _ => new(true, "protected"), _ => throw new Exception("Mutated target started"),
            (_, _) => { captured!.FileName = Path.Combine(temporary, "other", "DOOMEternalx64vk.exe"); return Task.CompletedTask; });
        try { delayedMutation.Start(new() { GameExecutablePath = exe }, info => captured = info); throw new Exception("Target mutation during Steam startup was accepted"); }
        catch (InvalidOperationException ex) when (ex.Message.StartsWith("Launch blocked: the configured process")) { }
        checks = 0;
        var changedLauncher = new GameLauncher(store, _ => ++checks == 1 ? new(true, "initially protected") : new(false, "rule-disabled-during-preparation"), _ =>
        { throw new Exception("Started game after its blocking rule became disabled"); }, (_, _) => Task.CompletedTask);
        try { changedLauncher.Start(new() { GameExecutablePath = exe, CollisionViewerEnabled = false }); throw new Exception("Stale firewall result accepted"); }
        catch (InvalidOperationException ex) when (ex.Message == "rule-disabled-during-preparation") { }
        foreach (Action<ProcessStartInfo> mutate in new Action<ProcessStartInfo>[] { info => info.FileName = Path.Combine(temporary, "other", "DOOMEternalx64vk.exe"), info => info.UseShellExecute = true })
        {
            try { protectedLauncher.Start(new() { GameExecutablePath = exe, CollisionViewerEnabled = false }, mutate); throw new Exception("Configured launcher bypassed the verified direct executable"); }
            catch (InvalidOperationException ex) when (ex.Message.StartsWith("Launch blocked: the configured process")) { }
        }
        Check(starts == 1, "A mutated launch target reached Process.Start");

        string Remote(string account) { string path = Path.Combine(temporary, "sources", "Steam", "userdata", account, "782330", "remote"); Directory.CreateDirectory(path); return path; }
        string one = Remote("10001"), two = Remote("10002");
        string owner = SteamSaveLocation.Parse(one).Account, other = SteamSaveLocation.Parse(two).Account;
        Check(PracticeSaveSource.Choose([one], null, null) == one, "Single source not chosen");
        Check(PracticeSaveSource.Choose([one, two], null, null) is null, "Ambiguous account chosen");
        Check(PracticeSaveSource.Choose([one, two], owner, other) == one, "Runtime owner did not override registry");
        Check(PracticeSaveSource.Choose([one], other, null) is null, "Wrong account fallback chosen");
        Check(PracticeSaveSource.Choose([one, one], null, null) == one, "Duplicate roots made source ambiguous");
        string response = Path.Combine(temporary, $"practice-live-{Environment.ProcessId}.tsv"), token = new('a', 32);
        string Reply(long time) => $"#LABPLUS_LIVE_PRACTICE_V1\t{Environment.ProcessId}\t{token}\t{time}\tarchived\t{owner}\tDLC1-AUTOSAVE5\tgame/dlc/e4m1_rig/e4m1_rig\tcp_01a\tfixture";
        File.WriteAllText(response, Reply(1)); Check(PracticeSaveSource.RuntimeAccount(temporary, Environment.ProcessId) is null, "Stale PID reply selected an owner");
        File.WriteAllText(response, Reply(DateTimeOffset.UtcNow.ToUnixTimeMilliseconds())); Check(PracticeSaveSource.RuntimeAccount(temporary, Environment.ProcessId) == owner, "Current owner response not recognized");
        Console.WriteLine("PASS: firewall path/profile/protocol/address/interface/identity/service/policy checks; viewer on/off launch gates, final rule recheck and target mutation rejection; automatic save source chooses one matching owner and rejects stale PID data.");
    }
}
