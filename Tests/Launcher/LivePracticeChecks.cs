using System.Text;
using LabPlus.Game;
internal static class LivePracticeChecks
{
    private static void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
    private static void Reject(Action action, string message) { try { action(); } catch (IOException) { return; } throw new Exception(message); }
    public static void Run(string root)
    {
        string directory = Path.Combine(root, "live-practice-tests"), remote = Path.Combine(directory, "Steam", "userdata", "10001", "782330", "remote"); Directory.CreateDirectory(remote);
        var location = SteamSaveLocation.Parse(remote); string token = new('a', 32); long now = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
        string response = $"#LABPLUS_LIVE_PRACTICE_V1\t1234\t{token}\t{now}\tarchived\t{location.Account}\tDLC1-AUTOSAVE5\tgame/dlc/e4m1_rig/e4m1_rig\tcp1\tCopied\n";
        var capture = LivePracticeResult.Parse(response) ?? throw new Exception("Valid live response rejected");
        Check(LivePracticeResult.Parse(response.Replace("DLC1-AUTOSAVE5", "GAME-AUTOSAVE0")) is null, "Main slot accepted by prototype");
        Check(LivePracticeResult.Parse(response.Replace(token, "../escape")) is null, "Capture path traversal accepted");
        Check(LivePracticeResult.Parse(response + "another row\n") is null, "Extra response data accepted");
        Reject(() => LivePracticeBridge.RequestText(1234, token, now, "archive", location, "DLC1-AUTOSAVE5", false), "Cloud acknowledgment missing");
        Reject(() => LivePracticeBridge.RequestText(1234, token, now, "restore", location, "DLC1-AUTOSAVE5", true), "Unimplemented live replacement exposed");
        Check(LivePracticeBridge.RequestText(1234, token, now, "archive", location, "GAME-AUTOSAVE1", true).Contains("\tGAME-AUTOSAVE1\t"), "Main campaign request lost slot");
        Reject(() => LivePracticeBridge.RequestText(1234, token, now, "archive", location, "../escape", true), "Unsafe live slot accepted");
        string request = LivePracticeBridge.RequestText(1234, token, now, "reload", location, "DLC1-AUTOSAVE5", true); Check(request.Contains("\treload\t"), "Reload request malformed");
        string copy = capture.CaptureFolder(directory); Directory.CreateDirectory(copy);
        foreach (string name in PracticeSaveVault.RequiredFiles) File.WriteAllBytes(Path.Combine(copy, name), SteamSaveContainer.Seal(Encoding.UTF8.GetBytes("live fixture " + name), location.Account, name));
        var vault = new PracticeSaveVault(directory, () => ["DOOMEternalx64vk", "steam"]);
        var saved = vault.ImportLiveCheckpoint(directory, capture, remote, "Live fixture", "Training"); Check(vault.Verify(saved.Id).Files.Count == 4, "Live archive did not verify");
        string target = location.SlotPath("DLC1-AUTOSAVE5"); Directory.CreateDirectory(target);
        foreach (string name in PracticeSaveVault.RequiredFiles) File.Copy(Path.Combine(copy, name), Path.Combine(target, name));
        Reject(() => vault.PlanLiveRestore(1234, saved.Id, remote, "DLC1-AUTOSAVE5", false), "Live restore missing Cloud acknowledgment");
        string alternate = location.SlotPath("DLC1-AUTOSAVE4"); Directory.CreateDirectory(alternate);
        foreach (string name in PracticeSaveVault.RequiredFiles) File.Copy(Path.Combine(copy, name), Path.Combine(alternate, name));
        Check(vault.PlanLiveRestore(1234, saved.Id, remote, "DLC1-AUTOSAVE4", true).Slot == "DLC1-AUTOSAVE4", "Same-campaign restore to a different slot blocked");
        var plan = vault.PlanLiveRestore(1234, saved.Id, remote, "DLC1-AUTOSAVE5", true);
        var targetBefore = Directory.GetFiles(target).ToDictionary(p => Path.GetFileName(p)!, File.ReadAllBytes);
        vault.StageLiveRestore(directory, plan);
        Check(File.Exists(Path.Combine(plan.Bundle(directory), "plan.tsv")) && Directory.GetFiles(Path.Combine(plan.Bundle(directory), "incoming")).Length == 4, "Live staging incomplete");
        Check(File.ReadAllLines(Path.Combine(plan.Bundle(directory), "plan.tsv")).Length == 9, "Native restore plan must bind all four target AND incoming files");
        Check(targetBefore.All(f => File.ReadAllBytes(Path.Combine(target, f.Key!)).SequenceEqual(f.Value)), "Staging modified live slot");
        Reject(() => vault.StageLiveRestore(directory, plan with { Token = "../escape" }), "Live restore staging path escaped");
        var changedPlan = vault.PlanLiveRestore(1234, saved.Id, remote, "DLC1-AUTOSAVE5", true);
        string targetFile = Path.Combine(target, "game.details"); File.WriteAllBytes(targetFile, [1,2,3]);
        Reject(() => vault.StageLiveRestore(directory, changedPlan), "Changed live target accepted after review"); File.WriteAllBytes(targetFile, targetBefore["game.details"]);
        var badFiles = plan.TargetFiles.ToArray(); badFiles[0] = badFiles[0] with { Sha256 = new('0', 64) };
        Reject(() => vault.StageLiveRestore(directory, plan with { TargetFiles = badFiles }), "Forged target fingerprints accepted");
        string archiveManifest = Path.Combine(directory, "practice-saves", saved.Id, "manifest.json"); string originalManifest = File.ReadAllText(archiveManifest);
        File.WriteAllText(archiveManifest, System.Text.Json.JsonSerializer.Serialize(saved with { Build = "unsupported build" }));
        Reject(() => vault.PlanLiveRestore(1234, saved.Id, remote, "DLC1-AUTOSAVE5", true), "Unsupported live archive build accepted"); File.WriteAllText(archiveManifest, originalManifest);
        Reject(() => vault.Capture(remote, "DLC1-AUTOSAVE5", "x", "y"), "Live import relaxed offline capture guard");
        Reject(() => vault.PlanRestore(saved.Id, remote, "DLC1-AUTOSAVE5", true), "Live import relaxed restore guard");
        Reject(() => vault.ImportLiveCheckpoint(directory, capture with { Captured = now - 100000 }, remote, "x", "y"), "Stale live copy accepted");
        Reject(() => vault.ImportLiveCheckpoint(directory, capture with { Account = "76561197960275730" }, remote, "x", "y"), "Foreign owner accepted");
        string file = Path.Combine(copy, "game.details"); byte[] changed = File.ReadAllBytes(file); changed[^1] ^= 1; File.WriteAllBytes(file, changed);
        Reject(() => vault.ImportLiveCheckpoint(directory, capture, remote, "x", "y"), "Tampered live checkpoint archived");
        Check(vault.List().Count == 1, "Rejected import left an archive");
        foreach (var pair in targetBefore) File.WriteAllBytes(Path.Combine(copy, pair.Key), pair.Value);
        string v2 = response.Replace("#LABPLUS_LIVE_PRACTICE_V1", "#LABPLUS_LIVE_PRACTICE_V2").Replace("DLC1-AUTOSAVE5", "GAME-AUTOSAVE1").TrimEnd('\n') + "\t1.1\n";
        var main = LivePracticeResult.Parse(v2) ?? throw new Exception("1.1 main campaign V2 response rejected");
        Check(main.Build == "1.1" && main.Slot == "GAME-AUTOSAVE1", "V2 lost patch/slot identity");
        Check(LivePracticeResult.Parse(v2.Replace("GAME-AUTOSAVE1", "../escape")) is null, "V2 accepted unsafe slot");
        string mainTarget = location.SlotPath(main.Slot); Directory.CreateDirectory(mainTarget);
        foreach (var pair in targetBefore) File.WriteAllBytes(Path.Combine(mainTarget, pair.Key), pair.Value);
        var mainSave = vault.ImportLiveCheckpoint(directory, main, remote, "1.1 main", "Training");
        Check(PracticeSaveSource.SupportsLive(mainSave) && PracticeSaveSource.LiveBuild(mainSave) == "1.1", "1.1 archive patch identity lost");
        var mainPlan = vault.PlanLiveRestore(1234, mainSave.Id, remote, main.Slot, true);
        Check(mainPlan.Build == "1.1" && mainPlan.Slot == main.Slot, "Restore plan lost patch/slot");
        vault.StageLiveRestore(directory, mainPlan);
        string header = File.ReadLines(Path.Combine(mainPlan.Bundle(directory), "plan.tsv")).First();
        Check(header.Split('\t')[2] == main.Slot && header.EndsWith("\t1.1"), "Native plan did not bind main slot and patch");
        Reject(() => vault.StageLiveRestore(directory, mainPlan with { Build = "6.66 Rev 3" }), "Restore patch changed after review");
        Reject(() => vault.ImportLiveCheckpoint(directory, main with { Build = "7.0" }, remote, "unsupported", "Training"), "Unaudited patch imported as live archive");
        // Exercise the production asynchronous mailbox: a process selection lost
        // after installation must not return success or discard recovery details.
        int currentProcess = Environment.ProcessId; bool selected = true;
        var interrupted = mainPlan with { Process = currentProcess, Token = Guid.NewGuid().ToString("N") };
        File.WriteAllText(Path.Combine(directory, $"practice-live-{currentProcess}.tsv"),
            $"#LABPLUS_LIVE_PRACTICE_V2\t{currentProcess}\t{interrupted.Token}\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\tloading\t{location.Account}\t{main.Slot}\t{main.Map}\t{main.Checkpoint}\tFiles installed\t{main.Build}\n");
        try
        {
            LivePracticeBridge.ExecuteRestore(directory, () => selected ? currentProcess : 0, interrupted, true, _ => selected = false).GetAwaiter().GetResult();
            throw new Exception("Lost process selection reported successful restore");
        }
        catch (IOException ex)
        {
            Check(ex.Message.Contains("Selected game process changed") && ex.Message.Contains(interrupted.Original), "Interrupted restore lost its original checkpoint recovery path");
        }
        using (var child = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo("cmd.exe", "/d /c exit 0") { CreateNoWindow = true, UseShellExecute = false })!)
        {
            int exitedProcess = child.Id; child.WaitForExit();
            var closed = mainPlan with { Process = exitedProcess, Token = Guid.NewGuid().ToString("N") };
            try
            {
                LivePracticeBridge.ExecuteRestore(directory, () => exitedProcess, closed, true).GetAwaiter().GetResult();
                throw new Exception("Exited process reported successful restore");
            }
            catch (IOException ex)
            {
                Check(ex.Message.Contains("Game closed during live operation") && ex.Message.Contains(closed.Original), "Game exit lost restore recovery information");
            }
        }
        Console.WriteLine("PASS: live checkpoint protocol scope, Cloud acknowledgment, authenticated copy import while game/Steam run, stale/foreign/tampered copy rejection; offline restore guards unchanged.");
    }
}
