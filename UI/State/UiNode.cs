using System.Collections.ObjectModel;

namespace LabPlus;

// Serializable presentation state. None of these classes owns a native window,
// paints a widget, or derives from System.Windows.Forms.Control.
internal class UiNode : IDisposable
{
    private string _text = "";
    private Size _size = new(180, 40);
    private bool _enabled = true;
    internal UiNode? Parent { get; set; }
    public UiChildren Controls { get; }
    public UiNode() { Controls = new(this); }
    public string Text { get => _text; set { if (_text == value) return; _text = value ?? ""; TextChanged?.Invoke(this, EventArgs.Empty); } }
    public string Name { get; set; } = "";
    public object? Tag { get; set; }
    public bool Enabled { get => _enabled && (Parent?.Enabled ?? true); set => _enabled = value; }
    public virtual bool Visible { get; set; } = true;
    public bool IsDisposed { get; private set; }
    public Size Size { get => _size; set { _size = value; OnResize(EventArgs.Empty); } }
    public int Width { get => Size.Width; set => Size = new(value, Height); }
    public int Height { get => Size.Height; set => Size = new(Width, value); }
    public virtual Size ClientSize { get => Size; set => Size = value; }
    public Size MaximumSize { get; set; }
    public Point Location { get; set; }
    public int Left => Location.X;
    public int Top => Location.Y;
    public Rectangle Bounds { get => new(Location, Size); set { Location = value.Location; Size = value.Size; } }
    public Rectangle ClientRectangle => new(Point.Empty, Size);
    public int DeviceDpi => 96;
    public Font Font { get; set; } = LabPlusTheme.Ui(10);
    public Color BackColor { get; set; } = LabPlusTheme.Panel;
    public Color ForeColor { get; set; } = LabPlusTheme.Text;
    public DockStyle Dock { get; set; }
    public AnchorStyles Anchor { get; set; }
    public Padding Padding { get; set; }
    public Padding Margin { get; set; }
    public bool AutoSize { get; set; }
    public bool AutoEllipsis { get; set; }
    public bool UseMnemonic { get; set; } = true;
    public bool TabStop { get; set; }
    public string AccessibleName { get; set; } = "";
    public string AccessibleDescription { get; set; } = "";
    public string? ToolTipText { get; set; }
    public ContentAlignment TextAlign { get; set; }
    public BorderStyle BorderStyle { get; set; }
    public bool DoubleBuffered { get; set; }
    public bool Capture { get; set; }
    public bool ContainsFocus => false;
    public Cursor Cursor { get; set; } = Cursors.Default;
    internal int GridRow, GridColumn, GridSpan = 1;
    public event EventHandler? TextChanged;
    public event EventHandler? Invalidated;
    public event KeyEventHandler? KeyDown;
    protected virtual void OnKeyDown(KeyEventArgs e) => KeyDown?.Invoke(this, e);
    protected virtual void OnResize(EventArgs e) => Invalidate();
    protected virtual void OnCreateControl() { }
    protected virtual void OnMouseDown(MouseEventArgs e) { }
    protected virtual void OnMouseUp(MouseEventArgs e) { }
    protected virtual void OnMouseMove(MouseEventArgs e) { }
    protected virtual void OnMouseWheel(MouseEventArgs e) { }
    protected virtual void OnMouseClick(MouseEventArgs e) { }
    protected virtual void OnMouseEnter(EventArgs e) { }
    protected virtual void OnMouseCaptureChanged(EventArgs e) { }
    public void Invalidate() => Invalidated?.Invoke(this, EventArgs.Empty);
    public void Refresh()=>Invalidate();
    public void Focus() { }
    public void BringToFront() { }
    protected void SetStyle(ControlStyles _, bool __) { }
    public void BeginUpdate() { }
    public void EndUpdate() { }
    public LabPlusForm? FindForm() => this as LabPlusForm ?? Parent?.FindForm();
    public void BeginInvoke(Action action) => WorkspaceWindows.Dispatch(action);
    public void DispatchKey(KeyEventArgs e) => OnKeyDown(e);
    public void Dispose() { if (IsDisposed) return; IsDisposed = true; Dispose(true); GC.SuppressFinalize(this); }
    protected virtual void Dispose(bool disposing) { if (disposing) foreach (var child in Controls.ToArray()) child.Dispose(); }
}

