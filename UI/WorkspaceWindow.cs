using System.Diagnostics;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace LabPlus;

// A screen controller owns only data and actions. Its native host contains
// exactly one WebView2, never a second tree of native UI controls.
internal class LabPlusForm : UiNode, IWin32Window
{
    internal WorkspacePresenter? Workspace;
    internal WorkspaceHost? Host;
    private bool _opening, _closed;
    private bool _modelVisible;
    private Rectangle _bounds;
    public LabPlusForm() { _modelVisible=false; }
    public bool TopLevel { get; set; }=true;
    public bool ShowInTaskbar { get; set; }=true;
    public bool KeyPreview { get; set; }
    public bool ControlBox { get; set; }=true;
    public bool MaximizeBox { get; set; }=true;
    public bool MinimizeBox { get; set; }=true;
    public FormBorderStyle FormBorderStyle { get; set; }
    public FormStartPosition StartPosition { get; set; }=FormStartPosition.CenterParent;
    public AutoScaleMode AutoScaleMode { get; set; }
    public SizeF AutoScaleDimensions { get; set; }
    public SizeGripStyle SizeGripStyle { get; set; }
    public Size MinimumSize { get; set; }
    public Icon? Icon { get; set; }
    public UiAction? AcceptButton { get; set; }
    public UiAction? CancelButton { get; set; }
    public DialogResult DialogResult { get; set; }
    public override bool Visible { get=>_opening||_modelVisible;set=>_modelVisible=value; }
    public override Size ClientSize { get=>Host?.ClientSize??base.ClientSize;set { base.ClientSize=value;if(Host is not null)Host.ClientSize=value; } }
    public new int Width=>Host?.Width??Size.Width;
    public new int Height=>Host?.Height??Size.Height;
    public new int DeviceDpi=>Host?.DeviceDpi??96;
    public new Rectangle Bounds { get=>Host?.Bounds??(_bounds.IsEmpty?new Rectangle(Location,Size):_bounds);set { _bounds=value;Size=value.Size;Location=value.Location;if(Host is not null)Host.Bounds=value; } }
    public FormWindowState WindowState { get=>Host?.WindowState??FormWindowState.Normal;set { if(Host is not null)Host.WindowState=value; } }
    public IntPtr Handle=>Host?.Handle??IntPtr.Zero;
    public bool IsHandleCreated=>Host?.IsHandleCreated==true;
    public event EventHandler? Load;
    public event EventHandler? Shown;
    public event FormClosingEventHandler? FormClosing;
    public event FormClosedEventHandler? FormClosed;
    public void Show(LabPlusForm? owner=null)
    {
        if(IsDisposed||_opening||_modelVisible)return;
        if(!TopLevel||Environment.GetEnvironmentVariable("LABPLUS_STATE_TESTS")=="1") { _modelVisible=true;Load?.Invoke(this,EventArgs.Empty);Shown?.Invoke(this,EventArgs.Empty);return; }
        _opening=true;_ = OpenAsync(owner);
    }
    private async Task OpenAsync(LabPlusForm? owner)
    {
        try { await Prepare(owner);if(IsDisposed)return;Host!.Reveal(owner?.Host);_opening=false; }
        catch(Exception ex) { if(IsDisposed||WorkspaceWindows.Closing)NotifyClosed();else Fail(ex); }
    }
    private async Task Prepare(LabPlusForm? owner)
    {
        WorkspaceWindows.Trace("prepare "+GetType().Name);
        LabPlusTheme.Apply(this);
        Size requested=base.ClientSize;
        if(this is not MainForm and not WorkspaceMessageForm && requested.Height<740)requested.Height=740;
        Host=await WorkspaceWindows.Acquire(this is MainForm);
        if(IsDisposed) { Host.Dispose();return; }
        Host.Bind(this,requested,owner?.Host);
        Workspace=new WorkspacePresenter(this,Host);
        Load?.Invoke(this,EventArgs.Empty);
        await Workspace.Prepare();
    }
    public DialogResult ShowDialog(IWin32Window? owner=null)
    {
        if(Environment.GetEnvironmentVariable("LABPLUS_STATE_TESTS")=="1")throw new InvalidOperationException("Modal interaction belongs in browser scenario checks.");
        if(IsDisposed) return DialogResult.Cancel;
        _opening=true;
        var parent=owner as LabPlusForm;
        var task=Prepare(parent);
        // This is the normal modal message pump; browser callbacks dispatch
        // actions before entering here, so WebView2 callbacks are never nested.
        bool enabled=parent?.Host?.Enabled??true;
        if(parent?.Host is { } ownerHost)ownerHost.Enabled=false;
        try
        {
            while(!task.IsCompleted) { Application.DoEvents();Thread.Sleep(1); }
            task.GetAwaiter().GetResult();
            if(IsDisposed)return DialogResult.Cancel;
            if(parent?.Host is { } host)host.Enabled=enabled;
            Host!.Hide();Host.Opacity=1;
            var result=Host.ShowDialog(parent?.Host??owner);
            return DialogResult==DialogResult.None?result:DialogResult;
        }
        catch(Exception ex) { if(IsDisposed||WorkspaceWindows.Closing)NotifyClosed();else Fail(ex);return DialogResult.Cancel; }
        finally { _opening=false;if(parent?.Host is {IsDisposed:false} host)host.Enabled=enabled; }
    }
    public void Activate() { if(Host?.Visible==true)Host.Activate(); }
    public void Hide() { Host?.Hide();_modelVisible=false; }
    public void Close() { if(Host is {IsDisposed:false})Host.Close();else NotifyClosed(); }
    internal void CloseWithResult(DialogResult result) { DialogResult=result;if(Host is not null)Host.DialogResult=result;Close(); }
    internal void NotifyShown() { _modelVisible=true;_opening=false;Shown?.Invoke(this,EventArgs.Empty); }
    internal void NotifyClosing(FormClosingEventArgs e)=>FormClosing?.Invoke(this,e);
    internal void NotifyClosed()
    {
        if(_closed)return;_closed=true;_modelVisible=false;_opening=false;
        FormClosed?.Invoke(this,new FormClosedEventArgs(CloseReason.UserClosing));Dispose();
    }
    private void Fail(Exception ex)
    {
        if(WorkspaceWindows.StartupFailed is { } failed)failed(ex);
        else System.Windows.Forms.MessageBox.Show("The LAB Plus interface could not start.\n\n"+ex.GetBaseException().Message,"LAB Plus",MessageBoxButtons.OK,MessageBoxIcon.Error);
        NotifyClosed();
    }
    protected override void Dispose(bool disposing)
    {
        if(disposing) { Workspace?.Dispose();if(Host is {IsDisposed:false})Host.Dispose();NotifyClosed(); }
        base.Dispose(disposing);
    }
}

