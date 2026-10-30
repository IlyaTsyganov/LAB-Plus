using System.Drawing.Imaging;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text.Json;

namespace LabPlus;

// Serialize headless screen state and dispatch actions. These nodes are plain
// data, with no native controls, handles, layout engine or second UI surface.
internal sealed class ScreenStateAdapter : IDisposable
{
    private readonly LabPlusForm _form;
    internal readonly UiNode[] Roots;
    private readonly Dictionary<string, UiNode> _controls = [];
    private readonly Dictionary<UiNode, string> _ids = [];
    private readonly Dictionary<string, UiTreeNode> _nodes = [];
    private readonly Dictionary<UiTreeNode, string> _nodeIds = [];
    private readonly Dictionary<UiNode, int> _limits = [];
    private readonly Dictionary<UiNode, (bool Dirty, string Image)> _pictures = [];
    private int _sequence;
    private bool _disposed;
    internal ScreenStateAdapter(LabPlusForm form) { _form = form; Roots = form.Controls.Cast<UiNode>().ToArray(); }
    private string Id(UiNode control)
    { if (_ids.TryGetValue(control, out string? id)) return id; id = "c" + ++_sequence; _ids.Add(control, id); _controls.Add(id, control); return id; }
    private string Id(UiTreeNode node)
    { if (_nodeIds.TryGetValue(node, out string? id)) return id; id = "n" + ++_sequence; _nodeIds.Add(node, id); _nodes.Add(id, node); return id; }
    internal object[] Snapshot(IEnumerable<UiNode> roots) => roots.Where(c => !c.IsDisposed && c.Visible && (c is not UiText || !string.IsNullOrWhiteSpace(c.Text))).Select(Capture).ToArray();

