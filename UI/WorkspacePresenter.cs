using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;
using System.Text.Json;

namespace LabPlus;

internal sealed class WorkspacePresenter : IDisposable
{
    internal static readonly JsonSerializerOptions Json = new() { PropertyNamingPolicy = JsonNamingPolicy.CamelCase, DefaultIgnoreCondition = System.Text.Json.Serialization.JsonIgnoreCondition.WhenWritingNull };
    private readonly LabPlusForm _form;
    private readonly WorkspaceHost _host;
    private readonly WebView2 _browser;
    private readonly ScreenStateAdapter _projection;
    private readonly System.Windows.Forms.Timer _refresh = new() { Interval = 250 };
    private readonly TaskCompletionSource _ready = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly TaskCompletionSource _rendered = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private string _lastState = "";
    private bool _disposed;
    private Rectangle? _resizeStart;
    private Point _resizePointer;
    private string _resizeEdge = "";
    internal Task Ready => _ready.Task;
    internal WebView2 Browser => _browser;
    internal ScreenStateAdapter Projection => _projection;
    internal string StartupStage = "preloaded";
    internal long PrepareMilliseconds;
    internal WorkspacePresenter(LabPlusForm form, WorkspaceHost host)
    {
        _form=form;_host=host;_browser=host.Browser;_projection=new(form);host.Presenter=this;
        _refresh.Tick+=(_,_)=>Publish();
    }
    internal async Task Prepare()
    {
        var clock=System.Diagnostics.Stopwatch.StartNew();
        WorkspaceWindows.Trace("bind "+_form.GetType().Name);Publish(true);await _rendered.Task.WaitAsync(TimeSpan.FromSeconds(20));
        WorkspaceWindows.Trace("layout ready "+_form.GetType().Name);
        // Force the first composited frame while the host is still hidden. It
        // becomes visible only after the actual UI, fonts and state are ready.
        await _browser.CoreWebView2.CallDevToolsProtocolMethodAsync("Page.captureScreenshot","{\"format\":\"png\",\"fromSurface\":true,\"optimizeForSpeed\":true,\"captureBeyondViewport\":false}").WaitAsync(TimeSpan.FromSeconds(5));
        PrepareMilliseconds=clock.ElapsedMilliseconds;StartupStage="first frame ready";
        WorkspaceWindows.Trace("frame ready "+_form.GetType().Name+" "+PrepareMilliseconds+"ms");
        _refresh.Start();_ready.TrySetResult();
    }
    internal void Message(CoreWebView2WebMessageReceivedEventArgs e)
    {
        if(_disposed)return;
        using var document=JsonDocument.Parse(e.WebMessageAsJson);
        var message=document.RootElement.Clone();
        _host.BeginInvoke((Action)(()=>HandleMessage(message)));
    }
    private void HandleMessage(JsonElement message)
    {
        if (_disposed) return;
        try
        {
            string kind = message.GetProperty("kind").GetString() ?? "";
            if (kind == "rendered") { _rendered.TrySetResult(); return; }
            if (kind == "window")
            {
                switch (message.GetProperty("action").GetString())
                {
                    case "close": _form.Close(); break;
                    case "minimize": _form.WindowState = FormWindowState.Minimized; break;
                    case "maximize": _form.WindowState = _form.WindowState == FormWindowState.Maximized ? FormWindowState.Normal : FormWindowState.Maximized; break;
                }
                return;
            }
            if (kind == "link")
            {
                string? url = message.GetProperty("name").GetString() switch { "community" => "https://discord.gg/dtDa9VZ", "wiki" => "https://github.com/IlyaTsyganov/LAB-Plus/wiki", _ => null };
                if (url is not null) System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(url) { UseShellExecute = true });
                return;
            }
            if (kind == "resize") { Resize(message); return; }
            if (kind == "key") _projection.Key(message);
            else if (kind == "control") _projection.Dispatch(message);
            else if (_form is MainForm main) main.DispatchWorkspace(message);
            Publish(true);
        }
        catch (Exception ex) when (ex is JsonException or InvalidOperationException or ArgumentException or IOException or UnauthorizedAccessException)
        { if (!_disposed) _browser.CoreWebView2.PostWebMessageAsJson(JsonSerializer.Serialize(new { kind = "error", text = ex.GetBaseException().Message }, Json)); }
    }
    private void Resize(JsonElement message)
    {
        string? phase = message.GetProperty("phase").GetString();
        if (phase == "end") { _resizeStart = null; return; }
        if (_form.WindowState != FormWindowState.Normal) return;
        double scale = _form.DeviceDpi / 96d;
        var point = new Point((int)(message.GetProperty("x").GetDouble() * scale), (int)(message.GetProperty("y").GetDouble() * scale));
        if (phase == "start")
        {
            string edge = message.GetProperty("edge").GetString() ?? "";
            if (edge is not ("n" or "s" or "w" or "e" or "nw" or "ne" or "sw" or "se")) return;
            _resizeStart = _form.Bounds; _resizePointer = point; _resizeEdge = edge; return;
        }
        if (phase != "move" || _resizeStart is not { } start) return;
        int dx = point.X - _resizePointer.X, dy = point.Y - _resizePointer.Y;
        bool west = _resizeEdge.Contains('w'), east = _resizeEdge.Contains('e'), north = _resizeEdge.Contains('n'), south = _resizeEdge.Contains('s');
        int width = Math.Max(_form.MinimumSize.Width, start.Width + (west ? -dx : east ? dx : 0));
        int height = Math.Max(_form.MinimumSize.Height, start.Height + (north ? -dy : south ? dy : 0));
        _form.Bounds = new Rectangle(west ? start.Right-width : start.X, north ? start.Bottom-height : start.Y, width, height);
    }

    internal void Publish(bool force = false)
    {
        if (_disposed || _browser.CoreWebView2 is null) return;
        object state = _form is MainForm main ? main.WorkspaceSnapshot(_projection) : new { title = ToolTitle(_form), subtitle = ToolSubtitle(_form), tool = _form.GetType().Name, dialog = _form is WorkspaceMessageForm, tree = _projection.Snapshot(_projection.Roots) };
        string text = JsonSerializer.Serialize(new { kind = "state", state }, Json);
        if (!force && text == _lastState) return;
        _lastState = text; _browser.CoreWebView2.PostWebMessageAsJson(text);
    }
    internal static string ToolTitle(LabPlusForm form) => form switch
    {
        MovementLabForm => "MOVEMENT LAB", ResearchForm => "RESEARCH TIMELINE", EntityGraphForm => "ENTITY LINKS",
        LevelStateForm => "LEVEL COMPARISON", ActiveEncountersForm => "ACTIVE ENCOUNTERS", InvisibleNumbersForm => "INVISIBLE NUMBERS", ResearchPointsForm => "RESEARCH POINTS",
        SupportPlaneForm => "SURFACE PROJECTIONS", TriggerTypesForm => "TRIGGER TYPES", PracticeSaveAdvancedForm => "ARCHIVES AND RECOVERY",
        PracticeSaveForm => "CHECKPOINT LIBRARY", _ => form.Text.Replace("LAB Plus — ", "", StringComparison.Ordinal)
    };
    internal static string ToolSubtitle(LabPlusForm form) => form switch
    {
        MovementLabForm => "Record attempts, inspect movement and compare paths.", ResearchForm => "Record paths, mark observations and review the journal.",
        EntityGraphForm => "Explore entity references and their exact fields.", LevelStateForm => "Capture and compare observations before and after an action.",
        InvisibleNumbersForm => "Living enemies, health and engine countdowns.", ActiveEncountersForm => "Inspect live encounter managers and their observed state.", ResearchPointsForm => "Keep named observations, notes and archive links.",
        SupportPlaneForm => "Inspect tangent-plane cuts and support angles.", TriggerTypesForm => "Choose classes and folders to display.",
        PracticeSaveAdvancedForm => "Validated checkpoint archives, diagnostics and recovery.", _ => ""
    };
    public void Dispose()
    {
        if (_disposed) return; _disposed = true; _refresh.Stop(); _refresh.Dispose();
        _projection.Dispose(); _ready.TrySetCanceled(); _rendered.TrySetCanceled();
    }
}