internal sealed class WorkspaceHost : Form
{
    internal readonly WebView2 Browser=new() {Dock=DockStyle.Fill,DefaultBackgroundColor=LabPlusTheme.Canvas,AllowExternalDrop=false};
    internal LabPlusForm? Screen;
    internal WorkspacePresenter? Presenter;
    internal readonly TaskCompletionSource Loaded=new(TaskCreationOptions.RunContinuationsAsynchronously);
    internal bool WasPrewarmed;
    internal bool FirstRevealWasReady;
    private bool _presented;
    internal WorkspaceHost(bool main)
    {
        FormBorderStyle=FormBorderStyle.None;BackColor=LabPlusTheme.Canvas;Controls.Add(Browser);
        string icon=Path.Combine(AppContext.BaseDirectory,"Assets","labplus.ico");if(File.Exists(icon))Icon=new(icon);
        Text="LAB Plus";ShowInTaskbar=main;Opacity=0;Show();
    }
    internal void Bind(LabPlusForm screen,Size size,Form? owner)
    {
        Screen=screen;Text=screen.Text;ClientSize=size;MinimumSize=screen.MinimumSize;
        StartPosition=owner is null?FormStartPosition.CenterScreen:FormStartPosition.Manual;
        if(owner is not null)Location=new(owner.Left+(owner.Width-Width)/2,owner.Top+(owner.Height-Height)/2);
    }
    protected override bool ShowWithoutActivation=>Opacity==0;
    internal void Reveal(Form? owner) { if(owner is not null)Owner=owner;Opacity=1;Presented();Activate(); }
    private void Presented()
    {
        if(_presented||Screen is null)return;
        if(Presenter?.Ready.IsCompletedSuccessfully!=true||Controls.Count!=1||Controls[0]!=Browser)throw new InvalidOperationException("An unprepared screen or a native widget was about to become visible.");
        _presented=true;FirstRevealWasReady=true;Screen.NotifyShown();
    }
    protected override void OnVisibleChanged(EventArgs e) { base.OnVisibleChanged(e);if(Visible&&Opacity==1)Presented(); }
    protected override void OnFormClosing(FormClosingEventArgs e) { Screen?.NotifyClosing(e);base.OnFormClosing(e); }
    protected override void OnFormClosed(FormClosedEventArgs e) { base.OnFormClosed(e);Screen?.NotifyClosed(); }
    protected override void Dispose(bool disposing) { if(disposing)Presenter=null;base.Dispose(disposing);if(disposing)Screen?.NotifyClosed(); }
}

