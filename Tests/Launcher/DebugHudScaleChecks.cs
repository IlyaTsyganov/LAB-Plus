using System.Globalization;
using LabPlus.Configuration;
using LabPlus.Game;

internal static class DebugHudScaleChecks
{
    private static void Check(bool value, string detail) { if (!value) throw new Exception(detail); }
    internal static void Run(string root)
    {
        var store = new SettingsStore(Path.Combine(root, "debughud-scale"));
        Check(store.Load().DebugHudScale == 1, "Missing debug HUD scale does not default to 1");
        CultureInfo savedCulture = CultureInfo.CurrentCulture;
        try
        {
            CultureInfo.CurrentCulture = CultureInfo.GetCultureInfo("ru-RU");
            var settings = new LabPlusSettings();
            foreach (var (text, expected) in new[] { ("1", 1d), ("1.5", 1.5), ("0.5", 0.5), ("3", 3d) })
            {
                Check(LabCommands.Apply(settings, ["labplus_debughud_scale", text], 0, 0) && settings.DebugHudScale == expected,
                    "Valid fractional debug HUD scale rejected: " + text);
                store.Save(settings);
                Check(store.Load().DebugHudScale == expected, "Debug HUD scale did not survive reload: " + text);
                Check(File.ReadAllLines(store.ViewerSettingsPath).Contains("DebugHudScale=" + expected.ToString("R", CultureInfo.InvariantCulture)),
                    "Debug HUD scale INI is absent or culture-dependent: " + text);
            }
            settings.DebugHudScale = 1.5;
            foreach (string[] args in new[]
            {
                new[] { "labplus_debughud_scale" }, new[] { "labplus_debughud_scale", "1", "extra" },
                new[] { "labplus_debughud_scale", "NaN" }, new[] { "labplus_debughud_scale", "Infinity" },
                new[] { "labplus_debughud_scale", "-Infinity" }, new[] { "labplus_debughud_scale", "1e999" },
                new[] { "labplus_debughud_scale", "0.499" }, new[] { "labplus_debughud_scale", "3.001" },
                new[] { "labplus_debughud_scale", "-1" }, new[] { "labplus_debughud_scale", "1,5" },
                new[] { "labplus_debughud_scale", "" }
            })
                Check(!LabCommands.Apply(settings, args, 0, 0) && settings.DebugHudScale == 1.5, "Invalid scale changed prior preference: " + string.Join(' ', args));
            foreach (string raw in new[] { "-1", "0.49", "3.01", "\"NaN\"", "\"Infinity\"", "\"-Infinity\"" })
            {
                File.WriteAllText(store.SettingsPath, "{\"DebugHudScale\":" + raw + ",\"ViewDistanceMeters\":135}");
                var loaded = store.Load();
                Check(loaded.DebugHudScale == 1 && loaded.ViewDistanceMeters == 135, "Invalid persisted scale did not normalize independently: " + raw);
            }
            foreach (double invalid in new[] { double.NaN, double.PositiveInfinity, double.NegativeInfinity, 0.49, 3.01 })
            {
                settings.DebugHudScale = invalid; store.Save(settings);
                Check(settings.DebugHudScale == 1 && store.Load().DebugHudScale == 1 && File.ReadAllLines(store.ViewerSettingsPath).Contains("DebugHudScale=1"),
                    "Invalid in-memory scale was not normalized before publishing");
            }
        }
        finally { CultureInfo.CurrentCulture = savedCulture; }
        Console.WriteLine("PASS: fractional debug HUD scale command, 0.5..3 bounds, invariant INI, persistence and independent invalid-value normalization.");
    }
}
