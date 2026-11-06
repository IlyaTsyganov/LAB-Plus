using System.Diagnostics;
using System.Runtime.InteropServices;
using LabPlus.Configuration;
using LabPlus.Game;

namespace LabPlus;

internal sealed partial class MainForm : LabPlusForm
{
    private static readonly Color Canvas = LabPlusTheme.Canvas;
    private static readonly Color PanelColor = LabPlusTheme.Panel;
    private static readonly Color ForegroundColor = LabPlusTheme.Text;
    private static readonly Color AccentColor = LabPlusTheme.Accent;

    private readonly SettingsStore _settingsStore = new();
    private readonly ProcessMonitor _processMonitor = new();
    private readonly GameLauncher _gameLauncher;
    private readonly VulkanLayerRegistration _layerRegistration;
    private readonly System.Windows.Forms.Timer _refreshTimer = new() { Interval = 1500 };
    private readonly LabPlusSettings _settings;

    private readonly UiText _gameStatus = new();
    private readonly UiText _desruStatus = new();
    private readonly UiText _viewerStatus = new();
    private readonly UiText _pathValue = new();
    private readonly UiText _gameVersion = new();
    private readonly UiToolTip _gamePathToolTip = new() { AutoPopDelay = 30000, InitialDelay = 400, ReshowDelay = 100, ShowAlways = true };
    private readonly UiToggle _viewerEnabled = new WorkspaceSwitch();
    private readonly UiToggle _triggers = new WorkspaceSwitch();
    private readonly UiToggle _worldCollision = new WorkspaceSwitch();
    private readonly UiToggle _shotCollision = new WorkspaceSwitch();
    private readonly UiToggle _trimShotCollision = new WorkspaceSwitch();
    private readonly UiToggle _invisibleWalls = new WorkspaceSwitch();
    private readonly UiToggle _inspector = new WorkspaceSwitch();
    private readonly UiToggle _disabledVolumes = new WorkspaceSwitch();
    private readonly UiToggle _volumeHistory = new WorkspaceSwitch();
    private readonly UiAction _triggerTypes = new WorkspaceButton();
    private readonly UiChoice _outlines = new WorkspaceRadio();
    private readonly UiChoice _filled = new WorkspaceRadio();
    private readonly WorkspaceSlider _distance = new();
    private readonly UiNumber _distanceInput = new();
    private readonly UiAction _launchButton = new WorkspaceButton { Primary = true };
    private bool _syncingDistance;
    private bool _loadingControls;
    private ActiveEncountersForm? _activeEncounters;
    private InvisibleNumbersForm? _invisibleNumbers;
    private EntityGraphForm? _entityGraph;
    private MovementLabForm? _movementLab;
    private LevelStateForm? _levelStates;
    private ResearchPointsForm? _researchPoints;
    private PracticeSaveForm? _practiceSaves;

    public MainForm()
    {
        _settings = _settingsStore.Load();
        _gameLauncher = new GameLauncher(_settingsStore);
        _layerRegistration = new VulkanLayerRegistration(_settingsStore);
        if (!_preview) _layerRegistration.Disable();
        LabPlusTheme.Initialize();

        Text = "LAB Plus";
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(820, 800);
        MinimumSize = new Size(680, 580);
        FormBorderStyle = FormBorderStyle.None;
        ControlBox = false;
        MaximizeBox = false;
        SizeGripStyle = SizeGripStyle.Hide;
        BackColor = Canvas;
        ForeColor = ForegroundColor;
        Font = UiFont(11.25f);
        AutoScaleDimensions = new SizeF(96, 96);
        AutoScaleMode = AutoScaleMode.Dpi;
        try
        {
            string icon = Path.Combine(AppContext.BaseDirectory, "Assets", "labplus.ico");
            if (File.Exists(icon)) Icon = LabPlusTheme.ApplicationIcon();
        }
        catch (IOException) { }

        InitializeLabFeatures();
        BuildMainPage();
        LoadSettingsIntoControls();
        WireSettings();
        if (_settingsStore.Migrated && !_preview) SaveSettings();
        RefreshProcessStatus();
        _refreshTimer.Tick += (_, _) => RefreshProcessStatus();
        _refreshTimer.Start();
        FormClosed += (_, _) => { _refreshTimer.Stop(); _refreshTimer.Dispose(); _gamePathToolTip.Dispose(); _launchCancellation.Dispose(); };
    }

