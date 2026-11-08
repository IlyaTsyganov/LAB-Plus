using System.Text.Json;

namespace LabPlus.Game;

// Keep the last confirmed load even after deletion. Capturing an archive
// must never change the destination of the repeat-load console command.
internal sealed class PracticeLatestSave
{
    private sealed record Entry(string Account, string Build, string Campaign, string Id, long Captured);
    private static readonly object Gate = new();
    private readonly string _path;
    internal PracticeLatestSave(string directory) => _path = Path.Combine(directory, "practice-last-loaded.json");
    private List<Entry> Read()
    {
        PracticeSaveVault.NoLinks(_path);
        if (!File.Exists(_path)) return [];
        try
        {
            if (new FileInfo(_path).Length > 2_000_000) throw new IOException("Last-save index is too large.");
            var entries = JsonSerializer.Deserialize<List<Entry>>(File.ReadAllText(_path));
            if (entries is null || entries.Count > 10000 || entries.Any(e => e is null || !ulong.TryParse(e.Account, out var account)
                || account <= 76561197960265728UL || account > 76561197960265728UL + uint.MaxValue
                || !PracticeSaveSource.SupportsBuild(e.Build) || e.Campaign is not ("Main" or "TAG1" or "TAG2")
                || !LevelStateSnapshot.ValidToken(e.Id) || e.Captured is <= 0 or > 253402300799999)
                || entries.GroupBy(e => (e.Account, e.Build, e.Campaign)).Any(g => g.Count() != 1))
                throw new IOException("Last-save index is invalid.");
            return entries;
        }
        catch (JsonException ex) { throw new IOException("Last-save index is unreadable.", ex); }
    }
    internal string? Find(string account, string build, string slot)
    { lock (Gate) return Read().SingleOrDefault(e => e.Account == account && e.Build == build && e.Campaign == PracticeSaveSource.Campaign(slot))?.Id; }
    internal void Remember(LiveRestorePlan plan, LivePracticeResult result)
    {
        if (result.Phase != "reloaded" || result.Token != plan.Token || result.Process != plan.Process
            || result.Account != plan.Account || result.Build != plan.Build || result.Slot != plan.Slot
            || !PracticeSaveSource.SupportsSlot(plan.Build, plan.Slot) || !LevelStateSnapshot.ValidToken(plan.ArchiveId)) return;
        lock (Gate)
        {
            var entries = Read();
            string campaign = PracticeSaveSource.Campaign(plan.Slot);
            var previous = entries.SingleOrDefault(e => e.Account == plan.Account && e.Build == plan.Build && e.Campaign == campaign);
            if (previous is not null && previous.Captured > result.Captured) return;
            if (previous is not null) entries.Remove(previous);
            entries.Add(new(plan.Account, plan.Build, campaign, plan.ArchiveId, result.Captured));
            if (entries.Count > 10000) throw new IOException("Last-save index limit reached.");
            PracticeSaveVault.NoLinks(_path + ".tmp"); Directory.CreateDirectory(Path.GetDirectoryName(_path)!);
            File.WriteAllText(_path + ".tmp", JsonSerializer.Serialize(entries)); File.Move(_path + ".tmp", _path, true);
        }
    }
}

public sealed partial class PracticeSaveVault
{
    internal string LastLoadedLiveArchiveId(string remote, string targetSlot, string build)
    {
        var location = SteamSaveLocation.Parse(remote);
        if (!PracticeSaveSource.SupportsSlot(build, targetSlot)) throw new IOException("Load a supported campaign slot first.");
        var history = new PracticeLatestSave(Path.GetDirectoryName(_root)!);
        return history.Find(location.Account, build, targetSlot)
            ?? throw new IOException("No LAB Plus checkpoint has been loaded in this campaign on the current patch. Load an archive from Saves first.");
    }
}