    private Dictionary<string, object?> Capture(UiNode control)
    {
        control.TabStop = false;
        string id = Id(control); float scale = control.DeviceDpi / 96f;
        var node = new Dictionary<string, object?> { ["id"] = id, ["kind"] = "column", ["text"] = control.Text, ["enabled"] = control.Enabled, ["width"] = Math.Round(control.Width / scale), ["height"] = Math.Round(control.Height / scale), ["small"] = control.Font.SizeInPoints <= 9.5f, ["fill"]=control.Dock==DockStyle.Fill };
        switch (control)
        {
            case UiChoice radio:
                node["kind"] = "radio"; node["checked"] = radio.Checked; break;
            case UiToggle check:
                node["kind"] = "check"; node["checked"] = check.Checked; break;
            case UiAction button:
                node["kind"] = "button"; node["primary"] = button is WorkspaceButton { Primary: true }; break;
            case UiText label:
                node["kind"] = "label"; node["heading"] = label.Font.SizeInPoints >= 15;
                node["collapsible"] = label.Font.SizeInPoints <= 9.5f && label.Text.Length > 220;
                node["small"] = label.Font.SizeInPoints <= 9.5f || label.Text.Length > 160; break;
            case UiInput text:
                node["kind"] = text.Multiline ? "textarea" : "text"; node["value"] = text.Text;
                node["readonly"] = text.ReadOnly; node["maxLength"] = text.MaxLength;
                node["wrap"] = text.WordWrap;
                node["placeholder"] = text.PlaceholderText;
                break;
            case UiNumber numeric:
                node["kind"] = "number"; node["value"] = numeric.Value; node["minimum"] = numeric.Minimum; node["maximum"] = numeric.Maximum; node["step"] = numeric.Increment; break;
            case UiSelect combo:
                node["kind"] = combo.DropDownStyle == ComboBoxStyle.DropDownList ? "select" : "combo";
                node["items"] = combo.Items.Cast<object>().Select(combo.GetItemText).ToArray(); node["selected"] = combo.SelectedIndex; node["value"] = combo.Text; break;
            case UiCheckList list:
                node["kind"] = "checked-list"; node["items"] = list.Items.Cast<object>().Select(list.GetItemText).ToArray();
                node["selected"] = list.SelectedIndex; node["checks"] = Enumerable.Range(0, list.Items.Count).Select(list.GetItemChecked).ToArray(); break;
            case UiList list:
                node["kind"] = "list"; node["items"] = list.Items.Cast<object>().Select(list.GetItemText).ToArray(); node["selected"] = list.SelectedIndex; break;
            case UiRange track:
                node["kind"] = "slider"; node["minimum"] = track.Minimum; node["maximum"] = track.Maximum; node["value"] = track.Value; node["step"] = track.SmallChange; break;
            case UiTable grid:
                CaptureGrid(grid, node); break;
            case UiTree tree:
                node["kind"] = "tree"; node["checks"] = tree.CheckBoxes; node["nodes"] = CaptureTree(tree.Nodes); break;
            case UiTabs tabs:
                node["kind"] = "tabs"; node["items"] = tabs.TabPages.Cast<UiTab>().Select(p => p.Text).ToArray(); node["selected"] = tabs.SelectedIndex;
                node["children"] = tabs.SelectedTab is null ? [] : Snapshot(tabs.SelectedTab.Controls.Cast<UiNode>()); break;
            case UiSplit split:
                node["kind"] = "split"; node["vertical"] = split.Orientation == Orientation.Vertical;
                node["ratio"] = Math.Clamp(split.SplitterDistance / (double)Math.Max(1, split.Orientation == Orientation.Vertical ? split.Width : split.Height), .18, .82);
                node["children"] = new[] { Capture(split.Panel1), Capture(split.Panel2) }; break;
            case UiToolbar flow:
                node["kind"] = flow.FlowDirection is FlowDirection.TopDown or FlowDirection.BottomUp ? "column" : "toolbar";
                node["children"] = Snapshot(flow.Controls.Cast<UiNode>()); break;
            case UiGrid table:
                node["kind"] = table.ColumnCount <= 1 ? "column" : "grid";
                var widths = new List<string>();
                for (int i = 0; i < table.ColumnCount; i++)
                {
                    var style = i < table.ColumnStyles.Count ? table.ColumnStyles[i] : null;
                    widths.Add(style?.SizeType == SizeType.Absolute ? "minmax(0," + Math.Max(100, Math.Round(style.Width / scale)).ToString(System.Globalization.CultureInfo.InvariantCulture) + "px)" : "minmax(0," + (style?.SizeType == SizeType.Percent ? Math.Max(1, style.Width) : 1).ToString(System.Globalization.CultureInfo.InvariantCulture) + "fr)");
                }
                node["columns"] = widths;
                node["children"] = table.Controls.Cast<UiNode>().Where(c => c.Visible && !c.IsDisposed).OrderBy(c => table.GetRow(c)).ThenBy(c => table.GetColumn(c)).Select(c =>
                { var child = Capture(c); child["row"] = table.GetRow(c) + 1; child["column"] = table.GetColumn(c) + 1; child["span"] = table.GetColumnSpan(c); return child; }).ToArray();
                break;
            case UiGroup group:
                node["kind"] = "group"; node["children"] = Snapshot(group.Controls.Cast<UiNode>()); break;
            case MotionPlot or EntityGraphCanvas or PlaneProjection:
                if(control is PlaneProjection)control.Width=340;
                int cap = (int)((control is PlaneProjection ? 280 : 300) * scale);
                control.MaximumSize = new Size(0, cap);
                if (control.Height > cap) control.Height = cap;
                node["kind"] = "canvas"; node["image"] = Picture(control); node["actions"]=Snapshot(control.Controls); break;
            case UiStack panel:
                node["kind"] = "column"; node["children"] = Snapshot(panel.Controls.Cast<UiNode>()); break;
            default:
                node["children"] = Snapshot(control.Controls.Cast<UiNode>()); break;
        }
        return node;
    }

