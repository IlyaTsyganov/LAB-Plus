using LabPlus.Game;
using System.Text;

internal static class LatestSaveChecks
{
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    private static void Reject(Action action, string message) { try { action(); } catch (IOException) { return; } throw new Exception(message); }
    internal static void Run(string parent)
    {
        string root = Path.Combine(parent, "last-save"), remote = Path.Combine(root, "Steam", "userdata", "10001", "782330", "remote");
        Directory.CreateDirectory(remote);
        var location = SteamSaveLocation.Parse(remote); Directory.CreateDirectory(location.SlotPath("GAME-AUTOSAVE0"));
        var originals = PracticeSaveVault.RequiredFiles.ToDictionary(n => n, n => SteamSaveContainer.Seal(Encoding.UTF8.GetBytes("last-save:" + n), location.Account, n));
        foreach (var entry in originals) File.WriteAllBytes(Path.Combine(location.SlotPath("GAME-AUTOSAVE0"), entry.Key), entry.Value);
        var vault = new PracticeSaveVault(root, () => []);
        var history = new PracticeLatestSave(root);
        string Last(string slot = "GAME-AUTOSAVE0", string build = "1.1") => vault.LastLoadedLiveArchiveId(remote, slot, build);
        (LiveRestorePlan Plan, PracticeSave Save) PrepareLast() => vault.PrepareLiveRestore(root, 1234, Last(), remote, "GAME-AUTOSAVE0", "1.1");
        Reject(() => Last(), "Empty history did not fail safely");
        var first = vault.Capture(remote, "GAME-AUTOSAVE0", "First", "Category A", build: PracticeSaveSource.ArchiveBuild("1.1"));
        var second = vault.Capture(remote, "GAME-AUTOSAVE0", "Second", "Category B", build: PracticeSaveSource.ArchiveBuild("1.1"));
        vault.Capture(remote, "GAME-AUTOSAVE0", "Other patch", "Category B", build: PracticeSaveSource.ArchiveBuild("2.0"));
        Reject(() => Last(), "A capture became the last loaded checkpoint");
        var plan = vault.PlanLiveRestore(1234, first.Id, remote, "GAME-AUTOSAVE0", true);
        var loaded = new LivePracticeResult(1234, plan.Token, DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(), "reloaded", plan.Account, plan.Slot, "", "", "Completed.", plan.Build);
        history.Remember(plan, loaded with { Phase = "loading" });
        history.Remember(plan, loaded with { Token = new string('b', 32) });
        Reject(() => Last(), "Pending or mismatched completion updated last-loaded history");
        history.Remember(plan, loaded);
        Check(Last("GAME-AUTOSAVE2") == first.Id, "Last loaded identity did not supersede the newer unplayed archive");
        Check(new PracticeSaveVault(root, () => []).LastLoadedLiveArchiveId(remote, "GAME-AUTOSAVE2", "1.1") == first.Id, "Confirmed identity did not persist");
        Reject(() => Last("GAME-AUTOSAVE0", "2.0"), "Command mixed patches");
        Reject(() => Last("DLC1-AUTOSAVE3", "3.0"), "Command mixed campaigns");
        string otherRemote = Path.Combine(root, "Steam", "userdata", "10002", "782330", "remote");
        Directory.CreateDirectory(otherRemote);
        Reject(() => vault.LastLoadedLiveArchiveId(otherRemote, "GAME-AUTOSAVE0", "1.1"), "Command mixed accounts");
        Check(PrepareLast().Save.Id == first.Id, "Remembered checkpoint was not prepared");
        string file = Path.Combine(root, "practice-saves", first.Id, "slot", PracticeSaveVault.RequiredFiles[0]);
        byte[] clean = File.ReadAllBytes(file), damaged = clean.ToArray();
        DateTime stamp = File.GetLastWriteTimeUtc(file);
        damaged[^1] ^= 1; File.WriteAllBytes(file, damaged); File.SetLastWriteTimeUtc(file, stamp);
        Reject(() => PrepareLast(), "Cached verification accepted altered bytes with unchanged size and timestamp");
        File.WriteAllBytes(file, clean);
        vault.DeleteArchives([first.Id]);
        Reject(() => PrepareLast(), "Deleted last-loaded archive fell back to a different checkpoint");
        Check(Last() == first.Id && vault.Verify(second.Id).Id == second.Id, "Deletion forgot the loaded identity or touched another archive");
        var secondPlan = vault.PlanLiveRestore(1234, second.Id, remote, "GAME-AUTOSAVE0", true);
        history.Remember(secondPlan, loaded with { Token = secondPlan.Token, Captured = loaded.Captured + 1 });
        Check(Last("GAME-AUTOSAVE2") == second.Id, "A new completed load did not update last-loaded history");
        foreach (var entry in originals) Check(File.ReadAllBytes(Path.Combine(location.SlotPath("GAME-AUTOSAVE0"), entry.Key)).SequenceEqual(entry.Value), "Latest lookup changed game files");
        string token = new string('a', 32);
        Check(PracticeSaveCommand.TryParse([PracticeSaveCommand.Name, token], out var parsed) && parsed == token, "Internal command token rejected");
        foreach (var args in new[] { new[] { PracticeSaveCommand.Name }, new[] { PracticeSaveCommand.Name, "../bad" }, new[] { PracticeSaveCommand.Name, token, "extra" } })
            Check(!PracticeSaveCommand.TryParse(args, out _), "Malformed command reached the loader");
        Check(PracticeSaveCommand.Acknowledge(root, 1234, token), "Console request was not acknowledged");
        var acknowledged = File.ReadAllText(Path.Combine(root, "practice-command-1234.tsv")).TrimEnd().Split('\t');
        Check(acknowledged.Length == 5 && acknowledged[2] == token && acknowledged[3] == "accepted",
            "Delivery acknowledgement must retain the request token without completing the load");
        Check(PracticeSaveCommand.Reply(root, 1234, token, false, "Deleted\nnot loaded\t100%"), "Console reply failed");
        string[] fields = File.ReadAllText(Path.Combine(root, "practice-command-1234.tsv")).TrimEnd().Split('\t');
        Check(fields.Length == 5 && fields[2] == token && fields[3] == "error" && fields[4] == "Deleted not loaded 100%", "Reply can inject controls or lost request identity");
        Console.WriteLine("PASS: last successfully loaded identity, account/patch/campaign and cross-slot scope, cached tamper rejection and deletion without fallback, unchanged game files and console replies.");
    }
}
