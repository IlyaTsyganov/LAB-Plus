using System.Text.Json;
using LabPlus.Game;

namespace LabPlus;

internal sealed class PracticeSaveForm : LabPlusForm
{
    private readonly string _directory;
    private readonly Func<int> _gameProcess;
    private readonly PracticeSaveVault _vault;
    private readonly Action<PracticeSave>? _link;
    private readonly ResearchBookmark? _context;
    private readonly UiInput _name = new() { Dock = DockStyle.Fill, MaxLength = 160, PlaceholderText = "Checkpoint name — optional" };
    private readonly UiInput _search = new() { Dock = DockStyle.Fill, PlaceholderText = "Search checkpoints…" };
    private readonly UiTable _grid = new();
    private readonly UiAction _save = MakeButton("Save", 150);
    private readonly UiAction _load = MakeButton("Load", 170);
    private readonly UiAction _settings = MakeButton("Advanced…", 170, false);
    private readonly UiText _selection = new() { Dock = DockStyle.Fill, AutoEllipsis = true, TextAlign = ContentAlignment.MiddleLeft, UseMnemonic = false };
    private readonly UiText _status = new() { Dock = DockStyle.Fill, AutoEllipsis = true, TextAlign = ContentAlignment.MiddleLeft, UseMnemonic = false };
    private readonly UiText _connection = new() { Dock = DockStyle.Fill, AutoEllipsis = true, UseMnemonic = false };
    private string _emptyText = "";
    private readonly System.Windows.Forms.Timer _timer = new() { Interval = 1000 };
    private IReadOnlyList<PracticeSave> _all = [];
    private List<PracticeSave> _visible = [];
    private string _remote = "", _details = "";
    private bool _busy, _populating;
    private int _lastProcess;

