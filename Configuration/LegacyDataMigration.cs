using System.Text.Json;
using System.Text.Json.Nodes;

namespace LabPlus.Configuration;

// Previous identifiers are confined to this read/import compatibility boundary.
// New registrations, files, commands and protocols always use LAB Plus names.
public static class LegacyDataMigration
{
    private const string PreviousProduct = "DEViL";
    public static string PreviousDirectory => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), PreviousProduct);
    public static string PreviousLayerManifest => Path.Combine(PreviousDirectory, "viewer-runtime", "DoomCollisionLayer.json");
    public static string PreviousOverrideManifest => Path.Combine(PreviousDirectory, "viewer-runtime", "DoomCollisionOverride.json");

    public static bool CopyTo(string destination, string? source = null)
    {
        source ??= PreviousDirectory;
        string completed = Path.Combine(destination, ".legacy-import-complete");
        if (!Directory.Exists(source) || File.Exists(completed)) return false;
        var options = new EnumerationOptions { RecurseSubdirectories = true, AttributesToSkip = FileAttributes.ReparsePoint, IgnoreInaccessible = false };
        bool copied = false;
        foreach (string path in Directory.EnumerateFiles(source, "*", options))
        {
            string relative = Path.GetRelativePath(source, path);
            if (relative.Split(Path.DirectorySeparatorChar)[0].Equals("viewer-runtime", StringComparison.OrdinalIgnoreCase)
                || path.EndsWith(".request", StringComparison.OrdinalIgnoreCase) || path.EndsWith(".tmp", StringComparison.OrdinalIgnoreCase)) continue;
            string target = Path.Combine(destination, relative);
            if (File.Exists(target)) continue;
            Directory.CreateDirectory(Path.GetDirectoryName(target)!);
            if (path.EndsWith(".json", StringComparison.OrdinalIgnoreCase) && new FileInfo(path).Length <= 1_048_576)
            {
                string text = File.ReadAllText(path);
                try
                {
                    var node = JsonNode.Parse(text);
                    ConvertFormats(node);
                    text = node?.ToJsonString(new JsonSerializerOptions { WriteIndented = true }) ?? text;
                }
                catch (JsonException) { /* Preserve unreadable input as evidence; normal readers still validate it. */ }
                File.WriteAllText(target, text);
            }
            else if ((path.EndsWith(".txt", StringComparison.OrdinalIgnoreCase) || path.EndsWith(".tsv", StringComparison.OrdinalIgnoreCase)) && new FileInfo(path).Length <= 1_048_576)
            {
                string text = File.ReadAllText(path);
                if (text.StartsWith("#DEVIL_", StringComparison.Ordinal)) text = "#LABPLUS_" + text[7..];
                File.WriteAllText(target, text);
            }
            else File.Copy(path, target, overwrite: false); // Encrypted save containers are copied byte for byte.
            copied = true;
        }
        Directory.CreateDirectory(destination);
        File.WriteAllText(completed, "LAB Plus imported previous local data without modifying the originals.\n");
        return copied;
    }

    private static void ConvertFormats(JsonNode? node)
    {
        if (node is JsonObject obj)
        {
            foreach (string key in obj.Select(p => p.Key).ToArray())
            {
                if (key.Equals("Format", StringComparison.OrdinalIgnoreCase) && obj[key] is JsonValue value
                    && value.TryGetValue<string>(out var format) && format.StartsWith("DEVIL_", StringComparison.Ordinal)) obj[key] = "LABPLUS_" + format[6..];
                else ConvertFormats(obj[key]);
            }
        }
        else if (node is JsonArray array) foreach (var child in array) ConvertFormats(child);
    }

    public static bool OwnsPreviousManifest(string path, bool implicitLayer)
    {
        string expected = implicitLayer ? PreviousOverrideManifest : PreviousLayerManifest;
        if (!Path.GetFullPath(path).Equals(Path.GetFullPath(expected), StringComparison.OrdinalIgnoreCase) || !File.Exists(path)) return false;
        try
        {
            using var document = JsonDocument.Parse(File.ReadAllText(path));
            var layer = document.RootElement.GetProperty("layer");
            return implicitLayer
                ? layer.GetProperty("name").GetString() == "VK_LAYER_LUNARG_override"
                    && layer.GetProperty("component_layers").EnumerateArray().Any(v => v.GetString() == "VK_LAYER_DOOM_collision")
                : layer.GetProperty("name").GetString() == "VK_LAYER_DOOM_collision";
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException or KeyNotFoundException or InvalidOperationException) { return false; }
    }
}