    private void LoadSettingsIntoControls()
    {
        _viewerEnabled.Checked = _settings.CollisionViewerEnabled;
        _viewerEnabled.Text = _viewerEnabled.Checked ? "Enabled" : "Disabled";
        _triggers.Checked = _settings.ShowTriggers;
        _worldCollision.Checked = _settings.ShowWorldCollision;
        _shotCollision.Checked = _settings.ShowShotCollision;
        _trimShotCollision.Checked = _settings.TrimShotCollision;
        _invisibleWalls.Checked = _settings.ShowInvisibleWalls;
        _inspector.Checked = _settings.InspectVolumes;
        _disabledVolumes.Checked = _settings.ShowDisabledVolumes;
        _volumeHistory.Checked = _settings.ShowVolumeHistory;
        _outlines.Checked = _settings.Style == RenderStyle.Outlines;
        _filled.Checked = _settings.Style == RenderStyle.Filled;
        int distance = Math.Clamp(_settings.ViewDistanceMeters, _distance.Minimum, _distance.Maximum);
        _distance.Value = distance;
        _distanceInput.Value = distance;
        UpdateGameSelection();
        UpdateViewerControls();
    }

    private void WireSettings()
    {
        _viewerEnabled.CheckedChanged += (_, _) => { _viewerEnabled.Text = _viewerEnabled.Checked ? "Enabled" : "Disabled"; _settings.CollisionViewerEnabled = _viewerEnabled.Checked; UpdateViewerControls(); SaveSettings(); };
        _triggers.CheckedChanged += (_, _) => { _settings.ShowTriggers = _triggers.Checked; SaveSettings(); };
        _worldCollision.CheckedChanged += (_, _) => { _settings.ShowWorldCollision = _worldCollision.Checked; UpdateViewerControls(); SaveSettings(); };
        _shotCollision.CheckedChanged += (_, _) => { _settings.ShowShotCollision = _shotCollision.Checked; SaveSettings(); };
        _trimShotCollision.CheckedChanged += (_, _) => { _settings.TrimShotCollision = _trimShotCollision.Checked; SaveSettings(); };
        _invisibleWalls.CheckedChanged += (_, _) => { _settings.ShowInvisibleWalls = _invisibleWalls.Checked; SaveSettings(); };
        _inspector.CheckedChanged += (_, _) => { _settings.InspectVolumes = _inspector.Checked; SaveSettings(); };
        _disabledVolumes.CheckedChanged += (_, _) => { _settings.ShowDisabledVolumes = _disabledVolumes.Checked; SaveSettings(); };
        _volumeHistory.CheckedChanged += (_, _) => { _settings.ShowVolumeHistory = _volumeHistory.Checked; SaveSettings(); };
        _outlines.CheckedChanged += (_, _) => { if (_outlines.Checked) { _settings.Style = RenderStyle.Outlines; SaveSettings(); } };
        _filled.CheckedChanged += (_, _) => { if (_filled.Checked) { _settings.Style = RenderStyle.Filled; SaveSettings(); } };
        _distance.ValueChanged += (_, _) =>
        {
            if (_syncingDistance) return;
            _syncingDistance = true;
            _distanceInput.Value = _distance.Value;
            _syncingDistance = false;
            _settings.ViewDistanceMeters = _distance.Value;
            SaveSettings();
        };
        _distanceInput.ValueChanged += (_, _) =>
        {
            if (_syncingDistance) return;
            _syncingDistance = true;
            _distance.Value = decimal.ToInt32(_distanceInput.Value);
            _syncingDistance = false;
            _settings.ViewDistanceMeters = _distance.Value;
            SaveSettings();
        };
    }

