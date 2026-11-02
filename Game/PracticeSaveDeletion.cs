using System.Text.Json;

namespace LabPlus.Game;

public sealed partial class PracticeSaveVault
{
    // Only direct token-named LAB Plus archives may be removed. Game/Steam
    // locations are never deletion targets. Avoid recursive traversal/deletion.
    public void DeleteArchives(IReadOnlyCollection<string> ids)
    {
        if (ids.Count is < 1 or > 10000 || ids.Distinct().Count() != ids.Count) throw new IOException("Select distinct checkpoint archives to delete.");
        var root = Path.GetFullPath(_root);
        var archives = ids.Select(id =>
        {
            string path = Path.GetFullPath(ArchivePath(id));
            if (!string.Equals(Path.GetDirectoryName(path), root, StringComparison.OrdinalIgnoreCase)) throw new IOException("Archive deletion escaped the library.");
            string manifest = Path.Combine(path, "manifest.json"); NoLinks(manifest);
            if (new FileInfo(manifest).Length > 131072) throw new IOException("Archive manifest is too large.");
            PracticeSave? save;
            try { save = JsonSerializer.Deserialize<PracticeSave>(File.ReadAllText(manifest)); }
            catch (JsonException ex) { throw new IOException("Invalid archive manifest.", ex); }
            Validate(save); if (save!.Id != id) throw new IOException("Archive identifier mismatch.");
            var folders = Directory.GetDirectories(path);
            string slot = Path.Combine(path, "slot"); NoLinks(slot);
            if (folders.Length != 1 || folders[0] != slot || Directory.GetFiles(path).Length != 1 || Directory.GetDirectories(slot).Length != 0) throw new IOException("Unexpected archive contents; nothing deleted.");
            var names = Directory.GetFiles(slot).Select(p => { NoLinks(p); return Path.GetFileName(p); }).ToArray();
            if (!names.Order().SequenceEqual(save.Files.Select(f => f.Name).Order())) throw new IOException("Archive file list changed; nothing deleted.");
            return (Id: id, Path: path, Files: names);
        }).ToArray();
        string staging = Path.Combine(root, ".delete-" + Guid.NewGuid().ToString("N")); NoLinks(staging);
        Directory.CreateDirectory(staging); var moved = new List<(string From, string To)>();
        try
        {
            foreach (var archive in archives)
            {
                string to = Path.Combine(staging, archive.Id); Directory.Move(archive.Path, to); moved.Add((archive.Path, to)); _step?.Invoke("delete_moved");
            }
        }
        catch
        {
            foreach (var item in moved.AsEnumerable().Reverse()) Directory.Move(item.To, item.From);
            Directory.Delete(staging); throw;
        }
        try
        {
            foreach (var archive in archives)
            {
                string folder = Path.Combine(staging, archive.Id), slot = Path.Combine(folder, "slot");
                NoLinks(folder); NoLinks(slot);
                foreach (string name in archive.Files) { string file = Path.Combine(slot, name); NoLinks(file); File.Delete(file); }
                Directory.Delete(slot); string manifest = Path.Combine(folder, "manifest.json"); NoLinks(manifest); File.Delete(manifest); Directory.Delete(folder);
            }
            Directory.Delete(staging);
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        { throw new IOException("The selected archives were removed from the library, but cleanup is incomplete. Remaining files: " + staging, ex); }
    }
}
