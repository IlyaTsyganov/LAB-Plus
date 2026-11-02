using System.Collections.ObjectModel;

namespace LabPlus;

internal sealed class UiCellStyle
{
    public Color BackColor { get; set; } public Color ForeColor { get; set; }
    public Color SelectionBackColor { get; set; } public Color SelectionForeColor { get; set; }
    public Font? Font { get; set; }
}
internal class UiTableColumn
{
    public string HeaderText { get; set; }="";
    public int Width { get; set; }=100;
    public int MinimumWidth { get; set; }
    public float FillWeight { get; set; }
    public DataGridViewColumnSortMode SortMode { get; set; }
    public int Index { get; internal set; }
    public int DisplayIndex=>Index;
    public bool Visible { get; set; }=true;
    public bool Frozen { get; set; }
}
internal sealed class UiTableColumns : Collection<UiTableColumn>
{
    protected override void InsertItem(int index, UiTableColumn item) { item.Index=index;base.InsertItem(index,item); }
}
internal sealed class UiTableCell(UiTable table,int row,int column)
{
    private object? _value;
    public object? Value { get=>table.VirtualMode?table.CellValue(row,column):_value; set=>_value=value; }
    public object? FormattedValue=>Value;
    public UiCellStyle Style { get; set; }=new();
    public string? ToolTipText { get; set; }
    public int RowIndex=>row; public int ColumnIndex=>column;
}
internal sealed class UiTableRow
{
    private bool _selected;
    private readonly UiTable _table;
    public List<UiTableCell> Cells { get; }
    public UiTableRow(UiTable table,int index) { _table=table;Cells=Enumerable.Range(0,table.Columns.Count).Select(c=>new UiTableCell(table,index,c)).ToList(); }
    public bool Selected { get=>_selected;set { if(_selected==value)return;_selected=value; } }
    public bool IsNewRow=>false;
    public object? Tag { get; set; }
    public int Height { get; set; }=32;
    public string Subtitle { get; set; } = "";
    public string Note { get; set; } = "";
    public bool Unavailable { get; set; }
}
internal sealed class UiTableRows(UiTable table) : Collection<UiTableRow>
{
    public new void Add(UiTableRow row)=>base.Add(row);
    public int Add(params object?[] values) { int row=Count;var item=new UiTableRow(table,row);for(int i=0;i<Math.Min(values.Length,item.Cells.Count);i++)item.Cells[i].Value=values[i];base.Add(item);if(table.CurrentCell is null&&item.Cells.Count>0)table.CurrentCell=item.Cells[0];return row; }
    protected override void ClearItems() { base.ClearItems();table.CurrentCell=null; }
}
internal sealed class UiTable : UiNode
{
    private UiTableCell? _current;
    public UiTableColumns Columns { get; }=new();
    public int ColumnCount=>Columns.Count;
    public UiTableRows Rows { get; }
    public UiTable() { Rows=new(this);RowTemplate=new(this,0); }
    public int RowCount { get=>Rows.Count;set { while(Rows.Count>value)Rows.RemoveAt(Rows.Count-1);while(Rows.Count<value)Rows.Add(new UiTableRow(this,Rows.Count));if(_current?.RowIndex>=value)CurrentCell=null; } }
    public UiTableCell? CurrentCell { get=>_current;set { if(_current==value)return;_current=value;_selectionAnchor=value?.RowIndex??-1;foreach(var row in Rows)row.Selected=false;if(value is not null&&value.RowIndex<Rows.Count)Rows[value.RowIndex].Selected=true;SelectionChanged?.Invoke(this,EventArgs.Empty); } }
    public int FirstDisplayedScrollingRowIndex { get; set; }=-1;
    public bool ReadOnly { get; set; } public bool VirtualMode { get; set; }
    public bool AllowUserToAddRows { get; set; } public bool AllowUserToDeleteRows { get; set; }
    public bool RowHeadersVisible { get; set; } public bool MultiSelect { get; set; }
    public bool ShowSelectionChecks { get; set; }
    private int _selectionAnchor = -1;
    public IReadOnlyList<int> SelectedIndices => Enumerable.Range(0, Rows.Count).Where(i => Rows[i].Selected).ToArray();
    public void SetSelectedRows(IEnumerable<int> indices)
    {
        ApplySelection(indices); _selectionAnchor = _current?.RowIndex ?? -1;
    }
    private void ApplySelection(IEnumerable<int> indices)
    {
        var selected = indices.Where(i => i >= 0 && i < Rows.Count).ToHashSet();
        for (int i = 0; i < Rows.Count; i++) Rows[i].Selected = selected.Contains(i);
        _current = selected.Count > 0 ? Rows[selected.Min()].Cells[0] : null;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
    }
    internal void SelectRow(int row, int column, bool toggle = false, bool range = false)
    {
        if (!MultiSelect) { CurrentCell = Rows[row].Cells[column]; return; }
        var selected = toggle ? SelectedIndices.ToHashSet() : new HashSet<int>();
        if (range && _selectionAnchor >= 0 && _selectionAnchor < Rows.Count)
            foreach (int i in Enumerable.Range(Math.Min(row, _selectionAnchor), Math.Abs(row - _selectionAnchor) + 1)) selected.Add(i);
        else { if (!selected.Add(row)) selected.Remove(row); _selectionAnchor = row; }
        ApplySelection(selected);
    }
    internal void SelectAllRows() => SetSelectedRows(SelectedIndices.Count == Rows.Count ? [] : Enumerable.Range(0, Rows.Count));
    public bool EnableHeadersVisualStyles { get; set; }
    public DataGridViewSelectionMode SelectionMode { get; set; }
    public DataGridViewAutoSizeColumnsMode AutoSizeColumnsMode { get; set; }
    public Color BackgroundColor { get; set; } public Color GridColor { get; set; }
    public UiCellStyle DefaultCellStyle { get; }=new();
    public UiCellStyle ColumnHeadersDefaultCellStyle { get; }=new();
    public UiTableRow RowTemplate { get; }
    public int ColumnHeadersHeight { get; set; }
    public event EventHandler? SelectionChanged;
    public event EventHandler<DataGridViewCellValueEventArgs>? CellValueNeeded;
    public event EventHandler<UiCellTipEvent>? CellToolTipTextNeeded;
    public event EventHandler<DataGridViewCellEventArgs>? CellDoubleClick;
    internal object? CellValue(int row,int column) { var e=new DataGridViewCellValueEventArgs(column,row);CellValueNeeded?.Invoke(this,e);return e.Value; }
    internal string? CellTip(int row,int column) { var e=new UiCellTipEvent(column,row);CellToolTipTextNeeded?.Invoke(this,e);return e.ToolTipText; }
    internal void DoubleClickCell(int row,int column)=>CellDoubleClick?.Invoke(this,new(column,row));
}
internal sealed class UiCellTipEvent(int column,int row) : EventArgs
{ public int ColumnIndex=>column;public int RowIndex=>row;public string? ToolTipText { get; set; } }
