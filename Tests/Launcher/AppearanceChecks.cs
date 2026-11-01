using LabPlus.Configuration;
internal static class AppearanceChecks
{
    internal static void Run(string directory)
    {
        var store = new SettingsStore(Path.Combine(directory, "appearances"));
        var settings = new LabPlusSettings();
        settings.WorldAppearance.Color = "#123456"; settings.WorldAppearance.FillOpacity = 0;
        settings.TriggerAppearance.Color = "#abcdef"; settings.TriggerAppearance.FillOpacity = 37;
        settings.WallAppearance.Color = "#fedcba"; settings.WallAppearance.FillOpacity = 100;
        settings.InspectorAppearance.Color = "#a1b2c3"; settings.InspectorAppearance.FillOpacity = 64;
        store.Save(settings);
        var saved = store.Load(); string ini = File.ReadAllText(store.ViewerSettingsPath);
        if (saved.WorldAppearance.Color != "#123456" || saved.TriggerAppearance.FillOpacity != 37 || saved.WallAppearance.FillOpacity != 100 || saved.InspectorAppearance.Color != "#a1b2c3") throw new Exception("Appearance settings did not survive reload.");
        foreach (string entry in new[] { "WorldColor=#123456", "WorldFillOpacity=0", "TriggerColor=#abcdef", "TriggerFillOpacity=37", "WallFillOpacity=100", "InspectorColor=#a1b2c3", "InspectorFillOpacity=64" })
            if (!ini.Contains(entry)) throw new Exception("The native renderer did not receive " + entry);
        settings.WorldAppearance.Color = "invalid"; settings.WorldAppearance.FillOpacity = -7;
        settings.InspectorAppearance.FillOpacity = 101; store.Save(settings);
        if (store.Load().WorldAppearance.Color != "#26ccff" || settings.WorldAppearance.FillOpacity != 0 || settings.InspectorAppearance.FillOpacity != 100) throw new Exception("Malformed appearances were not normalized.");
        Console.WriteLine("PASS: independent geometry and inspector colors/opacity persist and reach viewer.ini; malformed preferences are normalized.");
    }
}
