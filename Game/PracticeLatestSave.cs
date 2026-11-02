using System.Text.Json;

namespace LabPlus.Game;

// Keep the last-created identity even after deletion. A console load must not
// silently select an older checkpoint when its intended archive disappears.
internal sealed class PracticeLatestSave
{
    private sealed record Entry(string Account, string Build, string Campaign, string Id, long Captured);
    private static readonly object Gate = new();
    private readonly string _path;
    internal PracticeLatestSave(string directory) => _path = Path.Combine(directory, "practice-latest.json");
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
    internal void Remember(PracticeSave save)
    {
        if (!PracticeSaveSource.SupportsLive(save)) return;
        lock (Gate)
        {
            var entries = Read(); string build = PracticeSaveSource.LiveBuild(save)!, campaign = PracticeSaveSource.Campaign(save.SourceSlot);
            var previous = entries.SingleOrDefault(e => e.Account == save.Account && e.Build == build && e.Campaign == campaign);
            if (previous is not null && previous.Captured > save.Captured) return;
            if (previous is not null) entries.Remove(previous);
            entries.Add(new(save.Account, build, campaign, save.Id, save.Captured));
            if (entries.Count > 10000) throw new IOException("Last-save index limit reached.");
            PracticeSaveVault.NoLinks(_path + ".tmp"); Directory.CreateDirectory(Path.GetDirectoryName(_path)!);
            File.WriteAllText(_path + ".tmp", JsonSerializer.Serialize(entries)); File.Move(_path + ".tmp", _path, true);
        }
    }
}

public sealed partial class PracticeSaveVault
{
    public PracticeSave LastLiveArchive(string remote, string targetSlot, string build)
    {
        var location = SteamSaveLocation.Parse(remote);
        if (!PracticeSaveSource.SupportsSlot(build, targetSlot)) throw new IOException("Load a supported campaign slot first.");
        var latest = new PracticeLatestSave(Path.GetDirectoryName(_root)!);
        string? id = latest.Find(location.Account, build, targetSlot);
        if (id is null)
        {
            // One-time migration for libraries created before last-save tracking.
            var existing = List().Where(s => s.Account == location.Account && PracticeSaveSource.LoadBlockReason(s, build, targetSlot) is null)
                .OrderByDescending(s => s.Captured).ThenByDescending(s => s.Id, StringComparer.Ordinal).FirstOrDefault();
            if (existing is null) throw new IOException("No LAB Plus save is available for this account, patch and campaign.");
            latest.Remember(existing); id = existing.Id;
        }
        _step?.Invoke("latest_selected");
        try
        {
            var save = Verify(id);
            if (save.Account != location.Account || PracticeSaveSource.LoadBlockReason(save, build, targetSlot) is not null) throw new IOException("Last save no longer matches the running game.");
            return save;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException or System.Security.Cryptography.CryptographicException)
        { throw new IOException("The last LAB Plus save was deleted, is unavailable or is damaged. Create a new save before using this command again.", ex); }
    }
}
