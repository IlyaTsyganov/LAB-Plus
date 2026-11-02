using System.Text.Json;

namespace LabPlus.Game;

// View metadata is separate from authenticated checkpoint manifests/containers.
public sealed class PracticeSaveLibrary
{
    public const string All = "All checkpoints";
    private sealed record State(string Category, string[] Categories, string[] Order, Dictionary<string, string>? Assignments = null);
    private readonly string _path;
    public string Category { get; private set; } = All;
    private readonly List<string> _categories = [];
    private readonly List<string> _order = [];
    private Dictionary<string, string> _assignments = [];
    public PracticeSaveLibrary(string directory)
    {
        _path = Path.Combine(directory, "practice-library.json");
        try
        {
            PracticeSaveVault.NoLinks(_path);
            if (!File.Exists(_path) || new FileInfo(_path).Length > 1_000_000) return;
            var state = JsonSerializer.Deserialize<State>(File.ReadAllText(_path));
            if (state is null || !ValidCategory(state.Category) || state.Categories is not { Length: <= 1000 } || state.Order is not { Length: <= 10000 }
                || state.Categories.Any(c => !ValidCategory(c)) || state.Order.Any(id => !LevelStateSnapshot.ValidToken(id))
                || state.Assignments is { Count: > 10000 } || state.Assignments?.Any(p => !LevelStateSnapshot.ValidToken(p.Key) || !ValidCategory(p.Value) || p.Value.Equals(All, StringComparison.OrdinalIgnoreCase)) == true) return;
            Category = state.Category; _categories.AddRange(state.Categories.Distinct(StringComparer.OrdinalIgnoreCase)); _order.AddRange(state.Order.Distinct());
            _assignments = state.Assignments ?? [];
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException) { }
    }
    public static bool ValidCategory(string? text) => text is { Length: > 0 and <= 160 } && !text.Any(char.IsControl);
    public IReadOnlyList<string> Categories(IEnumerable<PracticeSave> saves) => _categories.Concat(saves.Select(s => s.Category)).Where(c => c != All).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
    public IReadOnlyList<PracticeSave> Categorized(IEnumerable<PracticeSave> saves) => saves.Select(s => _assignments.TryGetValue(s.Id, out var category) ? s with { Category = category } : s).ToArray();
    public void Transfer(IReadOnlyList<PracticeSave> all, ISet<string> selected, string category)
    {
        category = category.Trim();
        if (!ValidCategory(category) || category.Equals(All, StringComparison.OrdinalIgnoreCase)) throw new IOException("Choose a destination category.");
        var ids = all.Select(s => s.Id).ToHashSet();
        if (selected.Count == 0 || selected.Any(id => !ids.Contains(id))) throw new IOException("Select existing checkpoints to move.");
        var next = _assignments.Where(p => ids.Contains(p.Key)).ToDictionary(p => p.Key, p => p.Value);
        foreach (var id in selected) next[id] = category;
        if (next.Count > 10000) throw new IOException("The library limit has been reached.");
        var previous = _assignments; _assignments = next;
        try { Save(); } catch { _assignments = previous; throw; }
    }
    public IReadOnlyList<PracticeSave> Ordered(IEnumerable<PracticeSave> saves)
    {
        var positions = _order.Select((id, index) => (id, index)).ToDictionary(p => p.id, p => p.index);
        return saves.OrderBy(s => positions.GetValueOrDefault(s.Id, int.MaxValue)).ThenByDescending(s => s.Captured).ThenBy(s => s.Id, StringComparer.Ordinal).ToArray();
    }
    public void Select(string category)
    {
        if (!ValidCategory(category)) throw new IOException("Enter a category name of up to 160 characters.");
        if (category != All && !_categories.Contains(category, StringComparer.OrdinalIgnoreCase))
        { if (_categories.Count >= 1000) throw new IOException("The category limit has been reached."); _categories.Add(category); }
        Category = category; Save();
    }
    public void Add(string category)
    {
        category = category.Trim();
        if (!ValidCategory(category) || category.Equals(All, StringComparison.OrdinalIgnoreCase)) throw new IOException("Enter a different category name of up to 160 characters.");
        Select(category);
    }
    public static bool CanMove(IReadOnlyList<string> visible, ISet<string> selected, int direction) => direction is -1 or 1
        && visible.Where((id, i) => selected.Contains(id) && i + direction >= 0 && i + direction < visible.Count && !selected.Contains(visible[i + direction])).Any();
    public void Move(IReadOnlyList<PracticeSave> all, IReadOnlyList<string> visible, ISet<string> selected, int direction)
    {
        if (!CanMove(visible, selected, direction)) return;
        var rows = visible.ToArray();
        if (direction < 0) { for (int i = 1; i < rows.Length; i++) if (selected.Contains(rows[i]) && !selected.Contains(rows[i - 1])) (rows[i - 1], rows[i]) = (rows[i], rows[i - 1]); }
        else { for (int i = rows.Length - 2; i >= 0; i--) if (selected.Contains(rows[i]) && !selected.Contains(rows[i + 1])) (rows[i], rows[i + 1]) = (rows[i + 1], rows[i]); }
        var visibleSet = visible.ToHashSet(); int index = 0;
        var result = Ordered(all).Select(s => visibleSet.Contains(s.Id) ? rows[index++] : s.Id).ToArray();
        _order.Clear(); _order.AddRange(result); Save();
    }
    private void Save()
    {
        PracticeSaveVault.NoLinks(_path); PracticeSaveVault.NoLinks(_path + ".tmp"); Directory.CreateDirectory(Path.GetDirectoryName(_path)!);
        File.WriteAllText(_path + ".tmp", JsonSerializer.Serialize(new State(Category, _categories.ToArray(), _order.ToArray(), _assignments)));
        File.Move(_path + ".tmp", _path, true);
    }
}