internal sealed class UiChildren(UiNode owner) : Collection<UiNode>
{
    protected override void InsertItem(int index, UiNode item) { item.Parent?.Controls.Remove(item); item.Parent = owner; base.InsertItem(index, item); }
    protected override void RemoveItem(int index) { this[index].Parent = null; base.RemoveItem(index); }
    protected override void ClearItems() { foreach (var item in this) item.Parent = null; base.ClearItems(); }
    public void Add(UiNode item, int column, int row) { item.GridColumn=column; item.GridRow=row; Add(item); }
    public void AddRange(UiNode[] items) { foreach (var item in items) Add(item); }
}
internal class UiStack : UiNode;
internal class UiToolbar : UiNode { public bool WrapContents { get; set; } = true; public FlowDirection FlowDirection { get; set; } }
internal class UiGrid : UiNode
{
    public int ColumnCount { get; set; } = 1;
    public int RowCount { get; set; } = 1;
    public List<ColumnStyle> ColumnStyles { get; } = [];
    public List<RowStyle> RowStyles { get; } = [];
    public int GetRow(UiNode n) => n.GridRow;
    public int GetColumn(UiNode n) => n.GridColumn;
    public int GetColumnSpan(UiNode n) => n.GridSpan;
    public void SetColumnSpan(UiNode n,int span) => n.GridSpan=span;
    public void SetRowSpan(UiNode n,int span) { }
}
internal class UiSplit : UiNode
{
    public UiStack Panel1 { get; } = new(); public UiStack Panel2 { get; } = new();
    public UiSplit() { Controls.AddRange([Panel1,Panel2]); Size = new(1000,400); }
    public Orientation Orientation { get; set; } = Orientation.Vertical;
    public int SplitterDistance { get; set; } = 250;
    public int Panel1MinSize { get; set; }
    public int Panel2MinSize { get; set; }
    public FixedPanel FixedPanel { get; set; }
}
internal class UiGroup : UiStack;
internal class UiTabs : UiNode
{
    public List<UiTab> TabPages { get; }=[];
    public int SelectedIndex { get; set; }
    public int TabCount=>TabPages.Count;
    public UiTab? SelectedTab=>TabPages.ElementAtOrDefault(SelectedIndex);
}
internal class UiTab : UiStack;
internal class UiText : UiNode;
internal class UiAction : UiNode
{
    public event EventHandler? Click;
    public DialogResult DialogResult { get; set; }
    public bool Primary { get; set; }
    public FlatStyle FlatStyle { get; set; }
    public UiActionStyle FlatAppearance { get; } = new();
    public bool UseVisualStyleBackColor { get; set; }
    public void PerformClick() { if (!Enabled || IsDisposed) return; Click?.Invoke(this,EventArgs.Empty); if (DialogResult != DialogResult.None) FindForm()?.CloseWithResult(DialogResult); }
}
internal sealed class UiActionStyle { public int BorderSize { get; set; } public Color BorderColor { get; set; } public Color MouseOverBackColor { get; set; } }
internal class UiToggle : UiNode
{
    private bool _checked;
    public bool Checked { get => _checked; set { if(_checked==value)return;_checked=value;OnCheckedChanged(EventArgs.Empty); } }
    public event EventHandler? CheckedChanged;
    protected virtual void OnCheckedChanged(EventArgs e) => CheckedChanged?.Invoke(this,e);
}
internal class UiChoice : UiToggle
{
    protected override void OnCheckedChanged(EventArgs e) { if(Checked&&Parent is { } parent)foreach(var item in parent.Controls.OfType<UiChoice>().Where(x=>x!=this))item.Checked=false;base.OnCheckedChanged(e); }
}
internal class UiInput : UiNode
{
    public bool Multiline { get; set; }
    public bool ReadOnly { get; set; }
    public bool WordWrap { get; set; } = true;
    public int MaxLength { get; set; } = 32767;
    public string PlaceholderText { get; set; } = "";
    public ScrollBars ScrollBars { get; set; }
    public void Clear() => Text="";
    public void AppendText(string text) => Text+=text;
}
internal class UiNumber : UiNode
{
    private decimal _value;
    private decimal _minimum,_maximum=100;
    public decimal Minimum { get=>_minimum;set { _minimum=value;_maximum=Math.Max(_maximum,value);Value=_value; } }
    public decimal Maximum { get=>_maximum;set { _maximum=value;_minimum=Math.Min(_minimum,value);Value=_value; } }
    public decimal Increment { get; set; } = 1;
    public decimal Value { get=>_value; set { value=Math.Clamp(value,Minimum,Maximum);if(_value==value)return;_value=value;ValueChanged?.Invoke(this,EventArgs.Empty); } }
    public event EventHandler? ValueChanged;
}
internal class UiRange : UiNode
{
    private int _value;
    private int _minimum,_maximum=100;
    public int Minimum { get=>_minimum;set { _minimum=value;_maximum=Math.Max(_maximum,value);Value=_value; } }
    public int Maximum { get=>_maximum;set { _maximum=value;_minimum=Math.Min(_minimum,value);Value=_value; } }
    public int SmallChange { get; set; } = 1;
    public int LargeChange { get; set; }=10;
    public TickStyle TickStyle { get; set; }
    public int Value { get=>_value; set { value=Math.Clamp(value,Minimum,Maximum);if(_value==value)return;_value=value;ValueChanged?.Invoke(this,EventArgs.Empty); } }
    public event EventHandler? ValueChanged;
    public event EventHandler? Scroll;
    internal void UserValue(int value) { Value=value; Scroll?.Invoke(this,EventArgs.Empty); }
}
internal class UiList : UiNode
{
    private int _selected=-1;
    public UiItems Items { get; }
    public UiList() { Items=new(this); }
    public int SelectedIndex { get=>_selected; set { if(value < -1 || value >= Items.Count) value=-1;if(_selected==value)return;_selected=value;SelectedIndexChanged?.Invoke(this,EventArgs.Empty); } }
    public object? SelectedItem { get=>Items.ElementAtOrDefault(SelectedIndex); set=>SelectedIndex=Items.IndexOf(value!); }
    public int TopIndex { get; set; }
    public bool HorizontalScrollbar { get; set; }
    public bool IntegralHeight { get; set; }
    public event EventHandler? SelectedIndexChanged;
    public string GetItemText(object? item)=>item?.ToString()??"";
    internal virtual void ItemsCleared() { _selected=-1; }
}
internal sealed class UiItems(UiList owner) : Collection<object>
{
    public void AddRange(object[] items) { foreach(var item in items)Add(item); }
    public int Add(object item, bool check) { Add(item);int i=Count-1;if(owner is UiCheckList list)list.SetItemChecked(i,check);return i; }
    protected override void ClearItems() { base.ClearItems();owner.ItemsCleared(); }
}
internal class UiSelect : UiList
{
    public int MaxLength { get; set; }
    public ComboBoxStyle DropDownStyle { get; set; }
    public UiSelect() { SelectedIndexChanged+=(_,_)=>{ if(SelectedIndex>=0)Text=GetItemText(SelectedItem); }; }
}
internal class UiCheckList : UiList
{
    private readonly HashSet<int> _checks=[];
    public bool CheckOnClick { get; set; }
    public IReadOnlyList<object> CheckedItems=>Items.Where((_,i)=>_checks.Contains(i)).ToArray();
    public event ItemCheckEventHandler? ItemCheck;
    public bool GetItemChecked(int i)=>_checks.Contains(i);
    public void SetItemChecked(int i,bool check) { bool old=_checks.Contains(i);if(old==check)return;var e=new ItemCheckEventArgs(i,check?CheckState.Checked:CheckState.Unchecked,old?CheckState.Checked:CheckState.Unchecked);ItemCheck?.Invoke(this,e);if(e.NewValue==CheckState.Checked)_checks.Add(i);else _checks.Remove(i); }
    internal override void ItemsCleared() { _checks.Clear();base.ItemsCleared(); }
}
internal sealed class UiToolTip : IDisposable
{
    public int AutoPopDelay { get; set; } public int InitialDelay { get; set; } public int ReshowDelay { get; set; } public bool ShowAlways { get; set; }
    public void SetToolTip(UiNode node,string? text)=>node.ToolTipText=text;
    public void Dispose() { }
}
internal abstract class UiPlot : UiNode
{
    protected UiPlot() { Size=new(800,280); }
    protected virtual void OnPaintBackground(PaintEventArgs e)=>e.Graphics.Clear(BackColor);
    protected virtual void OnPaint(PaintEventArgs e) { }
    public void DrawToBitmap(Bitmap bitmap,Rectangle rectangle) { using var g=Graphics.FromImage(bitmap);var e=new PaintEventArgs(g,rectangle);OnPaintBackground(e);OnPaint(e); }
}
