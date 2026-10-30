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
        LabPlusTheme.Initialize();

        Text = "LAB Plus";
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(992, 840);
        MinimumSize = new Size(820, 730);
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
            if (File.Exists(icon)) Icon = new Icon(icon);
        }
        catch (IOException) { }

        BuildMainPage();
        LoadSettingsIntoControls();
        WireSettings();
        if (_settingsStore.Migrated && !_preview) SaveSettings();
        RefreshProcessStatus();
        _refreshTimer.Tick += (_, _) => RefreshProcessStatus();
        _refreshTimer.Start();
        FormClosed += (_, _) => { _refreshTimer.Stop(); _refreshTimer.Dispose(); _gamePathToolTip.Dispose(); };
    }

    private void LoadSettingsIntoControls()
    {
        _viewerEnabled.Checked = _settings.CollisionViewerEnabled;
        _viewerEnabled.Text = _viewerEnabled.Checked ? "Enabled" : "Disabled";
        _triggers.Checked = _settings.ShowTriggers;
        _worldCollision.Checked = _settings.ShowWorldCollision;
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
        _worldCollision.CheckedChanged += (_, _) => { _settings.ShowWorldCollision = _worldCollision.Checked; SaveSettings(); };
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

    private void LaunchGame()
    {
        try
        {
            var process = _gameLauncher.Start(_settings);
            process.Dispose();
            RefreshProcessStatus();
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidOperationException or System.ComponentModel.Win32Exception)
        {
            WorkspaceDialog.Show(this, ex.Message, "Unable to launch DOOM Eternal", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void SaveSettings()
    {
        if (_loadingControls) return;
        try
        {
            _settingsStore.Save(_settings);
            if (_preview) return;
            if (_settings.CollisionViewerEnabled && File.Exists(_settings.GameExecutablePath))
                _layerRegistration.EnableForGame(_settings.GameExecutablePath);
            else
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
            if (!LabCommands.Apply(_settings, args, process, DateTime.UtcNow.Ticks)) return;
            _loadingControls = true;
            try { LoadSettingsIntoControls(); }
            finally { _loadingControls = false; }
            SaveSettings();
        });
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
        var snapshot = _processMonitor.ReadSnapshot(expectedViewer);
        SetStatus(_gameStatus, snapshot.GameRunning ? "Running" : "Not running", snapshot.GameRunning);
        SetStatus(_desruStatus, snapshot.DesruRunning ? "Running" : "Not running", snapshot.DesruRunning);
        bool viewerEnabled = _settings.CollisionViewerEnabled;
        bool restartRequired = viewerEnabled && snapshot.GameRunning &&
            (snapshot.ViewerUpdateRequired || (!snapshot.ViewerVersionChecked && snapshot.GameStartedBeforeViewer));
        string viewerStatus = !viewerEnabled ? "Disabled"
            : restartRequired ? (snapshot.ViewerUpdateRequired ? "Update pending" : "Restart game")
            : snapshot.GameRunning ? "Running" : "Enabled";
        SetStatus(_viewerStatus, viewerStatus, viewerEnabled, restartRequired);
        _gamePathToolTip.SetToolTip(_viewerStatus, snapshot.ViewerUpdateRequired
            ? $"The running game has not loaded this viewer build. Expected: {expectedViewer}. Loaded: {string.Join(", ", snapshot.LoadedViewerLibraries.DefaultIfEmpty("none"))}. Fully exit the game and launch it again through LAB Plus."
            : snapshot.ViewerVersionChecked ? $"Loaded viewer: {string.Join(", ", snapshot.LoadedViewerLibraries)}" : null);
        _launchButton.Enabled = !snapshot.GameRunning && !string.IsNullOrWhiteSpace(_settings.GameExecutablePath);
    }

    private void ShowHelp()
    {
        WorkspaceDialog.Show(this,
            "LAB Plus is an unofficial DOOM Eternal research companion.\n\n" +
            "VIEWER: select DOOMEternalx64vk.exe in Settings, enable Collision Viewer, choose geometry and rendering, then launch the game here. Viewer activation requires the next full game launch. Preferences save automatically.\n\n" +
            "LAUNCH: use DESRU to enable an effective outbound Windows Firewall block for the selected executable. LAB Plus reads firewall state without changing its rules.\n\n" +
            "RESEARCH: movement attempts, recorded paths, entity references, before/after level comparisons, active encounters and named research points are grouped under Research. References and captured state are observations, not proof of script execution. The volume inspector and native HUD require compatible meathook.\n\n" +
            "SAVES: pause the game, optionally enter a name and press Save. Select a checkpoint and press Load. Live loading supports 6.66 Rev 3, TAG1, slot 3 with Steam Cloud disabled. Checkpoints are completed autosaves, not RAM snapshots. Advanced tools retain the existing account, archive and recovery checks.\n\n" +
            "CONSOLE: labplus_commands_list lists cvars and actions. Use labplus_encounters_show 1 for the encounter HUD, labplus_recording_start to record and labplus_recording_stop to stop. Keep LAB Plus open.\n\n" +
            "Bundled DOOM Eternal typefaces and logo assets remain the property of their respective owners. LAB Plus is not affiliated with id Software or Bethesda.",
            "LAB Plus Help", MessageBoxButtons.OK, MessageBoxIcon.Information);
    }

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
        _triggers.Enabled = _worldCollision.Enabled = _invisibleWalls.Enabled = enabled;
        _outlines.Enabled = _filled.Enabled = _distance.Enabled = _distanceInput.Enabled = enabled;
        _inspector.Enabled = enabled;
        _disabledVolumes.Enabled = _volumeHistory.Enabled = enabled;
        _triggerTypes.Enabled = enabled;
        RefreshProcessStatus();
    }

}