    private void CaptureGrid(UiTable grid, Dictionary<string, object?> node)
    {
        node["kind"] = "table";
        var columns = grid.Columns.Cast<UiTableColumn>().Where(c => c.Visible).OrderBy(c => c.DisplayIndex).ToArray();
        node["columns"] = columns.Select(c => new { index = c.Index, text = c.HeaderText }).ToArray();
        int limit = _limits.GetValueOrDefault(grid, 200);
        var rows = new List<object>();
        for (int row = 0; row < Math.Min(limit, grid.RowCount); row++)
        {
            if (grid.Rows[row].IsNewRow) continue;
            var values = columns.Select(c => Convert.ToString(grid.Rows[row].Cells[c.Index].FormattedValue, System.Globalization.CultureInfo.CurrentCulture) ?? "").ToArray();
            rows.Add(new { index = row, values, titles=columns.Select(c=>grid.CellTip(row,c.Index)).ToArray(), selected = grid.Rows[row].Selected });
        }
        node["rows"] = rows; node["total"] = grid.RowCount;
        node["selected"] = grid.CurrentCell?.RowIndex ?? -1;
        node["empty"] = grid.FindForm() is PracticeSaveForm ? "No checkpoints match this account, slot and search. Save a checkpoint to add one." : "No data yet.";
    }
    private object[] CaptureTree(UiTreeNodes nodes) => nodes.Cast<UiTreeNode>().Select(n => new
    { id = Id(n), text = n.Text, check = n.Checked, expanded = n.IsExpanded, hasChildren = n.Nodes.Count > 0, nodes = n.IsExpanded ? CaptureTree(n.Nodes) : [] }).Cast<object>().ToArray();

    private string Picture(UiNode control)
    {
        if (!_pictures.TryGetValue(control, out var entry))
        {
            _pictures[control] = (true, "");
            control.Invalidated += (_, _) => { if (!_disposed && _pictures.TryGetValue(control, out var current)) _pictures[control] = (true, current.Image); };
            entry = (true, "");
        }
        if (!entry.Dirty) return entry.Image;
        if (control.Width <= 0 || control.Height <= 0) return "";
        using var bitmap = new Bitmap(control.Width, control.Height);
        ((UiPlot)control).DrawToBitmap(bitmap, new Rectangle(Point.Empty, bitmap.Size));
        using var stream = new MemoryStream(); bitmap.Save(stream, ImageFormat.Png);
        string data = "data:image/png;base64," + Convert.ToBase64String(stream.ToArray());
        _pictures[control] = (false, data); return data;
    }

