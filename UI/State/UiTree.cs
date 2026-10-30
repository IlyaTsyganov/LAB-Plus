using System.Collections.ObjectModel;

namespace LabPlus;

internal sealed class UiTreeNode
{
    private bool _checked;
    public UiTreeNode(string text) { Text=text;Nodes=new(this); }
    public string Text { get; set; }
    public object? Tag { get; set; }
    public string ToolTipText { get; set; }="";
    public Color ForeColor { get; set; }
    public UiTreeNode? Parent { get; internal set; }
    public UiTree? TreeView { get; internal set; }
    public UiTreeNodes Nodes { get; }
    public bool Checked { get=>_checked;set { if(_checked==value)return;_checked=value;TreeView?.Checked(this); } }
    public bool IsExpanded { get; private set; }
    public void Expand()=>IsExpanded=true;
    public void Collapse()=>IsExpanded=false;
    internal void Attach(UiTree tree) { TreeView=tree;foreach(var child in Nodes)child.Attach(tree); }
}
internal sealed class UiTreeNodes : Collection<UiTreeNode>
{
    private readonly UiTreeNode? _parent;private readonly UiTree? _tree;
    public UiTreeNodes(UiTreeNode parent)=>_parent=parent;
    public UiTreeNodes(UiTree tree)=>_tree=tree;
    protected override void InsertItem(int i,UiTreeNode item) { item.Parent=_parent;var tree=_tree??_parent?.TreeView;if(tree is not null)item.Attach(tree);base.InsertItem(i,item); }
}
internal sealed class UiTree : UiNode
{
    public UiTreeNodes Nodes { get; }
    public UiTree()=>Nodes=new(this);
    public UiTreeNode? SelectedNode { get; set; }
    public UiTreeNode? TopNode { get; set; }
    public bool CheckBoxes { get; set; }
    public bool HideSelection { get; set; }
    public bool ShowNodeToolTips { get; set; }
    public bool FullRowSelect { get; set; }
    public event Action<object?,UiTreeEvent>? AfterCheck;
    internal void Checked(UiTreeNode node)=>AfterCheck?.Invoke(this,new(node));
}
internal sealed record UiTreeEvent(UiTreeNode Node);
