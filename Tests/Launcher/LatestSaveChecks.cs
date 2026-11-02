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
        Reject(() => vault.LastLiveArchive(remote, "GAME-AUTOSAVE0", "1.1"), "Empty library did not fail safely");
        var first = vault.Capture(remote, "GAME-AUTOSAVE0", "First", "Category A", build: PracticeSaveSource.ArchiveBuild("1.1"));
        File.Delete(Path.Combine(root, "practice-latest.json"));
        Check(vault.LastLiveArchive(remote, "GAME-AUTOSAVE2", "1.1").Id == first.Id, "Legacy migration or cross-slot latest lookup failed");
        var second = vault.Capture(remote, "GAME-AUTOSAVE0", "Second", "Category B", build: PracticeSaveSource.ArchiveBuild("1.1"));
        vault.Capture(remote, "GAME-AUTOSAVE0", "Other patch", "Category B", build: PracticeSaveSource.ArchiveBuild("2.0"));
        Check(vault.LastLiveArchive(remote, "GAME-AUTOSAVE2", "1.1").Id == second.Id, "Another patch or category overrode the correct latest save");
        Reject(() => vault.LastLiveArchive(remote, "DLC1-AUTOSAVE3", "3.0"), "Command mixed campaign families");
        string otherRemote = Path.Combine(root, "Steam", "userdata", "10002", "782330", "remote");
        Directory.CreateDirectory(otherRemote);
        Reject(() => vault.LastLiveArchive(otherRemote, "GAME-AUTOSAVE0", "1.1"), "Command mixed accounts");
        string file = Path.Combine(root, "practice-saves", second.Id, "slot", PracticeSaveVault.RequiredFiles[0]);
        byte[] clean = File.ReadAllBytes(file), damaged = clean.ToArray(); damaged[^1] ^= 1; File.WriteAllBytes(file, damaged);
        Reject(() => vault.LastLiveArchive(remote, "GAME-AUTOSAVE0", "1.1"), "Damaged latest archive fell back to an older save"); File.WriteAllBytes(file, clean);
        var racing = new PracticeSaveVault(root, () => [], step => { if (step == "latest_selected") vault.DeleteArchives([second.Id]); });
        Reject(() => racing.LastLiveArchive(remote, "GAME-AUTOSAVE0", "1.1"), "Deletion between selection and verification was not caught");
        Reject(() => new PracticeSaveVault(root, () => []).LastLiveArchive(remote, "GAME-AUTOSAVE0", "1.1"), "Deleted latest archive was forgotten after reopening");
        Check(vault.Verify(first.Id).Id == first.Id, "Older archive was removed during the missing-latest check");
        var third = vault.Capture(remote, "GAME-AUTOSAVE0", "Third", "Category A", build: PracticeSaveSource.ArchiveBuild("1.1"));
        Check(vault.LastLiveArchive(remote, "GAME-AUTOSAVE2", "1.1").Id == third.Id, "New archive did not replace the deleted last identity");
        foreach (var entry in originals) Check(File.ReadAllBytes(Path.Combine(location.SlotPath("GAME-AUTOSAVE0"), entry.Key)).SequenceEqual(entry.Value), "Latest lookup changed game files");
        string token = new string('a', 32);
        Check(PracticeSaveCommand.TryParse([PracticeSaveCommand.Name, token], out var parsed) && parsed == token, "Internal command token rejected");
        foreach (var args in new[] { new[] { PracticeSaveCommand.Name }, new[] { PracticeSaveCommand.Name, "../bad" }, new[] { PracticeSaveCommand.Name, token, "extra" } })
            Check(!PracticeSaveCommand.TryParse(args, out _), "Malformed command reached the loader");
        Check(PracticeSaveCommand.Reply(root, 1234, token, false, "Deleted\nnot loaded\t100%"), "Console reply failed");
        string[] fields = File.ReadAllText(Path.Combine(root, "practice-command-1234.tsv")).TrimEnd().Split('\t');
        Check(fields.Length == 5 && fields[2] == token && fields[3] == "error" && fields[4] == "Deleted not loaded 100%", "Reply can inject controls or lost request identity");
        Console.WriteLine("PASS: latest-save persistence/migration, account/patch/campaign and cross-slot scope, deletion race/tamper rejection without fallback, unchanged game files and console replies.");
    }
}
