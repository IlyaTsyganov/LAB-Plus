using LabPlus;
using LabPlus.Configuration;
using Microsoft.Web.WebView2.Core;
using System.Text.Json;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        string output = Path.GetFullPath(args.Length > 0 ? args[0] : "build/visual-qa"); Directory.CreateDirectory(output);
        string data = Path.Combine(output, "isolated-data");
        Environment.SetEnvironmentVariable("LABPLUS_PREVIEW", "1"); Environment.SetEnvironmentVariable("LABPLUS_DATA_DIRECTORY", data);
        Environment.SetEnvironmentVariable("LABPLUS_STATE_TESTS", null);
        BrowserChecks.Seed(data);
        new SettingsStore(data).Save(new LabPlusSettings { CollisionViewerEnabled = true, Style = RenderStyle.Outlines, ViewDistanceMeters = 100, ShowInvisibleWalls = false });
        Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
        Application.SetUnhandledExceptionMode(UnhandledExceptionMode.ThrowException);
        WorkspaceWindows.Initialize();
        var startupErrors=new List<Exception>();WorkspaceWindows.StartupFailed=ex=>{startupErrors.Add(ex);Console.Error.WriteLine(ex);};
        using var context=new ApplicationContext();
        int exit = 1;
        using var main = new MainForm { ShowInTaskbar = false };
        main.Shown += async (_, _) =>
        {
            try
            {
                await main.Workspace!.Ready.WaitAsync(TimeSpan.FromSeconds(30));
                await Settle(main); await Capture(main, output, "main-viewer");
                var checks = new List<object>();
                checks.Add(await Inspect(main, "main-viewer"));
                foreach (string page in new[] { "Research", "Saves", "Settings", "Viewer" })
                { await Script(main, $"document.querySelector('[data-screen={page}]').click()"); await Settle(main); checks.Add(await Inspect(main, "main-" + page.ToLowerInvariant())); await Capture(main, output, "main-" + page.ToLowerInvariant()); }
                await Script(main, "document.querySelector('[data-setting=walls]').click()"); await Settle(main);
                if (!new SettingsStore(data).Load().ShowInvisibleWalls) throw new Exception("Browser switch did not persist through the C# settings handler.");
                await Script(main, "document.querySelector('[data-setting=distance]').value='145'; document.querySelector('[data-setting=distance]').dispatchEvent(new Event('input',{bubbles:true}))"); await Settle(main);
                if (new SettingsStore(data).Load().ViewDistanceMeters != 145) throw new Exception("Browser range did not reach C#.");
                await Script(main,"document.querySelector('[data-setting=enabled]').click()");await Settle(main);
                if(JsonSerializer.Deserialize<bool>(await Script(main,"[...document.querySelectorAll('[data-setting]:not([data-setting=enabled]),[data-style],[data-action=types]')].some(e=>!e.disabled)")))throw new Exception("Master viewer switch left dependent browser controls enabled.");
                await Script(main,"chrome.webview.postMessage({kind:'setting',name:'distance',value:20})");await Settle(main);
                if(new SettingsStore(data).Load().ViewDistanceMeters!=145)throw new Exception("Disabled browser control changed settings.");
                await Script(main,"document.querySelector('[data-setting=enabled]').click()");await Settle(main);
                await CheckScale(main, output);
                var beforeResize=main.Bounds;
                await Script(main,"chrome.webview.postMessage({kind:'resize',phase:'start',edge:'se',x:100,y:100})");await Task.Delay(80);
                await Script(main,"chrome.webview.postMessage({kind:'resize',phase:'move',x:140,y:120})");await Task.Delay(80);
                await Script(main,"chrome.webview.postMessage({kind:'resize',phase:'end'})");await Task.Delay(80);
                if(main.Width<=beforeResize.Width||main.Height<=beforeResize.Height)throw new Exception("Borderless window resize did not update its bounds.");
                main.Bounds=beforeResize;await Settle(main);
                var settings = new LabPlusSettings(); Action save = () => { };
                Func<int> noGame = () => 0;
                using(var cancelled=new ResearchForm(settings,data,save,noGame)){cancelled.Show(main);cancelled.Close();await Task.Delay(150);}
                var windows = new Func<LabPlusForm>[] {
                    () => new ResearchForm(settings, data, save, noGame),
                    () => new MovementLabForm(settings, data, save, noGame),
                    () => new EntityGraphForm(data, noGame),
                    () => new LevelStateForm(data, noGame),
                    () => new ActiveEncountersForm(settings, data, save, noGame),
                    () => new ResearchPointsForm(data, noGame),
                    () => new SupportPlaneForm(settings, save),
                    () => new TriggerTypesForm(settings, Path.Combine(data,"trigger-classes.txt"), save),
                    () => new PracticeSaveForm(data, gameProcess:noGame),
                    () => new PracticeSaveAdvancedForm(data, gameProcess:noGame),
                    () => new WorkspaceMessageForm("This is a local preview of the shared LAB Plus confirmation window.\nNo game action has been requested.","Confirmation",MessageBoxButtons.OKCancel)
                };
                foreach (var create in windows)
                {
                    using var form = create(); form.ShowInTaskbar = false; form.Show(main);
                    await WaitReady(form); await Settle(form);
                    await BrowserChecks.Run(form, settings, output); await Settle(form);
                    checks.Add(await Inspect(form, form.GetType().Name)); await Capture(form, output, form.GetType().Name);
                    await CheckScale(form, output); form.Close();
                }
                if(startupErrors.Count>0)throw new AggregateException(startupErrors);
                File.WriteAllText(Path.Combine(output, "dom-checks.json"), JsonSerializer.Serialize(checks, new JsonSerializerOptions { WriteIndented = true }));
                Console.WriteLine("PASS: browser controls persist settings; Viewer/Research/Saves/Settings and 11 child windows rendered with the shared CSS.");
                exit = 0;
            }
            catch(Exception ex) { Console.Error.WriteLine(ex); Console.Error.WriteLine("Browser stage: " + main.Workspace?.StartupStage); if(main.Workspace?.Browser.CoreWebView2 is not null)Console.Error.WriteLine(await main.Workspace.Browser.ExecuteScriptAsync("JSON.stringify({url:location.href,ready:document.readyState,visible:document.visibilityState,files:[...document.scripts].map(s=>s.src),body:document.body.innerText})")); }
            finally { main.Close(); Application.Exit(); }
        };
        main.FormClosed+=(_,_)=>context.ExitThread();main.Show();
        Application.Run(context);WorkspaceWindows.Shutdown();return exit;
    }
    private static async Task Settle(LabPlusForm form)
    { form.Workspace!.Publish(true); await Task.Delay(350); }
    private static Task<string> Script(LabPlusForm form, string script) => form.Workspace!.Browser.ExecuteScriptAsync(script);
    private static async Task WaitReady(LabPlusForm form)
    {
        for(int i=0;i<600&&form.Workspace is null;i++)await Task.Delay(50);
        if(form.Workspace is null)throw new Exception("Screen presenter was not attached.");
        await form.Workspace.Ready.WaitAsync(TimeSpan.FromSeconds(30));
        for(int i=0;i<100&&form.Host?.Visible!=true;i++)await Task.Delay(10);
        if(form.Host is null||!form.Host.Visible||form.Host.Opacity!=1||!form.Host.FirstRevealWasReady||form.Host.Controls.Count!=1||form.Host.Controls[0]!=form.Workspace.Browser)throw new Exception("Screen contains native UI widgets or revealed before its first frame was ready.");
    }
    private static async Task CheckScale(LabPlusForm form, string output)
    {
        foreach (double scale in new[] {1.25,1.5})
        {
            form.Workspace!.Browser.ZoomFactor=scale; await Settle(form);
            using var json=JsonDocument.Parse(JsonSerializer.Deserialize<string>(await Script(form,"JSON.stringify([...document.querySelectorAll('.desktop-app,.lr-workspace,.research-content')].map(e=>({width:e.clientWidth,scroll:e.scrollWidth})))"))!);
            if(json.RootElement.EnumerateArray().Any(e=>e.GetProperty("scroll").GetInt32()>e.GetProperty("width").GetInt32()+1))throw new Exception("Horizontal content overflow at "+scale);
            await Capture(form,output,form.GetType().Name+"-scale-"+(int)(scale*100));
        }
        form.Workspace!.Browser.ZoomFactor=1;await Settle(form);
        Console.WriteLine("PASS: "+form.GetType().Name+" at 125% and 150% browser scale.");
    }
    private static async Task Capture(LabPlusForm form, string directory, string name)
    { using var stream = File.Create(Path.Combine(directory, name + ".png")); await form.Workspace!.Browser.CoreWebView2.CapturePreviewAsync(CoreWebView2CapturePreviewImageFormat.Png, stream); }
    private static async Task<JsonElement> Inspect(LabPlusForm form, string name)
    {
        string json = await Script(form, "JSON.stringify({width:innerWidth,height:innerHeight,scrollWidth:document.documentElement.scrollWidth,fonts:[...new Set([...document.querySelectorAll('.lr-app *')].filter(e=>e.getClientRects().length).map(e=>getComputedStyle(e).fontFamily))],gradient:getComputedStyle(document.querySelector('.lr-app')).backgroundImage,controls:document.querySelectorAll('button,input,select,textarea').length,metrics:[...document.querySelectorAll('.lr-brand,.lr-nav,.lr-heading,.lr-subtitle,.lr-panel-title,.lr-caption,.lr-toggle,.lr-range-label,.lr-range,.lr-inspector,.lr-advanced,.lr-game,.lr-statusbar')].map(e=>{const r=e.getBoundingClientRect(),s=getComputedStyle(e);return{cls:e.className,x:r.x,y:r.y,w:r.width,h:r.height,font:s.fontSize,line:s.lineHeight,fontWeight:s.fontWeight,margin:s.margin,pad:s.padding}}),panels:[...document.querySelectorAll('.lr-panel')].map(e=>({x:e.getBoundingClientRect().x,y:e.getBoundingClientRect().y,width:e.getBoundingClientRect().width,height:e.getBoundingClientRect().height}))})");
        string raw = JsonSerializer.Deserialize<string>(json)!;
        using var document = JsonDocument.Parse(raw);
        var result = document.RootElement;
        if (result.GetProperty("scrollWidth").GetInt32() > result.GetProperty("width").GetInt32()+1) throw new Exception("Horizontal overflow in " + name);
        if (!result.GetProperty("gradient").GetString()!.Contains("linear-gradient")) throw new Exception("Approved gradient missing in " + name);
        if (result.GetProperty("fonts").EnumerateArray().Any(f => f.GetString() is not ("\"LP UI\"" or "\"LP Battle\""))) throw new Exception("Foreign typeface in " + name + ": " + raw);
        var record=System.Text.Json.Nodes.JsonNode.Parse(raw)!.AsObject();
        record["screen"]=name;record["firstRevealReady"]=form.Host!.FirstRevealWasReady;record["nativeChildCount"]=form.Host.Controls.Count;
        record["prewarmed"]=form.Host.WasPrewarmed;record["prepareMilliseconds"]=form.Workspace!.PrepareMilliseconds;
        Console.WriteLine("Rendered: " + name); return JsonSerializer.Deserialize<JsonElement>(record.ToJsonString());
    }
}
