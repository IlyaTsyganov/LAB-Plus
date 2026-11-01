using LabPlus;
using LabPlus.Configuration;
using System.Text.Json;
internal static class CompactChecks
{
    private static Task<string> Script(MainForm form,string code)=>form.Workspace!.Browser.ExecuteScriptAsync(code);
    private static async Task Sync(MainForm form){form.Workspace!.Publish(true);await Task.Delay(220);}
    private static async Task Check(MainForm form,string code,string message)
    {if(JsonSerializer.Deserialize<bool>(await Script(form,code))!=true)throw new Exception(message);}
    internal static async Task Run(MainForm main,string data,string output)
    {
        foreach(var preset in new[]{("mini",680,580),("compact",820,800),("comfortable",920,860),("compact",820,800)})
        {
            await Script(main,$"document.querySelector('[data-window-size]').value='{preset.Item1}';document.querySelector('[data-window-size]').dispatchEvent(new Event('change',{{bubbles:true}}));");await Sync(main);
            if(main.ClientSize!=new Size(preset.Item2,preset.Item3))throw new Exception("LAB window preset failed: "+preset.Item1);
        }
        await Check(main,"getComputedStyle(document.querySelector('.lr-appbar')).borderBottomWidth==='1px'","Missing header separator");
        await Check(main,"document.querySelector('[data-action=choose] .lr-folder-icon')!==null&&!document.querySelector('[data-action=choose]').innerText.includes('Browse')","Browse did not use the LAB folder icon");
        await Check(main,"(()=>{const b=document.querySelector('[data-action=types]'),t=document.querySelector('[data-setting=triggers]');return b.getBoundingClientRect().top>=t.getBoundingClientRect().bottom&&getComputedStyle(b).borderTopWidth==='1px';})()","Types is not a button beneath the trigger switch");
        foreach(string page in new[]{"Viewer","LAB"})
        {
            await Script(main,$"document.querySelector('[data-screen={page}]').click()");await Sync(main);
            await Check(main,"document.querySelector('.lr-workspace').scrollHeight<=document.querySelector('.lr-workspace').clientHeight+1",page+" does not fit Compact without scrolling");
            await Check(main,"getComputedStyle(document.querySelector('.lr-app')).borderTopWidth==='0px'","Red stripe remains");
            await Check(main,"!document.querySelector('[data-screen=Settings]')&&!document.body.innerText.includes('RESEARCH WORKSPACE')&&!document.body.innerText.includes('Diagnostics')","Removed UI remains");
        }
        await Script(main,"(()=>{for(const [key,value] of [['current.size','1.2'],['landing.size','2.3'],['vertical.size','3.4']]){const e=document.querySelector('[data-lab=\"'+key+'\"]');e.value=value;e.dispatchEvent(new Event('input',{bubbles:true}));}document.querySelector('[data-lab=verticalShowSign]').click();})()");await Sync(main);
        var hud=new LabPlus.Hud.Runtime.SettingsStore(Path.Combine(data,"hud")).Snapshot();
        if(hud.Current.Size!=1.2||hud.Landing.Size!=2.3||hud.Vertical.Size!=3.4||hud.VerticalShowSign)throw new Exception("LAB browser controls did not persist their independent settings");
        await Script(main,"document.querySelector('[data-help]').click()");await Sync(main);
        foreach(string section in new[]{"Viewer","Research","Saves","LAB","Commands"})
        {
            await Script(main,$"document.querySelector('[data-help-tab={section}]').click()");
            await Check(main,$"!document.querySelector('[data-help-panel={section}]').hidden","Help section did not open: "+section);
        }
        await Check(main,"document.querySelector('[data-help-panel=Commands]').innerText.includes('labplus_speedometer_verticalsign')&&!document.querySelector('[data-help-panel=Commands]').innerText.includes('lab_speedometer')","Wrong console prefix in Help");
        await Check(main,"(()=>{const text=document.querySelector('[data-help-panel=LAB]').innerText;return text.includes('noclip')&&text.includes('notarget')&&text.includes('god')&&!/75%|quarter|baseline|snapshot|factory|six seconds/i.test(text);})()","Help exposes internal display implementation details");
        await Check(main,"document.querySelector('[data-help-panel=Commands]').innerText.includes('labplus_debughud_scale')","Help is missing the debug HUD scale command");
        await Script(main,"document.querySelector('[data-help-close]').click();document.querySelector('[data-screen=Viewer]').click()");await Sync(main);
        var appearances = new[] { (Key:"world",Color:"#123456",Opacity:0), (Key:"triggers",Color:"#abcdef",Opacity:37),
            (Key:"walls",Color:"#fedcba",Opacity:100), (Key:"inspector",Color:"#a1b2c3",Opacity:64) };
        foreach(var appearance in appearances)
        {
            await Script(main,$"(()=>{{const c=document.querySelector('[data-appearance={appearance.Key}]');c.value='{appearance.Color}';c.dispatchEvent(new Event('change',{{bubbles:true}}));const o=document.querySelector('[data-opacity={appearance.Key}]');o.value='{appearance.Opacity}';o.dispatchEvent(new Event('input',{{bubbles:true}}));}})()");await Sync(main);
        }
        var settings=new SettingsStore(data).Load();
        var actualAppearances = new[]{settings.WorldAppearance,settings.TriggerAppearance,settings.WallAppearance,settings.InspectorAppearance};
        for(int i=0;i<appearances.Length;i++)
            if(actualAppearances[i].Color!=appearances[i].Color||actualAppearances[i].FillOpacity!=appearances[i].Opacity)
                throw new Exception("Browser swapped the color or opacity for " + appearances[i].Key);
        foreach(var appearance in appearances)
            await Check(main,$"document.querySelector('[data-appearance={appearance.Key}]').value==='{appearance.Color}'&&Number(document.querySelector('[data-opacity={appearance.Key}]').value)==={appearance.Opacity}","Published UI appearance belongs to another category: "+appearance.Key);
        using(var image=File.Create(Path.Combine(output,"compact-custom-colors.png")))await main.Workspace!.Browser.CoreWebView2.CapturePreviewAsync(Microsoft.Web.WebView2.Core.CoreWebView2CapturePreviewImageFormat.Png,image);
        var privateFlags=System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic;
        var appSettings=(LabPlusSettings)typeof(MainForm).GetField("_settings",privateFlags)!.GetValue(main)!;
        var hudStore=(LabPlus.Hud.Runtime.SettingsStore)typeof(MainForm).GetField("_hudStore",privateFlags)!.GetValue(main)!;
        string previousPath=appSettings.GameExecutablePath;
        string game=Path.Combine(data,"DOOMEternalx64vk.exe");File.WriteAllBytes(game,[1,2,3]);appSettings.GameExecutablePath=game;
        typeof(MainForm).GetMethod("UpdateGameSelection",privateFlags)!.Invoke(main,null);
        hudStore.Update(s=>{s.LightingFix=false;return true;});
        var start=new System.Diagnostics.ProcessStartInfo(game){UseShellExecute=false};
        start.Environment["VK_INSTANCE_LAYERS"]="VK_LAYER_LABPLUS_lighting";
        start.Environment["LABPLUS_HUD_LIGHTING_FIX"]="1";
        typeof(MainForm).GetMethod("ConfigureLabLaunch",privateFlags)!.Invoke(main,[start]);
        if(start.Environment["LABPLUS_LIGHTING_FIX"]!="0"||start.Environment["LABPLUS_HUD_LIGHTING_FIX"]!="0"||start.Environment["VK_INSTANCE_LAYERS"]!.Contains("VK_LAYER_LABPLUS_lighting")||!File.ReadAllText(Path.Combine(hudStore.DirectoryPath,"lighting.ini")).Contains("Enabled=0"))throw new Exception("An unsupported executable inherited a lighting opt-in.");
        if(!start.Environment["VK_INSTANCE_LAYERS"]!.Contains("VK_LAYER_LABPLUS_collision")||!start.Environment["VK_LAYER_PATH"]!.Split(';').Any(p=>Path.GetFullPath(p).Equals(Path.GetFullPath(LabPlus.Hud.Runtime.GameLauncher.LoaderPath(Path.Combine(data,"viewer-runtime"))),StringComparison.OrdinalIgnoreCase)))throw new Exception("Combined launch hid the collision manifest from the Vulkan loader.");
        string probe=Path.GetFullPath(Path.Combine(AppContext.BaseDirectory,"../../../../../Native/CollisionViewer/bin/LayerLoaderProbe.exe"));
        async Task<int> Probe(Dictionary<string,string?> environment,bool joint)
        {
            var probeStart=new System.Diagnostics.ProcessStartInfo(probe){UseShellExecute=false,CreateNoWindow=true,RedirectStandardOutput=true,RedirectStandardError=true};
            probeStart.ArgumentList.Add(Path.Combine(data,"viewer-runtime"));if(joint)probeStart.ArgumentList.Add("--joint");
            foreach(var entry in environment)probeStart.Environment[entry.Key]=entry.Value;
            using var process=System.Diagnostics.Process.Start(probeStart)!;
            var stdout=process.StandardOutput.ReadToEndAsync();var stderr=process.StandardError.ReadToEndAsync();
            await process.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(30));
            if(process.ExitCode==0)Console.WriteLine(await stdout);else Console.WriteLine("Probe observation: "+await stderr);
            return process.ExitCode;
        }
        var viewerOnly=start.Environment.ToDictionary(e=>e.Key,e=>e.Value);
        if(await Probe(viewerOnly,false)!=0)throw new Exception("The viewer-only launch failed in the real Vulkan loader.");
        // Exercise composition in a non-game probe. The correction remains off:
        // this executable is unsupported, no VkDevice is created and Enabled=0.
        string lighting=LabPlus.Hud.Runtime.GameLauncher.PrepareLighting(hudStore);
        var jointEnvironment=new Dictionary<string,string?>(viewerOnly);
        jointEnvironment["VK_LAYER_PATH"]=lighting+";"+viewerOnly["VK_LAYER_PATH"];
        jointEnvironment["VK_INSTANCE_LAYERS"]="VK_LAYER_LABPLUS_lighting;"+viewerOnly["VK_INSTANCE_LAYERS"];
        jointEnvironment["LABPLUS_HUD_DATA_DIRECTORY"]=hudStore.DirectoryPath;
        jointEnvironment["LABPLUS_HUD_LIGHTING_FIX"]="0";
        var oldEnvironment=new Dictionary<string,string?>(jointEnvironment);
        oldEnvironment["VK_LAYER_PATH"]=lighting;oldEnvironment["VK_INSTANCE_LAYERS"]="VK_LAYER_LABPLUS_lighting";
        if(await Probe(oldEnvironment,true)==0)throw new Exception("The previous broken search path unexpectedly loaded collision.");
        if(await Probe(jointEnvironment,true)!=0)throw new Exception("The combined layer search failed in the real Vulkan loader.");
        Console.WriteLine($"PASS: primary GPU vendor=0x{LabPlus.Game.LightingFixPolicy.PrimaryGpuVendor:x}; unsupported builds clear inherited lighting, the old search path fails and the fixed viewer/joint paths load correctly.");
        typeof(MainForm).GetMethod("RefreshProcessStatus",privateFlags)!.Invoke(main,null);await Sync(main);
        await Check(main,"(()=>{const b=document.querySelector('[data-action=launch]');b.click();return b.disabled;})()","Launch game did not disable immediately");await Sync(main);
        await Check(main,"!document.querySelector('[data-action=launch]').disabled","Refused preview launch did not restore the button");
        appSettings.GameExecutablePath=previousPath;typeof(MainForm).GetMethod("UpdateGameSelection",privateFlags)!.Invoke(main,null);typeof(MainForm).GetMethod("RefreshProcessStatus",privateFlags)!.Invoke(main,null);hudStore.Update(s=>{s.LightingFix=true;return true;});await Sync(main);
        Console.WriteLine("PASS: all LAB window presets, immediate launch feedback and child-only combined lighting environment with the disabled switch respected.");
        Console.WriteLine("PASS: Viewer and LAB fit Compact without scrolling; five Help sections; all four appearances and independent meter settings persist through the browser.");
    }
}
