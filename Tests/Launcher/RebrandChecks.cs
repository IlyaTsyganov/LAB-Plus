using LabPlus.Configuration;
using LabPlus.Game;
using System.Text.Json;

internal static class RebrandChecks
{
    internal static void Run(string root)
    {
        string source = Path.Combine(root, "previous"), destination = Path.Combine(root, "current");
        Directory.CreateDirectory(Path.Combine(source, "attempts")); Directory.CreateDirectory(Path.Combine(source, "viewer-runtime"));
        File.WriteAllText(Path.Combine(source, "settings.json"), "{\"ViewDistanceMeters\":135,\"CollisionViewerEnabled\":true}");
        File.WriteAllText(Path.Combine(source, "attempts", "sample.json"), "{\"Format\":\"DEVIL_ATTEMPT_V1\",\"Name\":\"Keep my original note\"}");
        File.WriteAllText(Path.Combine(source, "trigger-paths.tsv"), "#DEVIL_TRIGGER_PATHS_V1\nidTrigger\ttrigger/props/\n");
        File.WriteAllBytes(Path.Combine(source, "game.details"), [0, 255, 12, 99]);
        File.WriteAllText(Path.Combine(source, "viewer-runtime", "old.dll"), "must not migrate");
        File.WriteAllText(Path.Combine(source, "live.request"), "must not replay");
        if (!LegacyDataMigration.CopyTo(destination, source)) throw new Exception("Existing data was not imported.");
        if (new SettingsStore(destination).Load().ViewDistanceMeters != 135) throw new Exception("Preferences were lost.");
        using var document = JsonDocument.Parse(File.ReadAllText(Path.Combine(destination, "attempts", "sample.json")));
        if (document.RootElement.GetProperty("Format").GetString() != "LABPLUS_ATTEMPT_V1" || document.RootElement.GetProperty("Name").GetString() != "Keep my original note") throw new Exception("Archive format migration changed user content.");
        if (!File.ReadAllText(Path.Combine(destination, "trigger-paths.tsv")).StartsWith("#LABPLUS_TRIGGER_PATHS_V1")) throw new Exception("Native catalogue header was not migrated.");
        if (!File.ReadAllBytes(Path.Combine(destination, "game.details")).SequenceEqual(new byte[] { 0, 255, 12, 99 })) throw new Exception("Encrypted bytes were altered.");
        if (Directory.Exists(Path.Combine(destination, "viewer-runtime")) || File.Exists(Path.Combine(destination, "live.request"))) throw new Exception("Stale runtime state was imported.");
        if (LegacyDataMigration.CopyTo(destination, source)) throw new Exception("Existing LAB Plus data was overwritten.");
        var settings = new LabPlusSettings();
        if (!LabCommands.Apply(settings, ["labplus_viewer", "1"], 1234, 1) || !settings.CollisionViewerEnabled) throw new Exception("New command prefix does not work.");
        if (LabCommands.Apply(settings, ["lab_viewer", "0"], 1234, 2) || LabCommands.Apply(settings, ["devil_viewer", "0"], 1234, 2)) throw new Exception("Old command namespaces are still active.");
        DebugHudScaleChecks.Run(root);
        Console.WriteLine("PASS: labplus command namespace, non-destructive preference/archive migration, byte-exact containers, no stale runtime requests and idempotence.");
    }
}
