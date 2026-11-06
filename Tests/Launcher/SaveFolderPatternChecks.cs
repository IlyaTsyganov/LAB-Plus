using LabPlus.Game;

internal static class SaveFolderPatternChecks
{
    internal static void Run(string root)
    {
        string prefix = Path.Combine(root, "folder-pattern", "custom-location");
        foreach (string id in new[] { "42", "987654", uint.MaxValue.ToString(System.Globalization.CultureInfo.InvariantCulture) })
        {
            string remote = Path.Combine(prefix, "Steam", "userdata", id, "782330", "remote"); Directory.CreateDirectory(remote);
            var parsed = SteamSaveLocation.Parse(remote);
            if (parsed.Account != (76561197960265728UL + uint.Parse(id)).ToString(System.Globalization.CultureInfo.InvariantCulture))
                throw new Exception("Save path must accept any valid userdata ID without a fixed install prefix");
            _ = SteamSaveLocation.Parse(remote.ToUpperInvariant());
        }
        foreach (string suffix in new[] {
            "NotSteam/userdata/42/782330/remote", "Steam/42/782330/remote", "Steam/userdata/42/remote",
            "Steam/userdata/42/782331/remote", "Steam/userdata/42/782330/remote/extra",
            "Steam/userdata/not-a-number/782330/remote", "Steam/userdata/0/782330/remote",
            "Steam/userdata/4294967296/782330/remote"
        })
        {
            string invalid = Path.Combine(prefix, suffix.Replace('/', Path.DirectorySeparatorChar)); Directory.CreateDirectory(invalid);
            try { SteamSaveLocation.Parse(invalid); }
            catch (IOException) { continue; }
            throw new Exception("Invalid Steam save path accepted: " + suffix);
        }
        Console.WriteLine("PASS: Steam save-folder pattern, arbitrary numeric userdata IDs, custom prefixes and case-insensitive folder names.");
    }
}
