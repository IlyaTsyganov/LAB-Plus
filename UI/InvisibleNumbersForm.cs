using LabPlus.Configuration;
using LabPlus.Game;
using System.Globalization;

namespace LabPlus;

internal sealed class InvisibleNumbersForm : LabPlusForm
{
    private readonly LabPlusSettings _settings;
    private readonly string _directory;
    private readonly Func<int> _gameProcess;
    private readonly Action _save;
    private readonly UiText _status = new() { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft };
    private readonly UiTable _techs = new();
    private readonly UiToggle _enemiesHud = new() { Text = "Enemy HUD", AutoSize = true };
    private readonly UiToggle _bars = new() { Text = "Enemy HP bars", AutoSize = true };
    private readonly UiToggle _syncTarget = new() { Text = "Sync animation zones (1.1)", AutoSize = true,
        ToolTipText = "A wire box marks the enemy under your crosshair. Native aim quadrants are highlighted on the box. If the chosen animation requires a specific head, arm or torso group, yellow patches mark its visible aim area on the box instead. Lists accepting all current hit groups do not receive anatomical patches. These follow the current body pose from the front, sides and back, and are clipped to any required quadrant; covered parts are left blank. Close side views keep visible parts even when the enemy box crosses the camera. Body-specific animations also show a standing reference before the current focus matches, assuming you aim at that highlighted part. Native priorities and shared aim requirements determine which closer variants are cut out. Different required body parts can share the same standing area. The HUD reports an unavailable or hidden body area. Keep aiming there until the native focus matches. A translucent reference sector follows the position and facing of the enemy, assuming the player faces its origin. Moving the crosshair within the same aim area does not drag or rotate the sector. Current aim and native validation are checked separately. The display height matches the enemy box. An amber boundary band marks exclusive origin positions without full player-hitbox clearance. Shared random-choice positions stay unfilled. Filled core areas keep clearance for the current horizontal player hitbox; native height checks still use the feet. Outside the filled area the selected animation can still occur. The in-game Engine now line separately reports current priority/distance choices. Grey is reference only, amber means boundary or pending checks, red means rejected, green means the sole observed choice with full footprint clearance. Unknown rules hide the positional area. Enemy state, obstacles and late checks still apply. DOOM Eternal 1.1 only. Experimental." };
    private readonly UiSelect _syncAction = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 150,
        ToolTipText = "Glory Kill includes its position and aim requirements. Chainsaw uses separate rules: a front cut does not imply a front-only sector. Both modes exclude competing variants allowed for the current aim area." };
    private readonly UiSelect _syncAnimation = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 275,
        ToolTipText = "Animations are read from the enemy under your crosshair, including its assigned rules before GK state. Competing areas are subtracted when their priority and required aim can conflict with the selected animation. Chainsaw also compares current native priorities and start poses; the random-choice band is excluded. The amber boundary band retains origin-exclusive positions near collider-clearance limits. The filled area is a conservative subset, not the complete activation region. The animation can still occur outside it, including in the excluded random band. Current engine priority/distance choices are reported separately. Automatic prioritizes a zone you enter, including its amber boundary, then current aim and a stable choice in overlaps. Looking away keeps the last enemy within the selected action radius as a reference. Aim at another enemy to switch targets. Selecting a variant here does not force it in the game." };
    private readonly UiText _syncHelp = new() { Dock = DockStyle.Fill, Text = "Aim at a living enemy to load its animations.", AutoSize = true };
    private sealed record AnimationOption(string Key, string Label) { public override string ToString() => Label; }
    private readonly UiToggle _timersHud = new() { Text = "Timer HUD", AutoSize = true };
    private readonly UiToggle _techsHud = new() { Text = "Speedrun tech HUD", AutoSize = true };
    private readonly UiTable _enemies = new(), _timers = new();
    private readonly System.Windows.Forms.Timer _refresh = new() { Interval = 250 };
    private List<EnemyNumber> _enemyRows = [];
    private IReadOnlyList<TimerNumber> _timerRows = [];
    private IReadOnlyList<TechNumber> _techRows = [];
    private bool _busy, _syncing;
    private SyncAnimationPreview? _lastSyncPreview;
    private SyncAnimationPreview? _shownSyncPreview;
    private string _syncCatalogMap = "";
    private int _syncCatalogProcess;

    internal InvisibleNumbersForm(LabPlusSettings settings, string directory, Action save, Func<int> gameProcess)
    {
        _settings = settings; _directory = directory; _save = save; _gameProcess = gameProcess;
        Text = "LAB Plus - Invisible numbers"; ClientSize = new(1060, 760); MinimumSize = new(800, 600);
        StartPosition = FormStartPosition.CenterParent; ShowInTaskbar = false;
        Font = LabPlusTheme.Ui(10f); BackColor = Color.FromArgb(35, 35, 35); ForeColor = Color.Gainsboro;
        var layout = new UiGrid { Dock = DockStyle.Fill, Padding = new(12), ColumnCount = 1, RowCount = 9 };
        layout.ColumnStyles.Add(new(SizeType.Percent, 100));
        foreach (var row in new[] { new RowStyle(SizeType.Absolute, 38), new(SizeType.Absolute, 64), new(SizeType.Absolute, 96), new(SizeType.Absolute, 30),
            new(SizeType.Percent, 55), new(SizeType.Absolute, 30), new(SizeType.Percent, 45), new(SizeType.Absolute, 30), new(SizeType.Absolute, 96) }) layout.RowStyles.Add(row);
        Controls.Add(layout);
        var options = new UiToolbar { Dock = DockStyle.Fill, WrapContents = false };
        foreach (var toggle in new[] { _enemiesHud, _bars, _syncTarget, _timersHud, _techsHud }) { toggle.Margin = new(0, 4, 22, 0); options.Controls.Add(toggle); }
        _enemiesHud.CheckedChanged += (_, _) => SaveOptions(); _bars.CheckedChanged += (_, _) => SaveOptions(); _timersHud.CheckedChanged += (_, _) => SaveOptions();
        _techsHud.CheckedChanged += (_, _) => SaveOptions();
        _syncTarget.CheckedChanged += (_, _) => SaveOptions();
        _syncAction.Items.AddRange(["Glory Kill", "Chainsaw"]); _syncAction.SelectedIndexChanged += (_, _) => { if (_syncing) return; SaveOptions(); UpdateSyncAnimations(null); };
        _syncAnimation.SelectedIndexChanged += (_, _) =>
        {
            if (_syncing || _syncAnimation.SelectedItem is not AnimationOption option) return;
            if (_settings.SyncTargetAction == 1) _settings.SyncChainsawAnimation = option.Key; else _settings.SyncGloryKillAnimation = option.Key;
            _save();
            if (_shownSyncPreview?.Action == _settings.SyncTargetAction) UpdateSyncAnimations(_shownSyncPreview);
        };
        var syncOptions = new UiToolbar { Dock = DockStyle.Fill, WrapContents = false };
        syncOptions.Controls.Add(new UiText { Text = "Sync preview:", AutoSize = true, Margin = new(0, 7, 12, 0) }); syncOptions.Controls.Add(_syncAction);
        syncOptions.Controls.Add(new UiText { Text = "Animation:", AutoSize = true, Margin = new(14, 7, 12, 0) }); syncOptions.Controls.Add(_syncAnimation);
        var syncGroup = new UiGrid { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 2 };
        syncGroup.ColumnStyles.Add(new(SizeType.Percent, 100)); syncGroup.RowStyles.Add(new(SizeType.Absolute, 34)); syncGroup.RowStyles.Add(new(SizeType.Percent, 100));
        syncGroup.Controls.Add(syncOptions, 0, 0); syncGroup.Controls.Add(_syncHelp, 0, 1);
        layout.Controls.Add(options, 0, 0); layout.Controls.Add(syncGroup, 0, 1); layout.Controls.Add(_status, 0, 2);
        layout.Controls.Add(new UiText { Text = "Living enemies by active encounter", Dock = DockStyle.Fill }, 0, 3);
        Setup(_enemies, [("Encounter", 270), ("Enemy", 220), ("HP", 100), ("Maximum HP", 110), ("Entity", 260)]);
        _enemies.CellValueNeeded += (_, e) =>
        {
            if (e.RowIndex >= _enemyRows.Count) return;
            var row = _enemyRows[e.RowIndex];
            e.Value = e.ColumnIndex switch { 0 => row.Encounter, 1 => row.Name.Length > 0 ? row.Name : row.ClassName, 2 => row.Health.ToString("0.##", CultureInfo.InvariantCulture),
                3 => row.MaximumHealth?.ToString("0.##", CultureInfo.InvariantCulture) ?? "Unavailable", 4 => $"{row.Id} / {row.Generation} · {row.ClassName}", _ => "" };
        };
        layout.Controls.Add(_enemies, 0, 4);
        layout.Controls.Add(new UiText { Text = "Equipment, powerups, weapon mods and sync animations", Dock = DockStyle.Fill }, 0, 5);
        Setup(_timers, [("Timer", 205), ("Category", 120), ("State", 115), ("Remaining / maximum", 170), ("Sticky charges", 290), ("Details", 345)]);
        _timers.CellValueNeeded += (_, e) =>
        {
            if (e.RowIndex >= _timerRows.Count) return;
            var row = _timerRows[e.RowIndex];
            e.Value = e.ColumnIndex switch { 0 => row.Label, 1 => row.Group, 2 => row.State == "not_acquired" ? "Not acquired" : row.State,
                3 => Countdown(row), 4 => row.Key == "sticky_bomb" ? string.Join("   ", _stickyCharges.Select((charge, index) => $"{index + 1}: {Countdown(charge)}")) : "",
                5 => row.Detail, _ => "" };
        };
        layout.Controls.Add(_timers, 0, 6);
        layout.Controls.Add(new UiText { Text = "Speedrun techniques", Dock = DockStyle.Fill }, 0, 7);
        Setup(_techs, [("Technique", 235), ("State", 125), ("Details", 595)]);
        _techs.CellValueNeeded += (_, e) =>
        {
            if (e.RowIndex >= _techRows.Count) return;
            var row = _techRows[e.RowIndex]; e.Value = e.ColumnIndex switch { 0 => row.Label, 1 => row.State, 2 => row.Detail, _ => "" };
        };
        layout.Controls.Add(_techs, 0, 8);
        _refresh.Tick += async (_, _) => await RefreshSnapshot();
        Shown += async (_, _) => { _refresh.Start(); await RefreshSnapshot(); };
    }

    private IReadOnlyList<TimerNumber> _stickyCharges = [];
    private static string Countdown(TimerNumber row) => row.Seconds is double seconds
        ? seconds.ToString("0.00", CultureInfo.InvariantCulture) + (row.MaximumSeconds is double maximum ? " / " + maximum.ToString("0.00", CultureInfo.InvariantCulture) : "") + " s"
        : "-";

    private static void Setup(UiTable table, (string Title, int Width)[] columns)
    {
        table.Dock = DockStyle.Fill; table.ReadOnly = true; table.VirtualMode = true; table.RowHeadersVisible = false;
        table.AllowUserToAddRows = table.AllowUserToDeleteRows = false;
        table.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
        foreach (var column in columns) table.Columns.Add(new UiTableColumn { HeaderText = column.Title, Width = column.Width, SortMode = DataGridViewColumnSortMode.NotSortable });
    }
    private void SaveOptions()
    {
        if (_syncing) return;
        _settings.ShowEnemyHealthHud = _enemiesHud.Checked; _settings.ShowEnemyHealthBars = _bars.Checked;
        _settings.ShowSyncTarget = _syncTarget.Checked;
        _settings.SyncTargetAction = _syncAction.SelectedIndex == 1 ? 1 : 0;
        _syncAction.Enabled = _syncTarget.Checked;
        _syncAnimation.Enabled = _syncTarget.Checked && _syncAnimation.Items.Count > 1;
        _settings.ShowGameTimersHud = _timersHud.Checked; _settings.ShowSpeedrunTechsHud = _techsHud.Checked; _save();
    }
    private void UpdateSyncAnimations(SyncAnimationPreview? preview)
    {
        _shownSyncPreview = preview?.Action == _settings.SyncTargetAction ? preview : null;
        bool syncing = _syncing; _syncing = true;
        try
        {
            string saved = LabPlusSettings.SyncAnimationKey(_settings.SyncTargetAction == 1 ? _settings.SyncChainsawAnimation : _settings.SyncGloryKillAnimation);
            var available = preview?.Action == _settings.SyncTargetAction ? preview.Animations : Array.Empty<string>();
            string automatic = preview?.Action == _settings.SyncTargetAction && available.Contains(preview.AutomaticAnimation, StringComparer.Ordinal) ? preview.AutomaticAnimation : "";
            var choices = new List<AnimationOption> { new("", automatic.Length > 0 ? "Automatic: " + AnimationLabel(automatic) : available.Count > 0 ? "Automatic (position and aim)" : "Automatic (aim at an enemy)") };
            choices.AddRange(available.Select(name => new AnimationOption(name, AnimationLabel(name))));
            if (saved.Length > 0 && !available.Contains(saved, StringComparer.Ordinal)) choices.Add(new(saved, AnimationLabel(saved) + " (unavailable on target)"));
            if (!_syncAnimation.Items.Cast<AnimationOption>().SequenceEqual(choices)) { _syncAnimation.Items.Clear(); _syncAnimation.Items.AddRange(choices.Cast<object>().ToArray()); }
            _syncAnimation.SelectedIndex = choices.FindIndex(option => option.Key == saved);
            _syncAnimation.Enabled = _settings.ShowSyncTarget && available.Count > 0;
            _syncHelp.Text = available.Count == 0 ? "Aim at a living enemy to load supported animations."
                : saved.Length > 0 && !available.Contains(saved, StringComparer.Ordinal) ? "This enemy has no " + AnimationLabel(saved) + ". Choose Automatic or another animation."
                : _settings.SyncTargetAction == 1 ? "Chainsaw: filled area excludes competing choices and adds player clearance. The animation can still occur outside it."
                : "Filled sectors exclude competing choices and add player clearance. Outside does not mean the animation is impossible.";
            string active = saved.Length > 0 ? saved : automatic;
            if (preview?.Action == _settings.SyncTargetAction && available.Contains(active, StringComparer.Ordinal) && preview.Hints.TryGetValue(active, out var hint))
                _syncHelp.Text = (hint.Enemy.Length > 0 ? hint.Enemy + ": " : "") + hint.Aim + ". " + _syncHelp.Text;
        }
        finally { _syncing = syncing; }
    }
    private static string AnimationLabel(string name) => name.Replace("kill_", "").Replace("cut_", "").Replace('_', ' ');
    private void UpdateSyncCatalog(InvisibleNumbersSnapshot? snapshot, bool fresh, int process)
    {
        var preview = fresh && snapshot!.Ready ? snapshot.SyncPreview : null;
        bool paused = fresh && snapshot!.Phase == "paused";
        if (preview is not null) { _lastSyncPreview = preview; _syncCatalogMap = snapshot!.Map; _syncCatalogProcess = process; }
        else if (paused && _syncCatalogProcess == process && _syncCatalogMap == snapshot!.Map) preview = _lastSyncPreview;
        else { _lastSyncPreview = null; _syncCatalogMap = ""; _syncCatalogProcess = 0; }
        UpdateSyncAnimations(preview);
        if (paused && preview?.Action == _settings.SyncTargetAction)
            _syncHelp.Text = "Paused: choices from the last aimed enemy. Return to the game to preview.";
    }
    private async Task RefreshSnapshot()
    {
        if (_busy || IsDisposed) return; _busy = true;
        try
        {
            _syncing = true; _enemiesHud.Checked = _settings.ShowEnemyHealthHud; _bars.Checked = _settings.ShowEnemyHealthBars;
            _syncTarget.Checked = _settings.ShowSyncTarget;
            _syncAction.SelectedIndex = _settings.SyncTargetAction == 1 ? 1 : 0; _syncAction.Enabled = _settings.ShowSyncTarget;
            _timersHud.Checked = _settings.ShowGameTimersHud; _techsHud.Checked = _settings.ShowSpeedrunTechsHud; _syncing = false;
            int process = _gameProcess();
            var snapshot = await Task.Run(() =>
            {
                if (process <= 0) return null;
                Directory.CreateDirectory(_directory);
                File.WriteAllText(Path.Combine(_directory, $"invisible-numbers-{process}.request"), DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(CultureInfo.InvariantCulture));
                return InvisibleNumbersSnapshot.Read(_directory, process);
            });
            if (IsDisposed) return;
            bool fresh = snapshot is not null && snapshot.Process == _gameProcess() && snapshot.IsFresh(DateTimeOffset.UtcNow);
            UpdateSyncCatalog(snapshot, fresh, process);
            _enemyRows = fresh && snapshot!.Ready ? snapshot.Enemies.OrderBy(row => row.Encounter, StringComparer.Ordinal).ThenBy(row => row.Id).ToList() : [];
            _stickyCharges = fresh && snapshot!.Ready ? snapshot.Timers.Where(row => row.Group == "sticky_charges").ToList() : [];
            _timerRows = fresh && snapshot!.Ready ? snapshot.Timers.Where(row => row.Group != "sticky_charges").ToList() : [];
            _techRows = fresh && snapshot!.Ready ? snapshot.Techs : [];
            _enemies.RowCount = _enemyRows.Count; _enemies.Invalidate(); _timers.RowCount = _timerRows.Count; _timers.Invalidate();
            _techs.RowCount = _techRows.Count; _techs.Invalidate();
            _status.Text = process <= 0 ? "Launch the selected game through LAB Plus."
                : snapshot is null ? "Waiting for Invisible numbers. Enable a display option above or Collision Viewer, then fully restart the game through this build of LAB Plus."
                : !fresh ? "Waiting for fresh game data."
                : $"{snapshot.Phase} · Enemies: {snapshot.EnemiesState} · Timers: {snapshot.TimersState}\n{snapshot.Map} · {snapshot.Detail}";
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        {
            if (!IsDisposed) UpdateSyncCatalog(null, false, 0);
            if (!IsDisposed) { _enemyRows = []; _timerRows = []; _techRows = []; _enemies.RowCount = 0; _timers.RowCount = 0; _techs.RowCount = 0; _status.Text = "Waiting for a readable game snapshot."; }
        }
        finally { _busy = false; }
    }
    protected override void Dispose(bool disposing) { if (disposing) _refresh.Dispose(); base.Dispose(disposing); }
}
