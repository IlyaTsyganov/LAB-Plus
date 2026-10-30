using LabPlus.Game;
using LabPlus.Configuration;

namespace LabPlus;

internal sealed class ActiveEncountersForm : LabPlusForm
{
    private readonly string _directory;
    private readonly Func<int> _gameProcess;
    private readonly LabPlusSettings _settings;
    private readonly Action _save;
    private readonly UiToggle _hud = new() { Text = "Show in-game HUD", AutoSize = true, Margin = new Padding(10, 4, 0, 0) };
    private readonly UiInput _search = new() { Width = 300, PlaceholderText = "Search encounter or manager…" };
    private readonly UiToggle _inactive = new() { Text = "Show inactive managers", AutoSize = true, Margin = new Padding(10, 4, 0, 0) };
    private readonly UiText _status = new() { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft };
    private readonly UiTable _grid = new();
    private readonly System.Windows.Forms.Timer _timer = new() { Interval = 250 };
    private ActiveEncounterSnapshot? _snapshot;
    private List<ActiveEncounter> _visible = [];
    private bool _busy;
    private bool _syncingHud;

    public ActiveEncountersForm(LabPlusSettings settings, string directory, Action save, Func<int> gameProcess)
    {
        _settings = settings; _save = save;
        _directory = directory; _gameProcess = gameProcess;
        Text = "LAB Plus — Active encounters";
        ClientSize = new Size(1190, 570); MinimumSize = new Size(900, 420);
        StartPosition = FormStartPosition.CenterParent; ShowInTaskbar = false;
        BackColor = Color.FromArgb(35, 35, 35); ForeColor = Color.Gainsboro; Font = LabPlusTheme.Ui(10f);
        AutoScaleMode = AutoScaleMode.Font;
        var layout = new UiGrid { Dock = DockStyle.Fill, Padding = new Padding(12), ColumnCount = 1, RowCount = 4 };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 52));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 50));
        Controls.Add(layout);
        var filters = new UiToolbar { Dock = DockStyle.Fill, WrapContents = false, Margin = Padding.Empty };
        filters.Controls.Add(_search); filters.Controls.Add(_inactive);
        _hud.Checked = settings.ShowActiveEncountersHud;
        _hud.CheckedChanged += (_, _) => { if (!_syncingHud) { _settings.ShowActiveEncountersHud = _hud.Checked; _save(); } };
        filters.Controls.Add(_hud);
        filters.Controls.Add(new UiText { Text = "Updates automatically", AutoSize = true, Margin = new Padding(16, 5, 0, 0), ForeColor = Color.Silver });
        layout.Controls.Add(filters, 0, 0); layout.Controls.Add(_status, 0, 1);
        _grid.Dock = DockStyle.Fill; _grid.ReadOnly = true; _grid.VirtualMode = true;
        _grid.AllowUserToAddRows = _grid.AllowUserToDeleteRows = false; _grid.RowHeadersVisible = false;
        _grid.SelectionMode = DataGridViewSelectionMode.FullRowSelect; _grid.MultiSelect = false;
        _grid.BackgroundColor = Color.FromArgb(45, 45, 45);
        _grid.DefaultCellStyle.BackColor = Color.FromArgb(45, 45, 45); _grid.DefaultCellStyle.ForeColor = ForeColor;
        _grid.DefaultCellStyle.SelectionBackColor = Color.FromArgb(80, 50, 80);
        foreach (var column in new[] { ("Encounter (reference)", 330), ("State", 145), ("Script", 65), ("Commands", 90), ("Technical manager", 390), ("ID / gen", 105) })
            _grid.Columns.Add(new UiTableColumn { HeaderText = column.Item1, Width = column.Item2, SortMode = DataGridViewColumnSortMode.NotSortable });
        _grid.CellValueNeeded += (_, e) =>
        {
            if (e.RowIndex >= _visible.Count) return;
            var row = _visible[e.RowIndex];
            e.Value = e.ColumnIndex switch { 0 => row.DisplayName, 1 => row.State, 2 => row.Script < 0 ? "—" : row.Script.ToString(),
                3 => row.Commands < 0 ? "—" : row.Commands.ToString(), 4 => row.Manager.Length == 0 ? "<name unavailable>" : row.Manager,
                5 => $"{row.Id} / {row.Generation}", _ => "" };
        };
        _grid.CellToolTipTextNeeded += (_, e) =>
        {
            if (e.RowIndex >= 0 && e.RowIndex < _visible.Count) e.ToolTipText = _visible[e.RowIndex].DisplayName + "\n" + _visible[e.RowIndex].Manager;
        };
        layout.Controls.Add(_grid, 0, 2);
        layout.Controls.Add(new UiText { Text = "Active follows meathook's manager-update criterion. It does not by itself prove a combat encounter is in progress.\nConsole: labplus_encounters_list · HUD cvar: labplus_encounters_show 0/1. Unknown reference names remain technical.",
            Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft, Font = new Font(Font.FontFamily, 9f) }, 0, 3);
        _search.TextChanged += (_, _) => Filter(); _inactive.CheckedChanged += (_, _) => Filter();
        _timer.Tick += async (_, _) => await RefreshSnapshot();
        Shown += async (_, _) => { _timer.Start(); await RefreshSnapshot(); };
    }

    private async Task RefreshSnapshot()
    {
        if (_busy || IsDisposed) return; _busy = true;
        try
        {
            _syncingHud = true; _hud.Checked = _settings.ShowActiveEncountersHud; _syncingHud = false;
            int process = _gameProcess();
            if (process <= 0) { _snapshot = null; Filter(); _status.Text = "The selected game is not running. Launch it through LAB Plus."; return; }
            var snapshot = await Task.Run(() => ActiveEncounterSnapshot.Read(_directory, process));
            if (IsDisposed) return;
            if (_gameProcess() != process) { _snapshot = null; Filter(); _status.Text = "Game process changed; waiting for a fresh snapshot…"; return; }
            _snapshot = snapshot; Filter();
            if (snapshot is null) { _status.Text = "Waiting for the viewer layer and compatible meathook. Enable Collision Viewer, then restart the game through LAB Plus."; return; }
            bool fresh = snapshot.IsFresh(DateTimeOffset.UtcNow);
            _grid.DefaultCellStyle.ForeColor = fresh ? Color.Gainsboro : Color.Gray;
            _status.ForeColor = fresh ? Color.Gainsboro : Color.FromArgb(245, 190, 90);
            string phase = snapshot.Phase switch {
                "live" => "Live", "paused" => "Game paused / menu — current manager values", "loading" => "Loading / no active level",
                "waiting_meathook" => "Waiting for compatible meathook", "waiting_callback" => "Waiting for meathook game-frame callback",
                "metadata_unavailable" => "Encounter state metadata unavailable", "unsupported_profile" => "Unsupported game executable",
                "read_error" => "Manager list could not be read", _ => "Snapshot unavailable"
            };
            if (!fresh) phase = "No fresh data — last snapshot shown";
            int active = snapshot.Rows.Count(row => row.Active);
            _status.Text = $"{phase} · {active} active / {snapshot.Rows.Count} managers · {snapshot.Unreadable} unavailable reads\n{snapshot.Map}";
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        {
            if (!IsDisposed) { _snapshot = null; Filter(); _status.Text = "Waiting for a readable encounter snapshot…"; }
        }
        finally { _busy = false; }
    }

    private void Filter()
    {
        _visible = _snapshot?.Select(_inactive.Checked, _search.Text).ToList() ?? [];
        int first = _grid.FirstDisplayedScrollingRowIndex;
        _grid.RowCount = _visible.Count; _grid.Invalidate();
        if (first >= 0 && first < _grid.RowCount) _grid.FirstDisplayedScrollingRowIndex = first;
    }
    protected override void Dispose(bool disposing) { if (disposing) _timer.Dispose(); base.Dispose(disposing); }
}
