using System.Text;
using LabPlus.Game;

internal static class CampaignSaveChecks
{
    internal static void Run(string parent)
    {
        string nativeProfiles = File.ReadAllText(Path.Combine(AppContext.BaseDirectory, "Fixtures", "practice_profile.h"));
        var nativeBuilds = System.Text.RegularExpressions.Regex.Matches(nativeProfiles, "static const PracticeProfile \\w+\\{\\s*\"([^\"]+)\"").Select(m => m.Groups[1].Value).ToArray();
        if (nativeBuilds.Length != 22 || nativeBuilds.Any(b => !PracticeSaveSource.SupportsBuild(b))) throw new Exception("Managed/native save patch matrices differ");
        foreach (string build in new[] { "1.0", "1.1", "2.0", "2.1" })
            if (PracticeSaveSource.AvailableLiveSlots(build).Count() != 3 || PracticeSaveSource.SupportsSlot(build, "DLC1-AUTOSAVE3")) throw new Exception("Pre-3.0 exposed DLC slots");
        foreach (string build in new[] { "3.0", "3.1", "4.0", "4.1" })
            if (PracticeSaveSource.AvailableLiveSlots(build).Count() != 6 || PracticeSaveSource.SupportsSlot(build, "DLC2-AUTOSAVE6")) throw new Exception("Pre-5.0 exposed TAG2 slots");
        foreach (string build in new[] { "5.0", "5.1", "6.66 Rev 3.2" })
            if (PracticeSaveSource.AvailableLiveSlots(build).Count() != 9) throw new Exception("TAG2 slots missing from 5.0+");
        if (PracticeSaveSource.LiveSlotCount("unknown") != 0) throw new Exception("Unknown patch exposed live slots");
        string root = Path.Combine(parent, "campaign-save-matrix");
        string remote = Path.Combine(root, "Steam", "userdata", "10001", "782330", "remote");
        Directory.CreateDirectory(remote);
        var location = SteamSaveLocation.Parse(remote);
        var vault = new PracticeSaveVault(root, () => []);
        var baseline = new Dictionary<string, Dictionary<string, byte[]>>();
        foreach (string slot in PracticeSaveVault.Slots)
        {
            var files = PracticeSaveVault.RequiredFiles.ToDictionary(n => n, n => SteamSaveContainer.Seal(Encoding.UTF8.GetBytes(slot + ":old:" + n), location.Account, n));
            baseline.Add(slot, files); Write(location.SlotPath(slot), files);
        }
        Directory.CreateDirectory(Path.Combine(remote, "PROFILE"));
        File.WriteAllText(Path.Combine(remote, "PROFILE", "profile.bin"), "profile sentinel");
        File.WriteAllText(Path.Combine(location.AppRoot, "remotecache.vdf"), "metadata sentinel");
        foreach (string slot in PracticeSaveVault.Slots)
        {
            var save = vault.Capture(remote, slot, "Checkpoint " + slot, "Campaign matrix");
            var newer = PracticeSaveVault.RequiredFiles.ToDictionary(n => n, n => SteamSaveContainer.Seal(Encoding.UTF8.GetBytes(slot + ":new:" + n), location.Account, n));
            Write(location.SlotPath(slot), newer);
            var plan = vault.PlanRestore(save.Id, remote, slot, true);
            var restored = vault.Restore(plan);
            Equal(location.SlotPath(slot), baseline[slot]); Equal(restored.OriginalSlot, newer);
            foreach (string other in PracticeSaveVault.Slots.Where(s => s != slot)) Equal(location.SlotPath(other), baseline[other]);
            if (File.ReadAllText(Path.Combine(remote, "PROFILE", "profile.bin")) != "profile sentinel" || File.ReadAllText(Path.Combine(location.AppRoot, "remotecache.vdf")) != "metadata sentinel") throw new Exception("Campaign restore changed profile or Steam metadata");
        }
        if (vault.List().Count != 9) throw new Exception("Campaign matrix lost archives");
        Console.WriteLine("PASS: nine campaign slots: full authenticated capture/restore, byte-exact primary/backup return, retained originals, other slots/PROFILE/Steam metadata preserved. Synthetic containers; no gameplay compatibility claim.");
    }
    private static void Write(string path, Dictionary<string, byte[]> files)
    {
        Directory.CreateDirectory(path); foreach (var pair in files) File.WriteAllBytes(Path.Combine(path, pair.Key), pair.Value);
    }
    private static void Equal(string path, Dictionary<string, byte[]> files)
    {
        if (Directory.GetFiles(path).Length != files.Count || files.Any(p => !File.ReadAllBytes(Path.Combine(path, p.Key)).SequenceEqual(p.Value))) throw new Exception("Checkpoint bytes differ: " + path);
    }
}