    private void BrowseForGame()
    {
        using var dialog = new OpenFileDialog
        {
            Title = "Select DOOM Eternal executable",
            Filter = "DOOM Eternal executable (DOOMEternalx64vk.exe)|DOOMEternalx64vk.exe|Executable files (*.exe)|*.exe",
            CheckFileExists = true
        };
        if (dialog.ShowDialog(this) != DialogResult.OK) return;
        _settings.GameExecutablePath = dialog.FileName;
        UpdateGameSelection();
        SaveSettings();
    }

    private void UpdateGameSelection()
    {
        SyncLabGamePath();
        string path = _settings.GameExecutablePath;
        bool selected = !string.IsNullOrWhiteSpace(path);
        _pathValue.Text = selected ? path : "No game executable selected";
        _gamePathToolTip.SetToolTip(_pathValue, selected ? path : null);
        if (!selected)
        {
            _gameVersion.Text = "Version: —";
            return;
        }
        try
        {
            var info = GameExecutableInfo.Read(path);
            _gameVersion.Text = "Version: " + (info?.Version ?? "Unknown");
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or System.Security.SecurityException)
        {
            _gameVersion.Text = "Version: Unavailable";
        }
    }

    private readonly CancellationTokenSource _launchCancellation = new();
    private string? _launchStatus;
    private bool _launching;
    private async void LaunchGame()
    {
        if (_launching || !_hudMonitor.TryBeginLaunch()) return;
        _launching = true;
        try
        {
            _launchButton.Enabled = false;
            Workspace?.Publish(true);
            string gamePath = _settings.GameExecutablePath;
            var progress = new Progress<string>(message => { if (_launching && !_labClosing && !IsDisposed) { _launchStatus = message; Workspace?.Publish(true); } });
            var process = await _gameLauncher.StartAsync(_settings, ConfigureLabLaunch, progress, _launchCancellation.Token);
            if (_labClosing || IsDisposed) { process.Dispose(); return; }
            Hud.Runtime.GameLaunchHistory.Record(process, gamePath, _hudStore);
            process.Dispose();
            RefreshProcessStatus();
        }
        catch (OperationCanceledException) when (_launchCancellation.IsCancellationRequested) { _hudMonitor.CancelLaunch(); }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidOperationException or System.ComponentModel.Win32Exception)
        {
            _hudMonitor.CancelLaunch();
            if (_labClosing || IsDisposed) return;
            RefreshProcessStatus();
            WorkspaceDialog.Show(this, ex.Message, "Unable to launch DOOM Eternal", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally { _launching = false; _launchStatus = null; if (!_labClosing && !IsDisposed) Workspace?.Publish(true); }
    }

    private void SaveSettings()
    {
        if (_loadingControls) return;
        try
        {
            _settingsStore.Save(_settings);
            if (_preview) return;
            _layerRegistration.Disable();
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidOperationException or System.Security.SecurityException)
        {
            WorkspaceDialog.Show(this, "Settings could not be applied: " + ex.Message, "LAB Plus Settings", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
    }

    private int SelectedGameProcess()
    {
        if (_preview) return 0;
        int selected = 0;
        foreach (Process process in Process.GetProcessesByName("DOOMEternalx64vk"))
        {
            using (process)
            {
                try
                {
                    if (!string.Equals(process.MainModule?.FileName, _settings.GameExecutablePath, StringComparison.OrdinalIgnoreCase)) continue;
                    if (!Hud.Runtime.GameLaunchHistory.Matches(process, _hudStore)) continue;
                    if (selected != 0) return 0;
                    selected = process.Id;
                }
                catch (Exception ex) when (ex is InvalidOperationException or System.ComponentModel.Win32Exception) { }
            }
        }
        return selected;
    }

    internal void ReceiveLabCommand(int process, string[] args)
    {
        if (!IsHandleCreated || IsDisposed) return;
        BeginInvoke(() =>
        {
            if (IsDisposed || SelectedGameProcess() != process) return;
            if (PracticeSaveCommand.TryParse(args, out var token)) { _ = LoadLastSaveCommand(process, token); return; }
            if (!LabCommands.Apply(_settings, args, process, DateTime.UtcNow.Ticks)) return;
            _loadingControls = true;
            try { LoadSettingsIntoControls(); }
            finally { _loadingControls = false; }
            SaveSettings();
        });
    }

    private async Task LoadLastSaveCommand(int process, string token)
    {
        var result = await EnsurePracticeSaves().LoadLastCheckpointFromCommand(process);
        if (!IsDisposed) PracticeSaveCommand.Reply(_settingsStore.SettingsDirectory, process, token, result.Success, result.Message);
    }

    internal void StopResearchOnExit()
    {
        if (!_settings.RecordResearch) return;
        _settings.RecordResearch = false;
        try { _settingsStore.Save(_settings); }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
    }

    private void RefreshProcessStatus()
    {
        string? expectedViewer = null;
        try { expectedViewer = _layerRegistration.ExpectedRuntimeLibraryName; }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or System.Security.SecurityException) { }
        var snapshot = _preview ? new ProcessSnapshot(false, 0, false, 0, DateTime.Now) : _processMonitor.ReadSnapshot(expectedViewer);
        SetStatus(_gameStatus, snapshot.GameRunning ? "Running" : "Not running", snapshot.GameRunning);
        SetStatus(_desruStatus, snapshot.DesruRunning ? "Running" : "Not running", snapshot.DesruRunning);
        bool viewerEnabled = _settings.CollisionViewerEnabled;
        bool restartRequired = _settings.NeedsNativeLayer && snapshot.GameRunning &&
            (_hudMonitor.RequiresRestart || snapshot.ViewerUpdateRequired || (!snapshot.ViewerVersionChecked && snapshot.GameStartedBeforeViewer));
        string viewerStatus = restartRequired ? (snapshot.ViewerUpdateRequired ? "Update pending" : "Restart game")
            : !viewerEnabled ? "Disabled"
            : snapshot.GameRunning ? "Running" : "Enabled";
        SetStatus(_viewerStatus, viewerStatus, viewerEnabled, restartRequired);
        _gamePathToolTip.SetToolTip(_viewerStatus, snapshot.ViewerUpdateRequired
            ? $"The running game has not loaded this viewer build. Expected: {expectedViewer}. Loaded: {string.Join(", ", snapshot.LoadedViewerLibraries.DefaultIfEmpty("none"))}. Fully exit the game and launch it again through LAB Plus."
            : snapshot.ViewerVersionChecked ? $"Loaded viewer: {string.Join(", ", snapshot.LoadedViewerLibraries)}" : null);
        _launchButton.Enabled = !snapshot.GameRunning && !_hudMonitor.LaunchPending && !string.IsNullOrWhiteSpace(_settings.GameExecutablePath);
    }

    private void ShowHelp() => _ = Workspace?.Browser.ExecuteScriptAsync("LabPlusHelp.open()");

    private void OpenLink(string url)
    {
        try
        {
            Process.Start(new ProcessStartInfo(url) { UseShellExecute = true });
        }
        catch (Exception ex) when (ex is InvalidOperationException or System.ComponentModel.Win32Exception)
        {
            WorkspaceDialog.Show(this, ex.Message, "Unable to open link", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void OpenDataDirectory()
    {
        try { Directory.CreateDirectory(_settingsStore.SettingsDirectory); OpenLink(_settingsStore.SettingsDirectory); }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        { WorkspaceDialog.Show(this, "The research folder could not be opened: " + ex.Message, "LAB Plus", MessageBoxButtons.OK, MessageBoxIcon.Error); }
    }

    private void UpdateViewerControls()
    {
        bool enabled = _viewerEnabled.Checked;
        _triggers.Enabled = _worldCollision.Enabled = _invisibleWalls.Enabled = _shotCollision.Enabled = enabled;
        _trimShotCollision.Enabled = enabled;
        _outlines.Enabled = _filled.Enabled = _distance.Enabled = _distanceInput.Enabled = enabled;
        _inspector.Enabled = enabled;
        _disabledVolumes.Enabled = _volumeHistory.Enabled = enabled;
        _triggerTypes.Enabled = enabled;
        RefreshProcessStatus();
    }

}
