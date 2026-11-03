using LabPlus.Updates;

namespace LabPlus;

internal static class StartupUpdates
{
    // Returns true only after a downloaded, validated update helper has started.
    internal static async Task<bool> RunAsync()
    {
        string data = Environment.GetEnvironmentVariable("LABPLUS_DATA_DIRECTORY")
            ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "labplus");
        var preferences = new UpdatePreferences(data);
        using var client = ReleaseUpdates.CreateClient();
        var release = await new ReleaseUpdates(client).CheckAsync(AppVersion.Number, preferences);
        if (release is null) return false;
        using var question = Prompt(release.DisplayVersion);
        var result = question.ShowDialog();
        if (result == DialogResult.Ignore)
        {
            try { preferences.Skip(release.Version); }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException)
            {
                using var warning = new WorkspaceMessageForm("Your choice could not be saved. LAB Plus will continue to open.\n\n" + error.Message, "LAB Plus", MessageBoxButtons.OK);
                warning.ShowDialog();
            }
        }
        if (result != DialogResult.Yes) return false;
        using var download = new UpdateDownloadForm(client, release);
        download.ShowDialog();
        return download.HandedOff;
    }

    internal static WorkspaceMessageForm Prompt(string version) => new(
        $"A new version of LAB Plus is available: {version}.\n\nWould you like to update?", "Update available",
        [("Don't ask again for this version", DialogResult.Ignore), ("No", DialogResult.No), ("Yes", DialogResult.Yes)])
        { ShowInTaskbar = true, StartPosition = FormStartPosition.CenterScreen, Compact = true, ClientSize = new Size(600, 300), MinimumSize = new Size(600, 300) };
}

internal sealed class UpdateDownloadForm : WorkspaceMessageForm
{
    private readonly CancellationTokenSource _cancel = new(TimeSpan.FromMinutes(5));
    internal bool HandedOff { get; private set; }
    internal UpdateDownloadForm(System.Net.Http.HttpClient client, ReleaseUpdate release)
        : base($"Downloading LAB Plus {release.DisplayVersion}…", "Updating LAB Plus", MessageBoxButtons.OKCancel)
    {
        ShowInTaskbar = true;
        Compact = true; ClientSize = new Size(600, 300); MinimumSize = new Size(600, 300);
        AcceptButton!.Enabled = false;
        FormClosing += (_, _) => { if (!HandedOff) _cancel.Cancel(); };
        Shown += async (_, _) =>
        {
            try
            {
                string job = await UpdateInstaller.PrepareAsync(client, release, AppContext.BaseDirectory, _cancel.Token);
                _cancel.Token.ThrowIfCancellationRequested();
                MessageText.Text = "Installing the update. LAB Plus will restart automatically…";
                UpdateInstaller.StartHelper(job);
                HandedOff = true;
                CloseWithResult(DialogResult.OK);
            }
            catch (OperationCanceledException)
            {
                if (!IsDisposed) { MessageText.Text = "The update was cancelled or timed out. You can continue using this version."; AcceptButton!.Text = "Continue"; AcceptButton.Enabled = true; }
            }
            catch (Exception error) when (error is IOException or InvalidDataException or UnauthorizedAccessException or System.Net.Http.HttpRequestException
                or System.ComponentModel.Win32Exception or InvalidOperationException or System.Text.Json.JsonException)
            {
                if (!IsDisposed) { MessageText.Text = "The update could not be installed. You can continue using this version.\n\n" + error.Message; AcceptButton!.Text = "Continue"; AcceptButton.Enabled = true; }
            }
        };
    }
    protected override void Dispose(bool disposing)
    {
        if (disposing) _cancel.Cancel();
        // The async download still owns the token until it observes cancellation.
        base.Dispose(disposing);
    }
}
