using LabPlus;
using System.Runtime.CompilerServices;
using System.Text.Json;

internal static class Program
{
    [STAThread]
    private static int Main()
    {
        (string Name, Action Run)[] tests = [
            ("detached subtrees and their caches are collected", DetachedSubtreesAreCollected),
            ("root replacements use the current screen tree", RootReplacementUsesCurrentTree),
            ("stale snapshot roots cannot restore removed controls", StaleSnapshotRootsAreIgnored),
            ("removed and disposed controls reject queued input", RemovedControlsRejectInput),
            ("removed tree nodes reject queued input", RemovedTreeNodesRejectInput),
            ("hidden controls, tabs and collapsed nodes keep state", AttachedStateKeepsIdentity),
            ("plot subscriptions release the adapter", PlotSubscriptionsReleaseAdapter),
            ("disposal releases pagination state and prevents reuse", DisposalReleasesState)
        ];

        int failures = 0;
        foreach (var test in tests)
        {
            try { test.Run(); Console.WriteLine("PASS: " + test.Name); }
            catch (Exception error) { failures++; Console.Error.WriteLine($"FAIL: {test.Name}: {error.Message}"); }
        }
        Console.WriteLine($"Screen state fixtures: {tests.Length - failures}/{tests.Length} passed. No browser or game was started.");
        return failures == 0 ? 0 : 1;
    }

