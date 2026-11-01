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
    private readonly UiToggle _timersHud = new() { Text = "Timer HUD", AutoSize = true };
    private readonly UiToggle _techsHud = new() { Text = "Speedrun tech HUD", AutoSize = true };
    private readonly UiTable _enemies = new(), _timers = new();
    private readonly System.Windows.Forms.Timer _refresh = new() { Interval = 250 };
    private List<EnemyNumber> _enemyRows = [];
    private IReadOnlyList<TimerNumber> _timerRows = [];
    private IReadOnlyList<TechNumber> _techRows = [];
    private bool _busy, _syncing;

    internal InvisibleNumbersForm(LabPlusSettings settings, string directory, Action save, Func<int> gameProcess)
    {
        _settings = settings; _directory = directory; _save = save; _gameProcess = gameProcess;
        Text = "LAB Plus - Invisible numbers"; ClientSize = new(1060, 760); MinimumSize = new(800, 600);
        StartPosition = FormStartPosition.CenterParent; ShowInTaskbar = false;
        Font = LabPlusTheme.Ui(10f); BackColor = Color.FromArgb(35, 35, 35); ForeColor = Color.Gainsboro;
        var layout = new UiGrid { Dock = DockStyle.Fill, Padding = new(12), ColumnCount = 1, RowCount = 8 };
        layout.ColumnStyles.Add(new(SizeType.Percent, 100));
        foreach (var row in new[] { new RowStyle(SizeType.Absolute, 38), new(SizeType.Absolute, 64), new(SizeType.Absolute, 30),
            new(SizeType.Percent, 55), new(SizeType.Absolute, 30), new(SizeType.Percent, 45), new(SizeType.Absolute, 30), new(SizeType.Absolute, 96) }) layout.RowStyles.Add(row);
        Controls.Add(layout);
        var options = new UiToolbar { Dock = DockStyle.Fill, WrapContents = false };
        foreach (var toggle in new[] { _enemiesHud, _bars, _timersHud, _techsHud }) { toggle.Margin = new(0, 4, 22, 0); options.Controls.Add(toggle); }
        _enemiesHud.CheckedChanged += (_, _) => SaveOptions(); _bars.CheckedChanged += (_, _) => SaveOptions(); _timersHud.CheckedChanged += (_, _) => SaveOptions();
        _techsHud.CheckedChanged += (_, _) => SaveOptions();
        layout.Controls.Add(options, 0, 0); layout.Controls.Add(_status, 0, 1);
        layout.Controls.Add(new UiText { Text = "Living enemies by active encounter", Dock = DockStyle.Fill }, 0, 2);
        Setup(_enemies, [("Encounter", 270), ("Enemy", 220), ("HP", 100), ("Maximum HP", 110), ("Entity", 260)]);
        _enemies.CellValueNeeded += (_, e) =>
        {
            if (e.RowIndex >= _enemyRows.Count) return;
            var row = _enemyRows[e.RowIndex];
            e.Value = e.ColumnIndex switch { 0 => row.Encounter, 1 => row.Name.Length > 0 ? row.Name : row.ClassName, 2 => row.Health.ToString("0.##", CultureInfo.InvariantCulture),
                3 => row.MaximumHealth?.ToString("0.##", CultureInfo.InvariantCulture) ?? "Unavailable", 4 => $"{row.Id} / {row.Generation} · {row.ClassName}", _ => "" };
        };
        layout.Controls.Add(_enemies, 0, 3);
        layout.Controls.Add(new UiText { Text = "Equipment, powerups, weapon mods and sync animations", Dock = DockStyle.Fill }, 0, 4);
        Setup(_timers, [("Timer", 205), ("Category", 120), ("State", 115), ("Remaining / maximum", 170), ("Sticky charges", 290), ("Details", 345)]);
        _timers.CellValueNeeded += (_, e) =>
        {
            if (e.RowIndex >= _timerRows.Count) return;
            var row = _timerRows[e.RowIndex];
            e.Value = e.ColumnIndex switch { 0 => row.Label, 1 => row.Group, 2 => row.State == "not_acquired" ? "Not acquired" : row.State,
                3 => Countdown(row), 4 => row.Key == "sticky_bomb" ? string.Join("   ", _stickyCharges.Select((charge, index) => $"{index + 1}: {Countdown(charge)}")) : "",
                5 => row.Detail, _ => "" };
        };
        layout.Controls.Add(_timers, 0, 5);
        layout.Controls.Add(new UiText { Text = "Speedrun techniques", Dock = DockStyle.Fill }, 0, 6);
        Setup(_techs, [("Technique", 235), ("State", 125), ("Details", 595)]);
        _techs.CellValueNeeded += (_, e) =>
        {
            if (e.RowIndex >= _techRows.Count) return;
            var row = _techRows[e.RowIndex]; e.Value = e.ColumnIndex switch { 0 => row.Label, 1 => row.State, 2 => row.Detail, _ => "" };
        };
        layout.Controls.Add(_techs, 0, 7);
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
        _settings.ShowGameTimersHud = _timersHud.Checked; _settings.ShowSpeedrunTechsHud = _techsHud.Checked; _save();
    }
    private async Task RefreshSnapshot()
    {
        if (_busy || IsDisposed) return; _busy = true;
        try
        {
            _syncing = true; _enemiesHud.Checked = _settings.ShowEnemyHealthHud; _bars.Checked = _settings.ShowEnemyHealthBars;
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
            if (!IsDisposed) { _enemyRows = []; _timerRows = []; _techRows = []; _enemies.RowCount = 0; _timers.RowCount = 0; _techs.RowCount = 0; _status.Text = "Waiting for a readable game snapshot."; }
        }
        finally { _busy = false; }
    }
    protected override void Dispose(bool disposing) { if (disposing) _refresh.Dispose(); base.Dispose(disposing); }
}