    public PracticeSaveForm(string directory, Action<PracticeSave>? link = null, PracticeSaveVault? vault = null, ResearchBookmark? context = null, Func<int>? gameProcess = null)
    {
        _directory = directory; _gameProcess = gameProcess ?? (() => 0); _vault = vault ?? new(directory); _link = link; _context = context;
        Text = "LAB Plus — Checkpoints"; ClientSize = new(860, 620); MinimumSize = new(760, 550); StartPosition = FormStartPosition.CenterParent; ShowInTaskbar = false;
        AutoScaleMode = AutoScaleMode.Font; BackColor = Color.FromArgb(35, 35, 35); ForeColor = Color.Gainsboro; Font = LabPlusTheme.Ui(10f); KeyPreview = true;
        var root = new UiGrid { Dock = DockStyle.Fill, Padding = new Padding(16), ColumnCount = 1, RowCount = 8 };
        foreach (float height in new[] { 92f, 42f, 42f }) root.RowStyles.Add(new(SizeType.Absolute, height));
        root.RowStyles.Add(new(SizeType.Percent, 100));
        foreach (float height in new[] { 30f, 44f, 44f, 42f }) root.RowStyles.Add(new(SizeType.Absolute, height)); Controls.Add(root);
        var intro = new UiGrid { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 2, Margin = Padding.Empty };
        intro.RowStyles.Add(new(SizeType.Absolute, 40)); intro.RowStyles.Add(new(SizeType.Percent, 100));
        intro.Controls.Add(new UiText { Text = "Checkpoints", Font = new(Font.FontFamily, 18f, FontStyle.Bold), Dock = DockStyle.Fill }, 0, 0);
        intro.Controls.Add(new UiText { Text = "Save a checkpoint, select it in the list and press Load.\nThe latest completed game autosave is stored.", Dock = DockStyle.Fill, Font = new(Font.FontFamily, 9.5f), ForeColor = Color.Silver }, 0, 1); root.Controls.Add(intro, 0, 0);
        void Bar(int row, UiNode fill, UiNode button)
        {
            var bar = new UiGrid { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Margin = Padding.Empty };
            bar.ColumnStyles.Add(new(SizeType.Percent, 100)); bar.ColumnStyles.Add(new(SizeType.Absolute, button.Width + 12));
            fill.Margin = new(0, 4, 12, 4); button.Anchor = AnchorStyles.Right | AnchorStyles.Top; button.Margin = new(0, 2, 0, 0);
            bar.Controls.Add(fill, 0, 0); bar.Controls.Add(button, 1, 0); root.Controls.Add(bar, 0, row);
        }
        Bar(1, _name, _save); Bar(2, _search, _settings);
        foreach (var field in new[] { _name, _search }) { field.BackColor = Color.FromArgb(55, 55, 55); field.ForeColor = ForeColor; field.BorderStyle = BorderStyle.FixedSingle; }
        var list = new UiStack { Dock = DockStyle.Fill, Margin = new(0, 5, 0, 0) }; root.Controls.Add(list, 0, 3);
        _grid.Dock = DockStyle.Fill; _grid.ReadOnly = true; _grid.AllowUserToAddRows = _grid.AllowUserToDeleteRows = false; _grid.MultiSelect = false;
        _grid.RowHeadersVisible = false; _grid.SelectionMode = DataGridViewSelectionMode.FullRowSelect; _grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
        _grid.BackgroundColor = _grid.DefaultCellStyle.BackColor = Color.FromArgb(45, 45, 45); _grid.DefaultCellStyle.ForeColor = ForeColor; _grid.DefaultCellStyle.SelectionBackColor = Color.FromArgb(110, 40, 40);
        _grid.EnableHeadersVisualStyles = false; _grid.ColumnHeadersDefaultCellStyle.BackColor = Color.FromArgb(60, 60, 60); _grid.ColumnHeadersDefaultCellStyle.ForeColor = ForeColor;
        _grid.ColumnHeadersDefaultCellStyle.SelectionBackColor = _grid.ColumnHeadersDefaultCellStyle.BackColor; _grid.ColumnHeadersDefaultCellStyle.SelectionForeColor = ForeColor;
        _grid.RowTemplate.Height = 32; _grid.ColumnHeadersHeight = 32;
        _grid.Columns.Add(new UiTableColumn { HeaderText = "Checkpoint", FillWeight = 72, MinimumWidth = 230, SortMode = DataGridViewColumnSortMode.NotSortable });
        _grid.Columns.Add(new UiTableColumn { HeaderText = "Created", FillWeight = 28, MinimumWidth = 180, SortMode = DataGridViewColumnSortMode.NotSortable });
        list.Controls.Add(_grid); root.Controls.Add(_selection, 0, 4);
        var actions = new UiGrid { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Margin = Padding.Empty };
        actions.ColumnStyles.Add(new(SizeType.Percent, 100)); actions.ColumnStyles.Add(new(SizeType.Absolute, _load.Width + 12));
        actions.Controls.Add(new UiText { Text = "Load the selected checkpoint directly in game.", AutoEllipsis = true, Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft, Font = new(Font.FontFamily, 9f), ForeColor = Color.Silver }, 0, 0);
        _load.Anchor = AnchorStyles.Right | AnchorStyles.Top; _load.Margin = new(0, 4, 0, 0); actions.Controls.Add(_load, 1, 0); root.Controls.Add(actions, 0, 5);
        root.Controls.Add(_status, 0, 6); _connection.Font = new(Font.FontFamily, 9f); _connection.ForeColor = Color.Silver; root.Controls.Add(_connection, 0, 7);
        if (context is not null) _name.Text = context.Name;
        _save.Click += (_, _) => _ = SaveCheckpoint(); _load.Click += (_, _) => _ = LoadCheckpoint(); _settings.Click += (_, _) => OpenSettings();
        _search.TextChanged += (_, _) => Populate(); _grid.SelectionChanged += (_, _) => { if (!_populating) RefreshState(); };
        _grid.CellDoubleClick += (_, e) => { if (e.RowIndex >= 0) _load.PerformClick(); };
        _name.KeyDown += (_, e) => { if (e.KeyCode == Keys.Enter) { e.SuppressKeyPress = true; _save.PerformClick(); } };
        _grid.KeyDown += (_, e) => { if (e.KeyCode == Keys.Enter) { e.SuppressKeyPress = true; _load.PerformClick(); } };
        KeyDown += (_, e) => { if (e.Control && e.KeyCode == Keys.S) { e.SuppressKeyPress = true; _save.PerformClick(); } };
        _lastProcess = _gameProcess(); Discover(); Reload(); _status.Text = "Pause the game before saving or loading.";
        _timer.Tick += (_, _) => { int process = _gameProcess(); if (!_busy && process != _lastProcess) { _lastProcess = process; Discover(); Populate(); } RefreshState(); };
        _timer.Start(); FormClosed += (_, _) => { _timer.Stop(); _timer.Dispose(); };
    }
    private static UiAction MakeButton(string text, int width, bool primary = true) => new WorkspaceButton()
    {
        Primary = primary, Text = text, Size = new(width, 32), FlatStyle = FlatStyle.Flat, BackColor = LabPlusTheme.Canvas, ForeColor = LabPlusTheme.Text
    };
    private PracticeSave? Selected => _grid.CurrentCell is { RowIndex: >= 0 } cell && cell.RowIndex < _visible.Count ? _visible[cell.RowIndex] : null;
    private SteamSaveLocation? SaveLocation()
    {
        try { return SteamSaveLocation.Parse(_remote); } catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { return null; }
    }
    private string PreferencePath => Path.Combine(_directory, "practice-ui.json");
    private sealed record Preferences(string Remote);
    private void Discover()
    {
        string? preferred = null;
        try { PracticeSaveVault.NoLinks(PreferencePath); if (File.Exists(PreferencePath) && new FileInfo(PreferencePath).Length < 4096) preferred = JsonSerializer.Deserialize<Preferences>(File.ReadAllText(PreferencePath))?.Remote; }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException) { }
        string? account = PracticeSaveSource.RuntimeAccount(_directory, _gameProcess()) ?? PracticeSaveSource.ActiveAccount();
        var paths = SteamSaveLocation.Discover();
        if (preferred is not null)
        {
            try { var previous = SteamSaveLocation.Parse(preferred); if (account is null || previous.Account == account) { _remote = previous.Remote; return; } }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
        }
        _remote = PracticeSaveSource.Choose(paths, account, null) ?? "";
    }
    private void UseSource(string remote)
    {
        var location = SteamSaveLocation.Parse(remote); _remote = location.Remote;
        Directory.CreateDirectory(_directory); PracticeSaveVault.NoLinks(_directory); PracticeSaveVault.NoLinks(PreferencePath);
        File.WriteAllText(PreferencePath, JsonSerializer.Serialize(new Preferences(_remote))); Populate();
    }
    private void Reload(string? id = null) { _all = _vault.List(); Populate(id); }
    private void Populate(string? id = null)
    {
        id ??= Selected?.Id; var location = SaveLocation(); string query = _search.Text.Trim();
        _visible = _all.Where(s => PracticeSaveSource.SupportsLive(s) && location?.Account == s.Account && $"{s.Name} {s.Category} {s.Map}".Contains(query, StringComparison.OrdinalIgnoreCase)).OrderByDescending(s => s.Captured).ToList();
        _populating = true; _grid.Rows.Clear();
        foreach (var save in _visible) _grid.Rows.Add(save.Name, DateTimeOffset.FromUnixTimeMilliseconds(save.Captured).ToLocalTime().ToString("dd.MM.yyyy HH:mm"));
        int index = _visible.FindIndex(s => s.Id == id); if (index >= 0) _grid.CurrentCell = _grid.Rows[index].Cells[0];
        _emptyText = query.Length == 0 ? "No checkpoints for this account and practice slot yet.\nPress Save to add the first checkpoint." : "No checkpoints match your search.";
        _grid.Invalidate();
        _populating = false; RefreshState();
    }
    private void RefreshState()
    {
        bool running = _gameProcess() > 0, source = SaveLocation() is not null;
        _save.Enabled = !_busy && running && source; _load.Enabled = !_busy && running && source && Selected is not null; _settings.Enabled = !_busy;
        _grid.Enabled = _search.Enabled = _name.Enabled = !_busy;
        _selection.Text = Selected is { } selected ? "Selected: " + selected.Name : "Select a checkpoint in the list.";
        _connection.Text = !source ? "Steam save folder could not be selected automatically. Open Advanced."
            : !running ? "Launch the game through LAB Plus, then open TAG1, slot 3."
            : "TAG1 · slot 3 · DOOM Eternal 6.66 Rev 3. Steam Cloud is checked automatically.";
    }
    private async Task Run(Func<Task> action)
    {
        if (_busy) return; _busy = true; _status.ForeColor = ForeColor; RefreshState();
        try { await action(); }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or System.Security.Cryptography.CryptographicException or InvalidOperationException or System.ComponentModel.Win32Exception)
        { if (!IsDisposed) { _details = ex.Message; _status.ForeColor = Color.Salmon; _status.Text = FriendlyError(ex.Message); } }
        finally { _busy = false; if (!IsDisposed) RefreshState(); }
    }
    internal static string FriendlyError(string text)
    {
        string message = text.Split("Steam Cloud flags:", StringSplitOptions.None)[0];
        if (message.Contains("rollback failed", StringComparison.OrdinalIgnoreCase)) return "Original files could not be restored. Close the game and Steam, then open details for the original backup path.";
        if (message.Contains("Files installed", StringComparison.OrdinalIgnoreCase)) return "Files were installed, but loading did not start. Open details before the next operation.";
        if (message.Contains("completion", StringComparison.OrdinalIgnoreCase) || message.Contains("not observed", StringComparison.OrdinalIgnoreCase)) return "Loading is not confirmed yet. Return to the game and press Space at 100% loading. Details have been retained.";
        if (message.Contains("authentication", StringComparison.OrdinalIgnoreCase)) return "Files are damaged or belong to another account. Open details.";
        if (message.Contains("Cloud", StringComparison.OrdinalIgnoreCase)) return "Steam Cloud must be disabled for this operation. Check the Steam setting and retry.";
        if (message.Contains("Pause", StringComparison.OrdinalIgnoreCase)) return "Pause the game and retry.";
        if (message.Contains("account", StringComparison.OrdinalIgnoreCase) || message.Contains("owner", StringComparison.OrdinalIgnoreCase)) return "The save account does not match the game account. Check the selected Steam folder in settings.";
        if (message.Contains("slot", StringComparison.OrdinalIgnoreCase) || message.Contains("campaign", StringComparison.OrdinalIgnoreCase) || message.Contains("profile", StringComparison.OrdinalIgnoreCase)) return "This operation supports DOOM Eternal 6.66 Rev 3, TAG1, slot 3. Open that slot and pause.";
        if (message.Contains("No response", StringComparison.OrdinalIgnoreCase)) return "The game did not respond. Make sure it was launched through this LAB Plus build.";
        if (message.Contains("busy", StringComparison.OrdinalIgnoreCase) || message.Contains("writing", StringComparison.OrdinalIgnoreCase)) return "The game is still saving. Wait for the autosave to complete and retry.";
        return message;
    }
    private Task SaveCheckpoint() => Run(async () =>
    {
        string remote = SaveLocation()?.Remote ?? throw new IOException("Select the Steam folder under Advanced."), name = _name.Text.Trim();
        if (_gameProcess() <= 0) throw new IOException("Launch the game through LAB Plus first.");
        if (name.Length == 0) name = "Checkpoint — " + DateTime.Now.ToString("dd.MM HH:mm:ss");
        _status.Text = "Saving checkpoint…";
        var capture = await LivePracticeBridge.Execute(_directory, _gameProcess, "archive", remote, PracticeSaveSource.LiveSlot, true);
        var saved = await Task.Run(() => _vault.ImportLiveCheckpoint(_directory, capture, remote, name, "My checkpoints"));
        if (IsDisposed) return; _search.Clear(); Reload(saved.Id); _details = capture.Detail; _status.ForeColor = Color.LightGreen; _status.Text = "Saved: " + saved.Name;
    });
    private Task LoadCheckpoint() => Run(async () =>
    {
        var save = Selected ?? throw new IOException("Select a checkpoint in the list.");
        string remote = SaveLocation()?.Remote ?? throw new IOException("Select the Steam folder under Advanced."); int process = _gameProcess();
        if (process <= 0) throw new IOException("Launch the game through LAB Plus first.");
        _status.Text = "Preparing checkpoint load…";
        var plan = await Task.Run(() => _vault.PlanLiveRestore(process, save.Id, remote, PracticeSaveSource.LiveSlot, true));
        await Task.Run(() => _vault.StageLiveRestore(_directory, plan)); if (IsDisposed) return;
        _details = plan.Description;
        var result = await LivePracticeBridge.ExecuteRestore(_directory, _gameProcess, plan, true, value =>
        { if (!IsDisposed && value.Phase == "loading") { _details = value.Detail; _status.Text = "Return to the game. Press Space when loading reaches 100%."; } });
        if (IsDisposed) return; _details = result.Detail; _status.ForeColor = Color.LightGreen; _status.Text = "Loaded: " + save.Name + ". Return to the game.";
    });
    private void OpenSettings()
    {
        using var dialog = new LabPlusForm { Text = "Checkpoint settings", ClientSize = new(710, 360), MinimumSize = new(650, 360), StartPosition = FormStartPosition.CenterParent, ShowInTaskbar = false, BackColor = BackColor, ForeColor = ForeColor, Font = Font, AutoScaleMode = AutoScaleMode.Font };
        var panel = new UiGrid { Dock = DockStyle.Fill, Padding = new(16), ColumnCount = 1, RowCount = 6 };
        foreach (float height in new[] { 40f, 38f, 44f, 42f }) panel.RowStyles.Add(new(SizeType.Absolute, height)); panel.RowStyles.Add(new(SizeType.Percent, 100)); panel.RowStyles.Add(new(SizeType.Absolute, 40)); dialog.Controls.Add(panel);
        panel.Controls.Add(new UiText { Text = "Steam save folder", Dock = DockStyle.Fill, Font = new(Font, FontStyle.Bold) }, 0, 0);
        var folders = new UiSelect { Dock = DockStyle.Fill, DropDownStyle = ComboBoxStyle.DropDown };
        foreach (string path in SteamSaveLocation.Discover()) folders.Items.Add(path); folders.Text = _remote; panel.Controls.Add(folders, 0, 1);
        var choose = MakeButton("Choose folder…", 170, false); choose.Click += (_, _) => { using var picker = new FolderBrowserDialog { Description = "Steam/userdata/<account>/782330/remote" }; if (picker.ShowDialog(dialog) == DialogResult.OK) folders.Text = picker.SelectedPath; }; panel.Controls.Add(choose, 0, 2);
        var tools = MakeButton("Tools and diagnostics…", 300, false); panel.Controls.Add(tools, 0, 3);
        var note = new UiInput { Dock = DockStyle.Fill, Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Text = _details.Length == 0 ? "The folder is normally selected automatically. File validation, offline archives, import and recovery are available in Tools." : _details }; panel.Controls.Add(note, 0, 4);
        var done = MakeButton("Done", 140); panel.Controls.Add(done, 0, 5);
        bool Apply()
        {
            try { UseSource(folders.Text); return true; }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { note.Text = "Select Steam/userdata/<account>/782330/remote.\n" + ex.Message; return false; }
        }
        done.Click += (_, _) => { if (Apply()) dialog.Close(); };
        tools.Click += (_, _) =>
        {
            using var advanced = new PracticeSaveAdvancedForm(_directory, _link, _vault, _context, _gameProcess) { SelectedRemote = folders.Text, SelectedSlot = PracticeSaveSource.LiveSlot };
            advanced.ShowDialog(dialog); folders.Text = advanced.SelectedRemote; Reload();
        };
        dialog.ShowDialog(this);
    }
}