    private static void DetachedSubtreesAreCollected()
    {
        using var screen = new Screen();
        var references = Enumerable.Range(0, 20).SelectMany(_ => ReplaceSubtree(screen)).ToArray();
        CollectGarbage();
        Require(references.All(reference => !reference.IsAlive), "An adapter cache still retains a removed subtree.");
        GC.KeepAlive(screen);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference[] ReplaceSubtree(Screen screen)
    {
        var panel = new UiStack();
        var button = new UiAction { Text = "Temporary button" };
        var table = Table("Temporary table");
        var plot = new MotionPlot { Text = "Temporary plot", Size = new(120, 40) };
        var tree = new UiTree { Text = "Temporary tree" };
        var branch = new UiTreeNode("Temporary branch");
        var leaf = new UiTreeNode("Temporary leaf");
        branch.Nodes.Add(leaf); branch.Expand(); tree.Nodes.Add(branch);
        panel.Controls.AddRange([button, table, plot, tree]);
        screen.Root.Controls.Add(panel);
        string tableId = Id(screen.Snapshot(), table.Text);
        screen.Adapter.Dispatch(Message(tableId, "more"));
        screen.Root.Controls.Remove(panel);
        screen.Snapshot();
        return [new(panel), new(button), new(table), new(plot), new(tree), new(branch), new(leaf)];
    }

    private static void RootReplacementUsesCurrentTree()
    {
        using var form = new LabPlusForm();
        using var adapter = new ScreenStateAdapter(form);
        var removed = ReplaceRoot(form, adapter);
        var snapshot = JsonSerializer.SerializeToElement(adapter.Snapshot(adapter.Roots));
        Require(Find(snapshot, "Replacement root") is not null, "The adapter kept its constructor-time roots.");
        CollectGarbage();
        Require(!removed.IsAlive, "The adapter still retains a removed root.");
        GC.KeepAlive(adapter);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference ReplaceRoot(LabPlusForm form, ScreenStateAdapter adapter)
    {
        var old = new UiAction { Text = "Old root" };
        form.Controls.Add(old); adapter.Snapshot(form.Controls);
        form.Controls.Clear();
        form.Controls.Add(new UiAction { Text = "Replacement root" });
        adapter.Snapshot(form.Controls);
        return new(old);
    }

    private static void StaleSnapshotRootsAreIgnored()
    {
        using var screen = new Screen();
        var button = new UiAction { Text = "Removed snapshot root" };
        screen.Root.Controls.Add(button);
        screen.Snapshot();
        screen.Root.Controls.Remove(button);
        Require(screen.Adapter.Snapshot([button]).Length == 0, "A stale root re-registered a removed control.");
    }

    private static void RemovedControlsRejectInput()
    {
        using var screen = new Screen();
        int clicks = 0, keys = 0;
        var button = new UiAction { Text = "Action" };
        var input = new UiInput { Text = "Input" };
        button.Click += (_, _) => clicks++;
        input.KeyDown += (_, _) => keys++;
        screen.Form.KeyDown += (_, _) => keys++;
        screen.Root.Controls.AddRange([button, input]);
        var snapshot = screen.Snapshot();
        string buttonId = Id(snapshot, button.Text), inputId = Id(snapshot, input.Text);
        screen.Root.Controls.Remove(button); screen.Root.Controls.Remove(input);

        // Queued messages can arrive before the next periodic snapshot.
        screen.Adapter.Dispatch(Message(buttonId, "click"));
        screen.Adapter.Key(JsonSerializer.SerializeToElement(new { id = inputId, key = "s", ctrl = true }));
        Require(clicks == 0 && keys == 0, "Queued input reached a removed control or fell through to the form.");

        screen.Root.Controls.Add(input);
        string disposedId = Id(screen.Snapshot(), input.Text);
        input.Dispose();
        screen.Adapter.Key(JsonSerializer.SerializeToElement(new { id = disposedId, key = "s", ctrl = true }));
        Require(keys == 0, "Queued keyboard input reached a disposed control.");
        screen.Adapter.Key(JsonSerializer.SerializeToElement(new { id = "", key = "s", ctrl = true }));
        Require(keys == 1, "Screen-wide keyboard shortcuts stopped working.");
    }

    private static void RemovedTreeNodesRejectInput()
    {
        using var screen = new Screen();
        var tree = new UiTree { Text = "Tree" };
        var branch = new UiTreeNode("Branch");
        var leaf = new UiTreeNode("Leaf");
        branch.Nodes.Add(leaf); branch.Expand(); tree.Nodes.Add(branch);
        screen.Root.Controls.Add(tree);
        int changes = 0; tree.AfterCheck += (_, _) => changes++;
        string oldId = Id(screen.Snapshot(), leaf.Text);
        branch.Nodes.Clear();
        screen.Adapter.Dispatch(JsonSerializer.SerializeToElement(new { id = oldId, action = "check", value = true }));
        Require(!leaf.Checked && changes == 0, "A stale TreeView reference allowed input to a removed tree node.");
    }

    private static void AttachedStateKeepsIdentity()
    {
        using var screen = new Screen();
        var tabs = new UiTabs();
        var first = new UiTab(); var second = new UiTab();
        var table = Table("Persistent table");
        var tree = new UiTree { Text = "Persistent tree" };
        var branch = new UiTreeNode("Persistent branch");
        var leaf = new UiTreeNode("Persistent leaf");
        branch.Nodes.Add(leaf); branch.Expand(); tree.Nodes.Add(branch);
        first.Controls.AddRange([table, tree]); tabs.TabPages.AddRange([first, second]);
        screen.Root.Controls.Add(tabs);
        var before = screen.Snapshot();
        string tableId = Id(before, table.Text), leafId = Id(before, leaf.Text);
        screen.Adapter.Dispatch(Message(tableId, "more"));

        branch.Collapse(); tree.Visible = false; tabs.SelectedIndex = 1;
        screen.Snapshot();
        // A partial projection must not evict state from another attached panel.
        screen.Adapter.Snapshot(second.Controls);
        tabs.SelectedIndex = 0; tree.Visible = true; branch.Expand();
        var after = screen.Snapshot();
        Require(Id(after, table.Text) == tableId && Id(after, leaf.Text) == leafId, "Attached state lost stable identifiers.");
        Require(Find(after, table.Text)!.Value.GetProperty("rows").GetArrayLength() == 400, "An inactive tab lost table pagination.");

        second.Controls.Add(table); tabs.SelectedIndex = 1;
        Require(Id(screen.Snapshot(), table.Text) == tableId, "Reparenting within the screen changed an identifier.");
    }

    private static void PlotSubscriptionsReleaseAdapter()
    {
        foreach (bool detach in new[] { false, true })
        {
            var (plot, adapter) = ReleasePlotAdapter(detach);
            CollectGarbage();
            Require(!adapter.IsAlive, detach ? "A detached plot retains its adapter." : "A live plot retains its disposed adapter.");
            plot.Invalidate(); // An unsubscribed plot remains safe to use.
            plot.FindForm()?.Dispose(); plot.Dispose();
            GC.KeepAlive(plot);
        }
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (MotionPlot Plot, WeakReference Adapter) ReleasePlotAdapter(bool detach)
    {
        var form = new LabPlusForm();
        var plot = new MotionPlot { Size = new(120, 40) };
        form.Controls.Add(plot);
        var adapter = new ScreenStateAdapter(form);
        adapter.Snapshot(adapter.Roots);
        if (detach) { form.Controls.Remove(plot); adapter.Snapshot(adapter.Roots); form.Dispose(); }
        else adapter.Dispose();
        return (plot, new(adapter));
    }

    private static void DisposalReleasesState()
    {
        using var screen = new Screen();
        var table = DisposeWithPagination(screen);
        Require(screen.Adapter.Snapshot(screen.Adapter.Roots).Length == 0, "A disposed adapter accepted another snapshot.");
        screen.Adapter.Dispatch(Message("stale", "more"));
        screen.Adapter.Key(JsonSerializer.SerializeToElement(new { id = "", key = "s", ctrl = true }));
        screen.Adapter.Dispose();
        CollectGarbage();
        Require(!table.IsAlive, "Disposal left a table in the pagination cache.");
        GC.KeepAlive(screen);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference DisposeWithPagination(Screen screen)
    {
        var table = Table("Disposed table"); screen.Root.Controls.Add(table);
        screen.Adapter.Dispatch(Message(Id(screen.Snapshot(), table.Text), "more"));
        screen.Root.Controls.Remove(table); screen.Adapter.Dispose();
        return new(table);
    }

    private static UiTable Table(string text)
    {
        var table = new UiTable { Text = text };
        table.Columns.Add(new UiTableColumn { HeaderText = "Value" });
        table.RowCount = 450;
        return table;
    }

    private static JsonElement Message(string id, string action) => JsonSerializer.SerializeToElement(new { id, action });
    private static string Id(JsonElement snapshot, string text) => Find(snapshot, text)!.Value.GetProperty("id").GetString()!;

    private static JsonElement? Find(JsonElement value, string text)
    {
        if (value.ValueKind == JsonValueKind.Object)
        {
            if (value.TryGetProperty("text", out var label) && label.GetString() == text) return value;
            foreach (var property in value.EnumerateObject()) if (Find(property.Value, text) is { } found) return found;
        }
        else if (value.ValueKind == JsonValueKind.Array)
            foreach (var item in value.EnumerateArray()) if (Find(item, text) is { } found) return found;
        return null;
    }

    private static void CollectGarbage() { GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); }
    private static void Require(bool condition, string message) { if (!condition) throw new Exception(message); }

    private sealed class Screen : IDisposable
    {
        internal readonly LabPlusForm Form = new();
        internal readonly UiStack Root = new();
        internal readonly ScreenStateAdapter Adapter;
        internal Screen() { Form.Controls.Add(Root); Adapter = new(Form); }
        internal JsonElement Snapshot() => JsonSerializer.SerializeToElement(Adapter.Snapshot(Adapter.Roots));
        public void Dispose() { Adapter.Dispose(); Form.Dispose(); }
    }
}
