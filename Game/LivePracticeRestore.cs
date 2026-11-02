using System.Text;

namespace LabPlus.Game;

public sealed record LiveRestorePlan(int Process, string Token, string ArchiveId, string ManifestHash, string Remote, string Account,
    string TargetHash, IReadOnlyList<SaveFileEntry> TargetFiles, string Description, string Slot = "DLC1-AUTOSAVE5", string Build = "6.66 Rev 3")
{
    public string Bundle(string directory)
    {
        if (Process <= 0 || Token.Length != 32 || Token.Any(c => !(c is >= '0' and <= '9' or >= 'a' and <= 'f'))) throw new IOException("Invalid live restore scope.");
        return Path.Combine(directory, "live-practice", "restore-pending", Process.ToString(System.Globalization.CultureInfo.InvariantCulture), Token);
    }
    public string Original => Path.Combine(SteamSaveLocation.Parse(Remote).AppRoot, ".labplus-live-original-" + Token);
}
public sealed partial class PracticeSaveVault
{
    private static void ValidateLiveArchive(PracticeSave source, SteamSaveLocation location)
    {
        if (source.Account != location.Account) throw new IOException("Archive belongs to a different Steam account. Live restore is blocked; ownership is never changed.");
        if (!PracticeSaveSource.SupportsLive(source)) throw new IOException("Archive has no supported live patch identity or complete file set.");
    }
    public LiveRestorePlan PlanLiveRestore(int process, string id, string remote, string slot, bool cloudDisabled)
    {
        if (process <= 0) throw new IOException("Select the running game first.");
        if (!cloudDisabled) throw new IOException("Confirm Steam Cloud is disabled before live restore.");
        if (!PracticeSaveVault.ValidSlot(slot)) throw new IOException("Unsupported campaign slot.");
        var location = SteamSaveLocation.Parse(remote); var source = VerifyFolder(ArchivePath(id)); ValidateLiveArchive(source.Save, location);
        if (source.Save.SourceSlot != slot) throw new IOException("Live restore requires the original campaign slot.");
        var current = ReadSet(location.SlotPath(slot), allowWriters: true);
        if (current.Count != 4 || !Health(current, location.Account).Authenticated) throw new IOException("All four current checkpoint containers must authenticate before live restore.");
        string manifestHash = Hash(File.ReadAllBytes(Path.Combine(ArchivePath(id), "manifest.json"))), token = Guid.NewGuid().ToString("N");
        var files = RequiredFiles.Select(name => new SaveFileEntry(name, current[name].Length, Hash(current[name]))).ToArray();
        string original = Path.Combine(location.AppRoot, ".labplus-live-original-" + token);
        return new(process, token, id, manifestHash, location.Remote, location.Account, Fingerprint(current), files,
            $"Restore {source.Save.Name} ({source.Save.Category}) into {location.SlotPath(slot)}\nSteam account: {location.Account}\nAll four primary/backup files will be written through Steam's storage API, verified in both Steam and on disk, then loaded through the engine.\nCurrent four-file checkpoint will be retained at {original}\nPROFILE and other campaign slots are not replaced. Steam updates its own slot-file metadata normally.\nPause first; keep Cloud disabled. At 100% loading, press Space in game and pause again. This restores a serialized checkpoint, not arbitrary live simulation state.", slot, PracticeSaveSource.LiveBuild(source.Save)!);
    }
    public void StageLiveRestore(string directory, LiveRestorePlan plan)
    {
        var location = SteamSaveLocation.Parse(plan.Remote); var source = VerifyFolder(ArchivePath(plan.ArchiveId)); ValidateLiveArchive(source.Save, location);
        if (plan.Slot != source.Save.SourceSlot || plan.Build != PracticeSaveSource.LiveBuild(source.Save)) throw new IOException("Restore slot or patch changed after review.");
        var current = ReadSet(location.SlotPath(plan.Slot), allowWriters: true);
        if (current.Count != 4 || !Health(current, location.Account).Authenticated) throw new IOException("Current training checkpoint changed or failed authentication.");
        if (plan.Account != location.Account || Hash(File.ReadAllBytes(Path.Combine(ArchivePath(plan.ArchiveId), "manifest.json"))) != plan.ManifestHash || Fingerprint(current) != plan.TargetHash
            || plan.TargetFiles.Count != 4 || RequiredFiles.Where((name, i) => plan.TargetFiles[i].Name != name || plan.TargetFiles[i].Length != current[name].Length || plan.TargetFiles[i].Sha256 != Hash(current[name])).Any())
            throw new IOException("Archive or target changed after review. Create a new live restore plan.");
        string bundle = plan.Bundle(directory); NoLinks(bundle);
        if (Directory.Exists(bundle) || File.Exists(bundle)) throw new IOException("Live restore staging destination already exists.");
        WriteSet(Path.Combine(bundle, "incoming"), source.Files);
        var text = new StringBuilder($"#LABPLUS_LIVE_RESTORE_PLAN_V2\t{location.Account}\t{plan.Slot}\t{plan.Token}\t{plan.Process}\t{plan.ArchiveId}\t{plan.Build}\n");
        foreach (var file in plan.TargetFiles) text.Append(file.Name).Append('\t').Append(file.Length.ToString(System.Globalization.CultureInfo.InvariantCulture)).Append('\t').Append(file.Sha256).Append('\n');
        foreach (string name in RequiredFiles) text.Append(name).Append('\t').Append(source.Files[name].Length.ToString(System.Globalization.CultureInfo.InvariantCulture)).Append('\t').Append(Hash(source.Files[name])).Append('\n');
        File.WriteAllText(Path.Combine(bundle, "plan.tsv"), text.ToString(), new UTF8Encoding(false));
    }
}