internal static class WorkspaceWindows
{
    internal static Action<Exception>? StartupFailed;
    internal static void Trace(string text) { if(System.Environment.GetEnvironmentVariable("LABPLUS_PREVIEW")=="1")Console.WriteLine("Startup: "+text); }
    private static Task<CoreWebView2Environment>? _environment;
    private static readonly Queue<Task<WorkspaceHost>> Pool=[];
    private static SynchronizationContext? _dispatcher;
    private static bool _closing;
    internal static bool Closing=>_closing;
    private static readonly HashSet<WorkspaceHost> Hosts=[];
    internal static void Dispatch(Action action) { if(_dispatcher is null)throw new InvalidOperationException("UI dispatcher is not initialized.");_dispatcher.Post(_=>action(),null); }
    internal static void Initialize() { _dispatcher=SynchronizationContext.Current??new WindowsFormsSynchronizationContext();SynchronizationContext.SetSynchronizationContext(_dispatcher); }
    internal static async Task<WorkspaceHost> Acquire(bool main)
    {
        if(main) { var created=Create(true);Warm();return await created; }
        var next=Pool.Count>0?Pool.Dequeue():Create(false);bool ready=next.IsCompletedSuccessfully;Warm();var host=await next;host.WasPrewarmed=ready;return host;
    }
    private static void Warm() { if(_closing)return;while(Pool.Count<2)Pool.Enqueue(Create(false)); }
    private static async Task<WorkspaceHost> Create(bool main)
    {
        var host=new WorkspaceHost(main);
        Hosts.Add(host);
        host.Disposed+=(_,_)=>Hosts.Remove(host);
        try
        {
            _environment??=Environment();
            await host.Browser.EnsureCoreWebView2Async(await _environment);
            Trace(main?"main controller":"pooled controller");
            var core=host.Browser.CoreWebView2;
            core.SetVirtualHostNameToFolderMapping("app.labplus.local",Path.Combine(AppContext.BaseDirectory,"Assets"),CoreWebView2HostResourceAccessKind.DenyCors);
            core.Settings.AreDefaultContextMenusEnabled=false;core.Settings.AreDevToolsEnabled=System.Environment.GetEnvironmentVariable("LABPLUS_PREVIEW")=="1";
            core.Settings.IsStatusBarEnabled=false;core.Settings.IsZoomControlEnabled=false;core.Settings.IsNonClientRegionSupportEnabled=true;
            core.Settings.IsGeneralAutofillEnabled=false;core.Settings.IsPasswordAutosaveEnabled=false;
            core.PermissionRequested+=(_,e)=>e.State=CoreWebView2PermissionState.Deny;
            core.NewWindowRequested+=(_,e)=>e.Handled=true;
            core.NavigationStarting+=(_,e)=>{ if(!e.Uri.StartsWith("https://app.labplus.local/",StringComparison.Ordinal))e.Cancel=true; };
            core.NavigationCompleted+=(_,e)=>{ if(!e.IsSuccess)host.Loaded.TrySetException(new IOException("Interface navigation failed: "+e.WebErrorStatus)); };
            core.WebMessageReceived+=(_,e)=>
            {
                if(!e.Source.StartsWith("https://app.labplus.local/",StringComparison.Ordinal))return;
                using var json=System.Text.Json.JsonDocument.Parse(e.WebMessageAsJson);
                if(json.RootElement.GetProperty("kind").GetString()=="ready") { Trace(main?"main assets ready":"pooled assets ready");host.Loaded.TrySetResult(); }
                else host.Presenter?.Message(e);
            };
            core.Navigate("https://app.labplus.local/ui/"+(main?"main.html":"window.html"));
            await host.Loaded.Task.WaitAsync(TimeSpan.FromSeconds(20));
            host.WasPrewarmed=!main;
            return host;
        }
        catch(Exception ex) { Trace(ex.ToString());host.Dispose();throw; }
    }
    private static Task<CoreWebView2Environment> Environment()
    {
        string data=System.Environment.GetEnvironmentVariable("LABPLUS_DATA_DIRECTORY")??Path.Combine(System.Environment.GetFolderPath(System.Environment.SpecialFolder.LocalApplicationData),"labplus");
        var options=new CoreWebView2EnvironmentOptions();
        if(System.Environment.GetEnvironmentVariable("LABPLUS_PREVIEW")=="1"&&int.TryParse(System.Environment.GetEnvironmentVariable("LABPLUS_WEBVIEW_DEBUG_PORT"),out int port)&&port is >=1024 and <=65535)options.AdditionalBrowserArguments="--remote-debugging-port="+port;
        return CoreWebView2Environment.CreateAsync(null,Path.Combine(data,"webview"),options);
    }
    internal static void Shutdown()
    {
        _closing=true;foreach(var host in Hosts.ToArray())if(!host.IsDisposed)host.Dispose();Hosts.Clear();Pool.Clear();
    }
}

internal sealed class WorkspaceApplicationContext : ApplicationContext
{
    internal WorkspaceApplicationContext(MainForm main)
    {
        WorkspaceWindows.Initialize();main.FormClosed+=(_,_)=>{WorkspaceWindows.Shutdown();ExitThread();};main.Show();
    }
}