    internal void Dispatch(JsonElement message)
    {
        string id = message.GetProperty("id").GetString() ?? "";
        string action = message.GetProperty("action").GetString() ?? "";
        if (_nodes.TryGetValue(id, out var treeNode))
        {
            if (treeNode.TreeView is not { Enabled: true }) return;
            if (action == "expand") { if (treeNode.IsExpanded) treeNode.Collapse(); else treeNode.Expand(); }
            else if (action == "check") treeNode.Checked = message.GetProperty("value").GetBoolean();
            else if (action == "select") treeNode.TreeView.SelectedNode = treeNode;
            return;
        }
        if (!_controls.TryGetValue(id, out var control) || control.IsDisposed || !control.Enabled) return;
        switch (action)
        {
            case "click" when control is UiAction button: button.PerformClick(); break;
            case "check" when control is UiChoice radio: radio.Checked = message.GetProperty("value").GetBoolean(); break;
            case "check" when control is UiToggle check: check.Checked = message.GetProperty("value").GetBoolean(); break;
            case "text" when control is UiInput text && !text.ReadOnly:
                string value = message.GetProperty("value").GetString() ?? "";
                text.Text = value[..Math.Min(value.Length, text.MaxLength > 0 ? text.MaxLength : 1_000_000)]; break;
            case "text" when control is UiSelect combo && combo.DropDownStyle != ComboBoxStyle.DropDownList: combo.Text = message.GetProperty("value").GetString() ?? ""; break;
            case "number" when control is UiNumber number:
                if (message.GetProperty("value").TryGetDecimal(out var numeric)) number.Value = Math.Clamp(numeric, number.Minimum, number.Maximum); break;
            case "number" when control is UiRange track:
                track.UserValue(message.GetProperty("value").GetInt32()); break;
            case "select" when control is UiSelect combo:
                int choice = message.GetProperty("value").GetInt32(); if (choice >= -1 && choice < combo.Items.Count) combo.SelectedIndex = choice; break;
            case "select" when control is UiList list:
                int selected = message.GetProperty("value").GetInt32(); if (selected >= -1 && selected < list.Items.Count) list.SelectedIndex = selected; break;
            case "item-check" when control is UiCheckList list:
                int index = message.GetProperty("index").GetInt32(); if (index >= 0 && index < list.Items.Count) list.SetItemChecked(index, message.GetProperty("value").GetBoolean()); break;
            case "select" when control is UiTabs tabs:
                int tab = message.GetProperty("value").GetInt32(); if (tab >= 0 && tab < tabs.TabCount) tabs.SelectedIndex = tab; break;
            case "row" or "row-double" when control is UiTable grid:
                int row = message.GetProperty("row").GetInt32();
                int column = message.TryGetProperty("column", out var c) ? c.GetInt32() : 0;
                if (row < 0 || row >= grid.RowCount || column < 0 || column >= grid.ColumnCount) return;
                grid.CurrentCell = grid.Rows[row].Cells[column]; grid.Rows[row].Selected = true;
                if (action == "row-double") grid.DoubleClickCell(row,column);
                break;
            case "more" when control is UiTable grid: _limits[grid] = Math.Min(grid.RowCount, _limits.GetValueOrDefault(grid, 200) + 200); break;
            case "mouse" when control is MotionPlot or EntityGraphCanvas or PlaneProjection:
                Mouse(control, message); break;
        }
        _form.Workspace?.Publish(true);
    }
    private void Mouse(UiNode control, JsonElement message)
    {
        string? phase = message.GetProperty("phase").GetString();
        string? method = phase switch { "down" => "OnMouseDown", "move" => "OnMouseMove", "up" => "OnMouseUp", "click" => "OnMouseClick", "wheel" => "OnMouseWheel", _ => null };
        if (method is null) return;
        int x = Math.Clamp((int)(message.GetProperty("x").GetDouble() * control.Width), 0, Math.Max(0, control.Width-1));
        int y = Math.Clamp((int)(message.GetProperty("y").GetDouble() * control.Height), 0, Math.Max(0, control.Height-1));
        int button = message.GetProperty("button").GetInt32();
        var buttons = button == 2 ? MouseButtons.Right : button == 1 ? MouseButtons.Left : MouseButtons.None;
        int delta = message.TryGetProperty("delta", out var wheel) ? Math.Clamp(wheel.GetInt32(), -1920, 1920) : 0;
        var args = new MouseEventArgs(buttons, phase is "down" or "click" ? 1 : 0, x, y, delta);
        if (control is EntityGraphCanvas graph) graph.BrowserMouse(phase!, args);
        else control.GetType().GetMethod(method, BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(control, [args]);
        if (_pictures.TryGetValue(control, out var picture)) _pictures[control] = (true, picture.Image);
    }
    internal void Key(JsonElement message)
    {
        string? key = message.GetProperty("key").GetString();
        bool ctrl = message.TryGetProperty("ctrl", out var flag) && flag.GetBoolean();
        Keys code = key == "Enter" ? Keys.Enter : key == "Escape" ? Keys.Escape : ctrl && key is "s" or "S" ? Keys.S | Keys.Control : Keys.None;
        if (code == Keys.None) return;
        string id = message.GetProperty("id").GetString() ?? "";
        _controls.TryGetValue(id, out var control);
        var form = control?.FindForm() ?? _form;
        if (code == Keys.Escape) { form.CancelButton?.PerformClick(); return; }
        if (form is WorkspaceMessageForm && code == Keys.Enter) { form.AcceptButton?.PerformClick(); return; }
        var args = new KeyEventArgs(code);
        if (control is { Enabled: true }) typeof(UiNode).GetMethod("OnKeyDown", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(control, [args]);
        if (!args.Handled) typeof(UiNode).GetMethod("OnKeyDown", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(form, [args]);
    }
    public void Dispose() { _disposed = true; _controls.Clear(); _nodes.Clear(); _ids.Clear(); _nodeIds.Clear(); _pictures.Clear(); }
}
