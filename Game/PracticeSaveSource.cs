using Microsoft.Win32;

namespace LabPlus.Game;

public static class PracticeSaveSource
{
    public const string LiveSlot = "DLC1-AUTOSAVE5";
    public static bool SupportsBuild(string build) => build is "1.0" or "1.1" or "2.0" or "2.1" or "3.0" or "3.1" or "4.0" or "4.1" or "5.0" or "5.1" or "6.0" or "6.1" or "6.2" or "6.3" or "6.4" or "6.66" or "6.66 Rev 1" or "6.66 Rev 1.1" or "6.66 Rev 2" or "6.66 Rev 2.2" or "6.66 Rev 3" or "6.66 Rev 3.2";
    public static int LiveSlotCount(string build) => build switch
    {
        "1.0" or "1.1" or "2.0" or "2.1" => 3,
        "3.0" or "3.1" or "4.0" or "4.1" => 6,
        _ when SupportsBuild(build) => 9,
        _ => 0
    };
    public static bool SupportsSlot(string build, string slot) => Array.IndexOf(PracticeSaveVault.Slots, slot) is var index && index >= 0 && index < LiveSlotCount(build);
    public static IEnumerable<string> AvailableLiveSlots(string build) => PracticeSaveVault.Slots.Take(LiveSlotCount(build));
    public static string ArchiveBuild(string build) => "Live checkpoint copy; " + build;
    public static string? LiveBuild(PracticeSave save) => save.Build.StartsWith("Live checkpoint copy; ", StringComparison.Ordinal) ? save.Build[22..] : null;
    public static bool SupportsLive(PracticeSave save) => SupportsSlot(LiveBuild(save) ?? "", save.SourceSlot) && !save.FromBackupPair && save.Files.Count == 4;
    public static string? RuntimeBuild(int process)
    {
        if (process <= 0) return null;
        try
        {
            using var game = System.Diagnostics.Process.GetProcessById(process);
            return game.MainModule?.FileName is string path ? GameExecutableInfo.Read(path)?.Version : null;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or ArgumentException or InvalidOperationException or System.ComponentModel.Win32Exception) { return null; }
    }
    public static string? Choose(IEnumerable<string> paths, string? runtimeAccount, string? activeAccount)
    {
        var locations = new List<SteamSaveLocation>();
        foreach (string path in paths.Distinct(StringComparer.OrdinalIgnoreCase))
            try { locations.Add(SteamSaveLocation.Parse(path)); } catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
        string? account = runtimeAccount ?? activeAccount;
        var matches = account is null ? locations : locations.Where(p => p.Account == account).ToList();
        return matches.Count == 1 ? matches[0].Remote : null;
    }
    public static string? RuntimeAccount(string directory, int process)
    {
        if (process <= 0) return null;
        try
        {
            string path = Path.Combine(directory, $"practice-live-{process}.tsv"); PracticeSaveVault.NoLinks(path);
            if (!File.Exists(path) || new FileInfo(path).Length > 8192) return null;
            var value = LivePracticeResult.Parse(File.ReadAllText(path));
            using var game = System.Diagnostics.Process.GetProcessById(process);
            long started = new DateTimeOffset(game.StartTime.ToUniversalTime()).ToUnixTimeMilliseconds();
            return value?.Process == process && value.Captured >= started ? value.Account : null;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or ArgumentException or InvalidOperationException or System.ComponentModel.Win32Exception) { return null; }
    }
    public static string? ActiveAccount()
    {
        try
        {
            object? value = Registry.GetValue(@"HKEY_CURRENT_USER\Software\Valve\Steam\ActiveProcess", "ActiveUser", null);
            uint id = value is int number ? unchecked((uint)number) : 0;
            return id == 0 ? null : (76561197960265728UL + id).ToString(System.Globalization.CultureInfo.InvariantCulture);
        }
        catch (Exception ex) when (ex is UnauthorizedAccessException or System.Security.SecurityException or IOException) { return null; }
    }
}
