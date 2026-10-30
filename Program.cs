namespace LabPlus;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        System.Globalization.CultureInfo.DefaultThreadCurrentCulture = System.Globalization.CultureInfo.GetCultureInfo("en-US");
        System.Globalization.CultureInfo.DefaultThreadCurrentUICulture = System.Globalization.CultureInfo.GetCultureInfo("en-US");
        if (args.Contains("--preview", StringComparer.Ordinal))
        {
            Environment.SetEnvironmentVariable("LABPLUS_PREVIEW", "1");
            Environment.SetEnvironmentVariable("LABPLUS_DATA_DIRECTORY", Path.Combine(Path.GetTempPath(), "labplus-preview-" + Guid.NewGuid().ToString("N")));
        }
        ApplicationConfiguration.Initialize();
        LabPlusTheme.Initialize();
        using var form = new MainForm();
        using var commands = Environment.GetEnvironmentVariable("LABPLUS_PREVIEW") == "1" ? null : new Game.LabCommandServer(form.ReceiveLabCommand);
        Application.Run(new WorkspaceApplicationContext(form));
        form.StopResearchOnExit();
    }
}
