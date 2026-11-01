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
        bool preview = Environment.GetEnvironmentVariable("LABPLUS_PREVIEW") == "1";
        using var instance = preview ? null : Game.SingleInstance.TryAcquire();
        if (!preview && (instance is null || Game.SingleInstance.OlderInstanceRunning()))
        {
            MessageBox.Show(Game.SingleInstance.Warning, "LAB Plus", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }
        LabPlusTheme.Initialize();
        using var form = new MainForm();
        using var commands = Environment.GetEnvironmentVariable("LABPLUS_PREVIEW") == "1" ? null : new Game.LabCommandServer(form.ReceiveLabCommand);
        form.FormClosing += (_, e) => { if (!e.Cancel) commands?.Stop(); };
        Application.Run(new WorkspaceApplicationContext(form));
        form.StopResearchOnExit();
    }
}
