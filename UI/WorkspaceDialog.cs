namespace LabPlus;

internal static class WorkspaceDialog
{
    internal static DialogResult Show(IWin32Window owner, string text, string caption = "LAB Plus", MessageBoxButtons buttons = MessageBoxButtons.OK, MessageBoxIcon icon = MessageBoxIcon.None)
    {
        using var dialog = new WorkspaceMessageForm(text, caption, buttons);
        return dialog.ShowDialog(owner);
    }
}

internal class WorkspaceMessageForm : LabPlusForm
{
    internal bool Compact { get; set; }
    internal UiText MessageText { get; private set; } = null!;
    internal WorkspaceMessageForm(string text, string caption, MessageBoxButtons buttons)
        : this(text, caption, buttons switch
        {
            MessageBoxButtons.OKCancel => [("Cancel", DialogResult.Cancel), ("OK", DialogResult.OK)],
            MessageBoxButtons.YesNo => [("No", DialogResult.No), ("Yes", DialogResult.Yes)],
            MessageBoxButtons.YesNoCancel => [("Cancel", DialogResult.Cancel), ("No", DialogResult.No), ("Yes", DialogResult.Yes)],
            _ => [("OK", DialogResult.OK)]
        }) { }

    internal WorkspaceMessageForm(string text, string caption, (string Label, DialogResult Result)[] choices)
    {
        Text = caption; ClientSize = new Size(760, Math.Min(800, Math.Max(410, 300 + text.Length / 5)));
        MinimumSize = new Size(600, 360); StartPosition = FormStartPosition.CenterParent; ShowInTaskbar = false; Font = LabPlusTheme.Ui(10);
        var root = new UiGrid { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 2, Padding = new Padding(24) };
        root.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); root.RowStyles.Add(new RowStyle(SizeType.Absolute, 50));
        MessageText = new UiText { Text = text, Dock = DockStyle.Fill, AutoEllipsis = false };
        root.Controls.Add(MessageText, 0, 0);
        var actions = new UiToolbar { Dock = DockStyle.Fill, FlowDirection = FlowDirection.RightToLeft };
        foreach (var (label, result) in choices)
        {
            var button = new WorkspaceButton { Text = label, DialogResult = result, Size = new Size(label.Length > 16 ? 260 : 110, 40), Primary = result is DialogResult.OK or DialogResult.Yes };
            actions.Controls.Add(button);
            if (result is DialogResult.OK or DialogResult.Yes) AcceptButton = button;
            if (result is DialogResult.Cancel or DialogResult.No) CancelButton = button;
        }
        root.Controls.Add(actions, 0, 1); Controls.Add(root);
    }
}
