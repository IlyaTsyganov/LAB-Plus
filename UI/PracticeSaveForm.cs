using System.Text.Json;
using LabPlus.Game;

namespace LabPlus;

internal sealed class PracticeSaveForm : LabPlusForm
{
    private const int ActionWidth = 170;
    private readonly string _directory;
    private readonly Func<int> _gameProcess;
    private readonly PracticeSaveVault _vault;
    private readonly PracticeSaveLibrary _library;
    private readonly Action<PracticeSave>? _link;
    private readonly ResearchBookmark? _context;
    private readonly UiInput _name = new() { Dock = DockStyle.Fill, MaxLength = 160, PlaceholderText = "Checkpoint name — optional" };
    private readonly UiInput _search = new() { Dock = DockStyle.Fill, PlaceholderText = "Search checkpoints…" };
    private readonly UiInput _folder = new() { Dock = DockStyle.Fill, ReadOnly = true, PlaceholderText = "Select the Steam save folder" };
    private readonly UiInput _newCategory = new() { Dock = DockStyle.Fill, MaxLength = 160, PlaceholderText = "New category" };
    private readonly UiTable _grid = new() { MultiSelect = true, ShowSelectionChecks = true, Presentation = "saves-table" };
    private readonly UiAction _save = MakeButton("Save", ActionWidth), _load = MakeButton("Load", ActionWidth), _delete = MakeButton("Delete", ActionWidth);
    private readonly UiAction _up = MakeButton("Up", 62, false), _down = MakeButton("Down", 62, false), _categories = MakeButton("Categories", ActionWidth, false);
    private readonly UiAction _chooseFolder = MakeButton("Choose Steam save folder", 42, false), _tools = MakeButton("Tools and diagnostics…", 220, false);
    private readonly UiStack _categoryMenu = new() { Dock = DockStyle.Fill, Visible = false, Presentation = "saves-category-menu" };
    private readonly UiStack _categoryButtons = new() { Dock = DockStyle.Fill };
    private readonly UiAction _moveCategory = MakeButton("Move to…", 110, false);
    private readonly UiText _categoryMenuTitle = new() { Text = "Categories", Dock = DockStyle.Fill };
    private bool _movingCategory;
    private readonly UiText _categoryHeading = new() { Dock = DockStyle.Fill, Presentation = "saves-category-heading" };
    private readonly UiText _selection = new() { Dock = DockStyle.Fill, UseMnemonic = false }, _status = new() { Dock = DockStyle.Fill, UseMnemonic = false };
    private readonly UiText _connection = new() { Dock = DockStyle.Fill, UseMnemonic = false, Presentation = "saves-destination" };
    private string _categorySignature = "";
    private readonly System.Windows.Forms.Timer _timer = new() { Interval = 1000 };
    private IReadOnlyList<PracticeSave> _all = [];
    private List<PracticeSave> _visible = [];
    private string _remote = "", _details = "", _slot = "", _runtimeMessage = "Load a campaign slot in the game first.";
    private bool _busy, _populating, _runtimeReady, _runtimePolling = true;
    private int _lastProcess, _buildProcess = -1, _sourceGeneration;
    private long _nextRuntimeRefresh;
    private Task? _runtimeRefresh;
    private string? _runningBuild;
    private string? RunningBuild()
    {
        int process = _gameProcess();
        if (process != _buildProcess) { _buildProcess = process; _runningBuild = PracticeSaveSource.RuntimeBuild(process); }
        return _runningBuild;
    }
    public PracticeSaveForm(string directory, Action<PracticeSave>? link = null, PracticeSaveVault? vault = null, ResearchBookmark? context = null, Func<int>? gameProcess = null)
    {
        _directory = directory; _gameProcess = gameProcess ?? (() => 0); _vault = vault ?? new(directory); _library = new(directory); _link = link; _context = context;
        Text = "LAB Plus — Checkpoints"; ClientSize = new(900, 780); MinimumSize = new(720, 570); StartPosition = FormStartPosition.CenterParent; ShowInTaskbar = false;
        AutoScaleMode = AutoScaleMode.Font; BackColor = LabPlusTheme.Canvas; ForeColor = LabPlusTheme.Text; Font = LabPlusTheme.Ui(10f); KeyPreview = true;
        var root = new UiGrid { Dock = DockStyle.Fill, Padding = new(16), ColumnCount = 1, RowCount = 11, Presentation = "saves-layout" }; Controls.Add(root);
        root.Controls.Add(new UiText { Text = "Archive the latest completed checkpoint, including before entering a loaded level.", Dock = DockStyle.Fill, Font = LabPlusTheme.Ui(9.5f) }, 0, 0);
        UiGrid Bar(UiNode fill, UiNode button, string presentation)
        {
            var bar = new UiGrid { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Margin = Padding.Empty, Presentation = presentation };
            bar.ColumnStyles.Add(new(SizeType.Percent, 100)); bar.ColumnStyles.Add(new(SizeType.Absolute, button.Width)); fill.Dock = button.Dock = DockStyle.Fill;
            bar.Controls.Add(fill, 0, 0); bar.Controls.Add(button, 1, 0); return bar;
        }
        root.Controls.Add(Bar(_name, _save, "saves-create"), 0, 1);
        var anchor = new UiStack { Dock = DockStyle.Fill, Width = ActionWidth, Presentation = "saves-category-anchor" };
        anchor.Controls.Add(_categories); anchor.Controls.Add(_categoryMenu);
        _categoryMenu.Controls.Add(_categoryMenuTitle); _categoryMenu.Controls.Add(_categoryButtons);
        var add = MakeButton("+", 38, false); _categoryMenu.Controls.Add(Bar(_newCategory, add, "saves-add-category"));
        root.Controls.Add(Bar(_search, anchor, "saves-filters"), 0, 2); root.Controls.Add(_categoryHeading, 0, 3);
        _grid.Dock = DockStyle.Fill; _grid.ReadOnly = true; _grid.AllowUserToAddRows = _grid.AllowUserToDeleteRows = false; _grid.RowHeadersVisible = false;
        _grid.SelectionMode = DataGridViewSelectionMode.FullRowSelect; _grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
        foreach (var title in new[] { "Checkpoint", "Campaign", "Created" }) _grid.Columns.Add(new UiTableColumn { HeaderText = title, SortMode = DataGridViewColumnSortMode.NotSortable });
        root.Controls.Add(_grid, 0, 4); root.Controls.Add(_selection, 0, 5);
        var actions = new UiGrid { Dock = DockStyle.Fill, ColumnCount = 3, RowCount = 1, Presentation = "saves-actions" };
        actions.ColumnStyles.Add(new(SizeType.Percent, 100)); actions.ColumnStyles.Add(new(SizeType.Absolute, ActionWidth)); actions.ColumnStyles.Add(new(SizeType.Absolute, ActionWidth));
        var order = new UiToolbar { Dock = DockStyle.Fill, Presentation = "saves-order" }; order.Controls.Add(_up); order.Controls.Add(_down); order.Controls.Add(_moveCategory);
        actions.Controls.Add(order, 0, 0); actions.Controls.Add(_delete, 1, 0); actions.Controls.Add(_load, 2, 0); root.Controls.Add(actions, 0, 6);
        root.Controls.Add(_connection, 0, 7); root.Controls.Add(_status, 0, 8);
        var folderArea = new UiStack { Dock = DockStyle.Fill, Presentation = "saves-folder-area" };
        folderArea.Controls.Add(new UiText { Text = "Steam save folder", Dock = DockStyle.Fill }); _chooseFolder.Icon = "folder";
        folderArea.Controls.Add(Bar(_folder, _chooseFolder, "saves-folder-row")); root.Controls.Add(folderArea, 0, 9);
        // Keep the advanced implementation available internally, outside Saves.
        _tools.Visible = false; root.Controls.Add(_tools, 0, 10); _tools.Click += (_, _) => OpenTools();
        if (context is not null) _name.Text = context.Name;
        _save.Click += (_, _) => _ = SaveCheckpoint(); _load.Click += (_, _) => _ = LoadCheckpoint(); _delete.Click += (_, _) => _ = DeleteSelected();
        _up.Click += (_, _) => _ = MoveSelected(-1); _down.Click += (_, _) => _ = MoveSelected(1);
        _categories.Click += (_, _) => OpenCategories(false); _moveCategory.Click += (_, _) => OpenCategories(true);
        add.Click += (_, _) => _ = Run(() =>
        {
            var selected = SelectedIds; string category = _newCategory.Text.Trim(); _library.Add(category);
            if (_movingCategory) _library.Transfer(_all, selected, category);
            _newCategory.Clear(); _categoryMenu.Visible = false; Reload(); return Task.CompletedTask;
        });
        _chooseFolder.Click += (_, _) => BrowseFolder(); _search.TextChanged += (_, _) => Populate();
        _grid.SelectionChanged += (_, _) => { if (!_populating) { _categoryMenu.Visible = false; RefreshState(); } };
        _grid.CellDoubleClick += (_, e) => { if (e.RowIndex >= 0) _load.PerformClick(); };
        _name.KeyDown += (_, e) => { if (e.KeyCode == Keys.Enter) { e.SuppressKeyPress = true; _save.PerformClick(); } };
        _grid.KeyDown += (_, e) => { if (e.KeyCode == Keys.Enter) { e.SuppressKeyPress = true; _load.PerformClick(); } };
        KeyDown += (_, e) => { if (e.Control && e.KeyCode == Keys.S) { e.SuppressKeyPress = true; _save.PerformClick(); } };
        _lastProcess = _gameProcess(); Discover(); Reload(); _status.Text = "Save or choose checkpoints.";
        _timer.Tick += (_, _) => Tick(); _timer.Start(); FormClosed += (_, _) => { _timer.Stop(); _timer.Dispose(); };
    }
    private static UiAction MakeButton(string text, int width, bool primary = true) => new WorkspaceButton
    { Primary = primary, Text = text, Size = new(width, primary ? 42 : 38), Presentation = primary ? "saves-primary" : "", BackColor = LabPlusTheme.Canvas, ForeColor = LabPlusTheme.Text };
    private IReadOnlyList<PracticeSave> SelectedArchives => _grid.SelectedIndices.Where(i => i < _visible.Count).Select(i => _visible[i]).ToArray();
    private HashSet<string> SelectedIds => SelectedArchives.Select(s => s.Id).ToHashSet();
    private PracticeSave? Selected => SelectedArchives.Count == 1 ? SelectedArchives[0] : null;
    private SteamSaveLocation? SaveLocation()
    { try { return SteamSaveLocation.Parse(_remote); } catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { return null; } }
    private string PreferencePath => Path.Combine(_directory, "practice-ui.json");
    private sealed record Preferences(string Remote);
    private void Discover()
    {
        string? preferred = null;
        try { PracticeSaveVault.NoLinks(PreferencePath); if (File.Exists(PreferencePath) && new FileInfo(PreferencePath).Length < 4096) preferred = JsonSerializer.Deserialize<Preferences>(File.ReadAllText(PreferencePath))?.Remote; }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException) { }
        string? account = PracticeSaveSource.RuntimeAccount(_directory, _gameProcess()) ?? PracticeSaveSource.ActiveAccount();
        if (preferred is not null)
        {
            try { var previous = SteamSaveLocation.Parse(preferred); if (account is null || previous.Account == account) { _remote = previous.Remote; _folder.Text = _remote; return; } }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
        }
        _remote = PracticeSaveSource.Choose(SteamSaveLocation.Discover(), account, null) ?? ""; _folder.Text = _remote;
    }
    private void UseSource(string remote)
    {
        var location = SteamSaveLocation.Parse(remote); _remote = location.Remote; _folder.Text = _remote; _sourceGeneration++; _runtimeReady = false; _slot = ""; _nextRuntimeRefresh = 0;
        Directory.CreateDirectory(_directory); PracticeSaveVault.NoLinks(PreferencePath); PracticeSaveVault.NoLinks(PreferencePath + ".tmp");
        File.WriteAllText(PreferencePath + ".tmp", JsonSerializer.Serialize(new Preferences(_remote))); File.Move(PreferencePath + ".tmp", PreferencePath, true); Populate();
    }
    private void BrowseFolder()
    {
        using var picker = new FolderBrowserDialog { Description = "Steam/userdata/<account>/782330/remote", SelectedPath = _remote };
        if (picker.ShowDialog(OwnerScreen) == DialogResult.OK) _ = Run(() => { UseSource(picker.SelectedPath); return Task.CompletedTask; });
    }
    private void Tick()
    {
        if (!_runtimePolling) return;
        int process = _gameProcess();
        if (!_busy && process != _lastProcess) { _lastProcess = process; _sourceGeneration++; _runtimeReady = false; _slot = ""; _nextRuntimeRefresh = 0; Discover(); Populate(); }
        if (!_busy && process > 0 && SaveLocation() is not null && Environment.TickCount64 >= _nextRuntimeRefresh && (_runtimeRefresh is null || _runtimeRefresh.IsCompleted))
        { _nextRuntimeRefresh = Environment.TickCount64 + 2000; _runtimeRefresh = RefreshRuntime(); }
        RefreshState();
    }
    internal void SetRuntimePolling(bool active)
    {
        if (_runtimePolling == active) return; _runtimePolling = active;
        if (active) { _runtimeReady = false; _nextRuntimeRefresh = 0; Tick(); }
    }
    private async Task RefreshRuntime()
    {
        int process = _gameProcess(), generation = _sourceGeneration; string remote = _remote;
        try
        {
            var result = await LivePracticeBridge.Execute(_directory, _gameProcess, "status", remote, "*", false);
            if (IsDisposed || process != _gameProcess() || generation != _sourceGeneration) return;
            _slot = result.Slot; _runningBuild = result.Build; _buildProcess = process; _runtimeReady = true; _runtimeMessage = "";
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidOperationException or System.ComponentModel.Win32Exception)
        {
            if (IsDisposed || process != _gameProcess() || generation != _sourceGeneration) return;
            _runtimeReady = false; _runtimeMessage = FriendlyError(ex.Message);
        }
        if (!IsDisposed) { UpdateAvailability(); RefreshState(); }
    }
    private void Reload(string? id = null) { _all = _library.Categorized(_vault.List()); Populate(id is null ? null : new HashSet<string> { id }); }
    private void OpenCategories(bool move)
    {
        bool close = _categoryMenu.Visible && _movingCategory == move; _movingCategory = move;
        _categoryMenuTitle.Text = move ? "Move selected checkpoints to" : "Categories"; RebuildCategories(); _categoryMenu.Visible = !close;
    }
    private Task TransferSelected(string category) => Run(() =>
    {
        var selected = SelectedIds; _library.Transfer(_all, selected, category); _library.Select(category);
        _all = _library.Categorized(_vault.List()); _search.Clear(); Populate(selected);
        _status.Text = $"Moved {selected.Count} checkpoint(s) to {category}."; return Task.CompletedTask;
    });
    private void Populate(HashSet<string>? selected = null)
    {
        selected ??= SelectedIds; var location = SaveLocation(); string query = _search.Text.Trim();
        _visible = _library.Ordered(_all.Where(s => location?.Account == s.Account && (_library.Category == PracticeSaveLibrary.All || s.Category.Equals(_library.Category, StringComparison.OrdinalIgnoreCase)) && $"{s.Name} {s.Category} {s.Map}".Contains(query, StringComparison.OrdinalIgnoreCase))).ToList();
        _populating = true; _grid.Rows.Clear();
        foreach (var save in _visible)
        {
            int row = _grid.Rows.Add(save.Name, PracticeSaveSource.Campaign(save.SourceSlot), DateTimeOffset.FromUnixTimeMilliseconds(save.Captured).ToLocalTime().ToString("dd.MM.yyyy HH:mm"));
            _grid.Rows[row].Subtitle = save.Map;
        }
        _grid.SetSelectedRows(Enumerable.Range(0, _visible.Count).Where(i => selected.Contains(_visible[i].Id))); _populating = false;
        _categoryHeading.Text = $"{_library.Category} · {_visible.Count} checkpoints"; RebuildCategories(); UpdateAvailability(); RefreshState();
    }
    private void RebuildCategories()
    {
        var accountSaves = _all.Where(s => SaveLocation()?.Account == s.Account).ToArray();
        string signature = JsonSerializer.Serialize(new { _movingCategory, _library.Category, categories = _library.Categories(accountSaves), counts = accountSaves.GroupBy(s => s.Category).Select(g => new { g.Key, Count = g.Count() }) });
        if (signature == _categorySignature) return; _categorySignature = signature;
        foreach (var button in _categoryButtons.Controls.ToArray()) button.Dispose(); _categoryButtons.Controls.Clear();
        foreach (string category in (_movingCategory ? Array.Empty<string>() : new[] { PracticeSaveLibrary.All }).Concat(_library.Categories(accountSaves)))
        {
            int count = accountSaves.Count(s => category == PracticeSaveLibrary.All || s.Category.Equals(category, StringComparison.OrdinalIgnoreCase));
            var button = MakeButton($"{category} ({count})", 260, false); button.Dock = DockStyle.Fill;
            button.Presentation = category.Equals(_library.Category, StringComparison.OrdinalIgnoreCase) ? "saves-category-active" : "saves-category-option";
            button.Click += (_, _) => { if (_movingCategory) _ = TransferSelected(category); else _ = Run(() => { _library.Select(category); _categoryMenu.Visible = false; Populate(new HashSet<string>()); return Task.CompletedTask; }); }; _categoryButtons.Controls.Add(button);
        }
    }
    private void UpdateAvailability()
    {
        string? build = RunningBuild();
        for (int i = 0; i < _visible.Count; i++)
        {
            var save = _visible[i]; _grid.Rows[i].Unavailable = build is not null && (_runtimeReady
                ? PracticeSaveSource.LoadBlockReason(save, build, _slot) : PracticeSaveSource.ArchiveBlockReason(save, build)) is not null;
            _grid.Rows[i].Note = build is not null && !PracticeSaveSource.SupportsSlot(build, save.SourceSlot) ? PracticeSaveSource.Campaign(save.SourceSlot) == "TAG1" ? "Requires patch 3.0+" : "Requires patch 5.0+" : "";
        }
    }
    private void RefreshState()
    {
        bool running = _gameProcess() > 0, ready = running && _runtimeReady && SaveLocation() is not null; var selected = SelectedArchives; var one = Selected;
        _save.Enabled = !_busy && ready; _load.Enabled = !_busy && ready && one is not null && PracticeSaveSource.LoadBlockReason(one, RunningBuild(), _slot) is null;
        _delete.Enabled = !_busy && selected.Count > 0; _categories.Enabled = _chooseFolder.Enabled = _categoryMenu.Enabled = !_busy; _grid.Enabled = _search.Enabled = _name.Enabled = !_busy;
        _moveCategory.Enabled = !_busy && selected.Count > 0;
        var ids = _visible.Select(s => s.Id).ToArray(); var chosen = selected.Select(s => s.Id).ToHashSet();
        _up.Enabled = !_busy && PracticeSaveLibrary.CanMove(ids, chosen, -1); _down.Enabled = !_busy && PracticeSaveLibrary.CanMove(ids, chosen, 1);
        _selection.Text = selected.Count > 1 ? $"{selected.Count} checkpoints selected · Load is unavailable" : one is not null ? "Selected: " + one.Name : "Select checkpoints.";
        _connection.Text = SaveLocation() is null ? "Choose the Steam save folder below." : !running ? "Launch the game through LAB Plus and open a campaign slot."
            : !_runtimeReady ? _runtimeMessage : one is not null && PracticeSaveSource.LoadBlockReason(one, RunningBuild(), _slot) is string reason ? reason
            : "Load into " + PracticeSaveSource.SlotLabel(_slot) + " · Current slot in the game";
    }
    private async Task Run(Func<Task> action, bool needsRuntime = false, Action<Exception>? failure = null)
    {
        if (_busy) return; _busy = true; _categoryMenu.Visible = false; RefreshState();
        try { if (needsRuntime && _runtimeRefresh is { IsCompleted: false }) await _runtimeRefresh; await action(); }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException or System.Security.Cryptography.CryptographicException or InvalidOperationException or System.ComponentModel.Win32Exception)
        { failure?.Invoke(ex); if (!IsDisposed) { _details = ex.Message; _status.Text = FriendlyError(ex.Message) + (ex.Message.Contains("original", StringComparison.OrdinalIgnoreCase) ? "\n" + ex.Message : ""); } }
        finally { _busy = false; if (!IsDisposed) { UpdateAvailability(); RefreshState(); } }
    }
    internal static string FriendlyError(string text)
    {
        string message = text.Split("Steam Cloud flags:", StringSplitOptions.None)[0];
        if (message.Contains("Game closed during live operation", StringComparison.OrdinalIgnoreCase)) return "The game closed before loading was confirmed.";
        if (message.Contains("Selected game process changed", StringComparison.OrdinalIgnoreCase)) return "The selected game changed before the operation finished.";
        if (message.Contains("rollback failed", StringComparison.OrdinalIgnoreCase)) return "Original files could not be restored. Close the game and Steam before recovering the retained original.";
        if (message.Contains("Files installed", StringComparison.OrdinalIgnoreCase)) return "Files were installed, but loading did not start.";
        if (message.Contains("completion", StringComparison.OrdinalIgnoreCase) || message.Contains("not observed", StringComparison.OrdinalIgnoreCase)) return "Loading is not confirmed yet. Continue in the game when loading reaches 100%.";
        if (message.Contains("authentication", StringComparison.OrdinalIgnoreCase)) return "Files are damaged or belong to another account.";
        if (message.Contains("Cloud", StringComparison.OrdinalIgnoreCase)) return "Disable Steam Cloud for DOOM Eternal and retry.";
        if (message.Contains("account", StringComparison.OrdinalIgnoreCase) || message.Contains("owner", StringComparison.OrdinalIgnoreCase)) return "The save account does not match the game account. Choose the matching Steam folder.";
        if (message.Contains("No response", StringComparison.OrdinalIgnoreCase)) return "The game did not respond. Restart it through this updated LAB Plus build.";
        if (message.Contains("Load the selected campaign slot first", StringComparison.OrdinalIgnoreCase)) return "Enter a campaign slot to enable Load. That slot becomes the destination for the archive.";
        return message;
    }
    private void RequireRuntime()
    { if (_gameProcess() <= 0 || !_runtimeReady || !PracticeSaveVault.ValidSlot(_slot)) throw new IOException("Load a campaign slot in the game first."); }
    private Task SaveCheckpoint() => Run(async () =>
    {
        RequireRuntime(); string remote = SaveLocation()?.Remote ?? throw new IOException("Choose the Steam save folder below."), name = _name.Text.Trim();
        if (name.Length == 0) name = "Checkpoint — " + DateTime.Now.ToString("dd.MM HH:mm:ss");
        string category = _library.Category == PracticeSaveLibrary.All ? "Uncategorized" : _library.Category; _status.Text = "Saving checkpoint…";
        var capture = await LivePracticeBridge.Execute(_directory, _gameProcess, "archive", remote, _slot, true, value =>
        { if (!IsDisposed && value.Phase == "waiting") _status.Text = value.Detail.Split("Steam Cloud flags:")[0]; });
        var save = await Task.Run(() => _vault.ImportLiveCheckpoint(_directory, capture, remote, name, category));
        if (IsDisposed) return; _search.Clear(); Reload(save.Id); _details = capture.Detail; _status.Text = "Saved: " + save.Name;
    }, needsRuntime: true);
    private Task LoadCheckpoint() => Run(() => LoadArchive(Selected ?? throw new IOException("Select one checkpoint to load.")), needsRuntime: true);
    internal async Task<(bool Success, string Message)> LoadLastCheckpointFromCommand(int process)
    {
        if (_busy) return (false, "A checkpoint operation is already running. Wait for it to finish.");
        if (process <= 0 || process != _gameProcess()) return (false, "The selected game process changed.");
        Exception? error = null; bool completed = false;
        await Run(async () =>
        {
            Discover(); await RefreshRuntime();
            if (!_runtimeReady) throw new IOException(_runtimeMessage);
            RequireRuntime(); string remote = SaveLocation()?.Remote ?? throw new IOException("Choose the Steam save folder in Saves first.");
            string build = RunningBuild() ?? "", slot = _slot;
            var save = await Task.Run(() => _vault.LastLiveArchive(remote, slot, build));
            await LoadArchive(save); completed = !IsDisposed;
        }, needsRuntime: true, failure: ex => error = ex);
        string message = error is FileNotFoundException or DirectoryNotFoundException or JsonException or System.Security.Cryptography.CryptographicException
            ? "The last LAB Plus save became unavailable or damaged. Loading was stopped." : error is null ? _status.Text : FriendlyError(error.Message);
        return (completed && error is null, message);
    }
    private async Task LoadArchive(PracticeSave save)
    {
        RequireRuntime();
        if (PracticeSaveSource.LoadBlockReason(save, RunningBuild(), _slot) is string reason) throw new IOException(reason);
        string remote = SaveLocation()?.Remote ?? throw new IOException("Choose the Steam save folder below."); int process = _gameProcess(); _status.Text = "Preparing checkpoint load…";
        await LivePracticeBridge.Execute(_directory, _gameProcess, "prepare", remote, _slot, true, value =>
        { if (!IsDisposed && value.Phase == "waiting") _status.Text = value.Detail.Split("Steam Cloud flags:")[0]; });
        if (IsDisposed) return;
        var plan = await Task.Run(() => _vault.PlanLiveRestore(process, save.Id, remote, _slot, true));
        await Task.Run(() => _vault.StageLiveRestore(_directory, plan)); if (IsDisposed) return; _details = plan.Description;
        var result = await LivePracticeBridge.ExecuteRestore(_directory, _gameProcess, plan, true, value =>
        { if (!IsDisposed && value.Phase is "waiting" or "loading") { _details = value.Detail; _status.Text = value.Phase == "loading" ? "Return to the game. Continue when loading reaches 100%." : value.Detail.Split("Steam Cloud flags:")[0]; } });
        if (IsDisposed) return; _details = result.Detail; _status.Text = "Loaded: " + save.Name;
    }
    private Task MoveSelected(int direction) => Run(() =>
    { var selected = SelectedIds; _library.Move(_all, _visible.Select(s => s.Id).ToArray(), selected, direction); Populate(selected); return Task.CompletedTask; });
    private Task DeleteSelected() => Run(async () =>
    {
        var selected = SelectedArchives; if (selected.Count == 0) return;
        string names = string.Join("\n", selected.Take(6).Select(s => s.Name)) + (selected.Count > 6 ? $"\n…and {selected.Count - 6} more" : "");
        using var review = new WorkspaceMessageForm($"Delete {selected.Count} checkpoint archive(s)?\n\n{names}\n\nOnly the selected LAB Plus archives will be deleted. The game's current save stays in place.", "Delete checkpoints", MessageBoxButtons.OKCancel);
        review.AcceptButton!.Text = "Delete"; review.AcceptButton.Size = new(ActionWidth, 42);
        if (review.ShowDialog(OwnerScreen) != DialogResult.OK) return;
        try { await Task.Run(() => _vault.DeleteArchives(selected.Select(s => s.Id).ToArray())); }
        finally { if (!IsDisposed) Reload(); }
        if (!IsDisposed) _status.Text = $"Deleted {selected.Count} checkpoint(s).";
    });
    private void OpenTools()
    {
        using var advanced = new PracticeSaveAdvancedForm(_directory, _link, _vault, _context, _gameProcess) { SelectedRemote = _remote, SelectedSlot = PracticeSaveVault.ValidSlot(_slot) ? _slot : "GAME-AUTOSAVE0" };
        advanced.ShowDialog(OwnerScreen); if (!string.IsNullOrEmpty(advanced.SelectedRemote)) UseSource(advanced.SelectedRemote); Reload();
    }
    private LabPlusForm OwnerScreen => Host is null ? Parent?.FindForm() ?? this : this;
}
