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
    private readonly object _liveArchiveGate = new();
    private (string Id, string ManifestHash)? _authenticatedLiveArchive;

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
        return BuildLiveRestorePlan(process, id, location, slot, source.Save, source.ManifestHash);
    }
    internal (LiveRestorePlan Plan, PracticeSave Save) PrepareLiveRestore(string directory, int process, string id, string remote, string slot, string build)
    {
        if (process <= 0 || !PracticeSaveSource.SupportsSlot(build, slot)) throw new IOException("Load a supported campaign slot first.");
        var location = SteamSaveLocation.Parse(remote);
        (PracticeSave Save, Dictionary<string, byte[]> Files, string ManifestHash) source;
        lock (_liveArchiveGate)
        {
            string? previous = _authenticatedLiveArchive is { } cached && cached.Id == id ? cached.ManifestHash : null;
            source = VerifyFolder(ArchivePath(id), previous);
            if (source.Save.Id != id) throw new IOException("Archive identifier mismatch.");
            _authenticatedLiveArchive = (id, source.ManifestHash);
        }
        ValidateLiveArchive(source.Save, location);
        if (PracticeSaveSource.LoadBlockReason(source.Save, build, slot) is string reason) throw new IOException(reason);
        var plan = BuildLiveRestorePlan(process, id, location, slot, source.Save, source.ManifestHash);
        WriteLiveRestoreBundle(directory, plan, location, source.Save, source.Files);
        return (plan, source.Save);
    }
    private static LiveRestorePlan BuildLiveRestorePlan(int process, string id, SteamSaveLocation location, string slot, PracticeSave source, string manifestHash)
    {
        if (source.Id != id) throw new IOException("Archive identifier mismatch.");
        if (Family(source.SourceSlot) != Family(slot)) throw new IOException("Campaign/TAG1/TAG2 slots cannot be mixed.");
        var current = ReadSet(location.SlotPath(slot), allowWriters: true);
        if (current.Count != 4 || !Health(current, location.Account).Authenticated) throw new IOException("All four current checkpoint containers must authenticate before live restore.");
        string token = Guid.NewGuid().ToString("N");
        var files = RequiredFiles.Select(name => new SaveFileEntry(name, current[name].Length, Hash(current[name]))).ToArray();
        string original = Path.Combine(location.AppRoot, ".labplus-live-original-" + token);
        return new(process, token, id, manifestHash, location.Remote, location.Account, Fingerprint(current), files,
            $"Restore {source.Name} ({source.Category}) into {location.SlotPath(slot)}\nSteam account: {location.Account}\nIf the live checkpoint differs, all four primary/backup files will be written through Steam's storage API and the original will be retained at {original}.\nThe files are verified in Steam and on disk before the engine loads them.\nPROFILE and other campaign slots are not replaced. Keep Cloud disabled and continue in the game when loading reaches 100%. This restores a serialized checkpoint, not arbitrary live simulation state.", slot, PracticeSaveSource.LiveBuild(source)!);
    }
    public void StageLiveRestore(string directory, LiveRestorePlan plan)
    {
        var location = SteamSaveLocation.Parse(plan.Remote); var source = VerifyFolder(ArchivePath(plan.ArchiveId)); ValidateLiveArchive(source.Save, location);
        if (!ValidSlot(plan.Slot) || Family(plan.Slot) != Family(source.Save.SourceSlot) || plan.Build != PracticeSaveSource.LiveBuild(source.Save)) throw new IOException("Restore campaign or patch changed after review.");
        var current = ReadSet(location.SlotPath(plan.Slot), allowWriters: true);
        if (current.Count != 4 || !Health(current, location.Account).Authenticated) throw new IOException("Current training checkpoint changed or failed authentication.");
        if (plan.Account != location.Account || source.Save.Id != plan.ArchiveId || source.ManifestHash != plan.ManifestHash || Fingerprint(current) != plan.TargetHash
            || plan.TargetFiles.Count != 4 || RequiredFiles.Where((name, i) => plan.TargetFiles[i].Name != name || plan.TargetFiles[i].Length != current[name].Length || plan.TargetFiles[i].Sha256 != Hash(current[name])).Any())
            throw new IOException("Archive or target changed after review. Create a new live restore plan.");
        WriteLiveRestoreBundle(directory, plan, location, source.Save, source.Files);
    }
    private static void WriteLiveRestoreBundle(string directory, LiveRestorePlan plan, SteamSaveLocation location, PracticeSave source, IReadOnlyDictionary<string, byte[]> files)
    {
        string bundle = plan.Bundle(directory); NoLinks(bundle);
        if (Directory.Exists(bundle) || File.Exists(bundle)) throw new IOException("Live restore staging destination already exists.");
        WriteSet(Path.Combine(bundle, "incoming"), files);
        var text = new StringBuilder($"#LABPLUS_LIVE_RESTORE_PLAN_V2\t{location.Account}\t{plan.Slot}\t{plan.Token}\t{plan.Process}\t{plan.ArchiveId}\t{plan.Build}\n");
        foreach (var file in plan.TargetFiles) text.Append(file.Name).Append('\t').Append(file.Length.ToString(System.Globalization.CultureInfo.InvariantCulture)).Append('\t').Append(file.Sha256).Append('\n');
        foreach (string name in RequiredFiles)
        {
            var file = source.Files.Single(entry => entry.Name == name);
            text.Append(name).Append('\t').Append(file.Length.ToString(System.Globalization.CultureInfo.InvariantCulture)).Append('\t').Append(file.Sha256.ToLowerInvariant()).Append('\n');
        }
        File.WriteAllText(Path.Combine(bundle, "plan.tsv"), text.ToString(), new UTF8Encoding(false));
    }
}
