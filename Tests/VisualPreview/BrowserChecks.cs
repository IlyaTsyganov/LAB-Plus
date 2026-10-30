using System.Collections;
using System.Reflection;
using System.Text.Json;
using LabPlus;
using LabPlus.Configuration;
using LabPlus.Game;

internal static class BrowserChecks
{
    private const BindingFlags Private = BindingFlags.Instance | BindingFlags.NonPublic;
    private static T Field<T>(object obj, string name) => (T)obj.GetType().GetField(name, Private)!.GetValue(obj)!;
    private static void Set(object obj, string name, object value) => obj.GetType().GetField(name, Private)!.SetValue(obj, value);
    private static void Invoke(object obj, string name) => obj.GetType().GetMethod(name, Private)!.Invoke(obj, null);
    private static void Check(bool condition, string text) { if (!condition) throw new Exception(text); }
    private static Task<string> Script(LabPlusForm form, string code) => form.Workspace!.Browser.ExecuteScriptAsync(code);
    private static async Task Wait(Func<bool> ready)
    { for (int i = 0; i < 100 && !ready(); i++) await Task.Delay(50); Check(ready(), "Browser action did not complete."); }
    private static async Task Sync(LabPlusForm form) { form.Workspace!.Publish(true); await Task.Delay(220); }
    private static async Task Input(LabPlusForm form, string placeholder, string value)
    { await Script(form, $"(()=>{{const e=[...document.querySelectorAll('input')].find(e=>e.placeholder.startsWith({JsonSerializer.Serialize(placeholder)}));e.value={JsonSerializer.Serialize(value)};e.dispatchEvent(new Event('change',{{bubbles:true}}));}})()"); await Sync(form); }

    internal static void Seed(string directory)
    {
        Directory.CreateDirectory(Path.Combine(directory, "research"));
        File.WriteAllText(Path.Combine(directory, "research-current.txt"), "0-1.tsv");
        File.WriteAllLines(Path.Combine(directory, "research", "0-1.tsv"), new[] { "#LABPLUS_RESEARCH_V1\t0\t0\t0\t0-1", "sequence\ttime\tevent\tmap\tid\tgeneration\tclass\tx\ty\tz\tdetail" }
            .Concat(Enumerable.Range(1, 250).Select(i => $"{i}\t{i * 100}\tmarker\tpreview/arena\t{i}\t1\tidTrigger\t1\t2\t3\t{(i == 11 ? "Lift gate" : "Observation " + i.ToString("000"))}"))
            .Append("251\t25100\tposition\tpreview/arena\t0\t0\t\t2\t3\t4\tPlayer position"));
        var store = new MotionStore(directory);
        long epoch = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
        for (int j = 0; j < 2; j++)
        {
            var samples = Enumerable.Range(0, 35).Select(i => new MotionSample(i + 1, epoch + i * 50, 1, new(i / 5f, j + i / 12f, 1 + MathF.Sin(i / 5f)), new(3, 4, .2f), new(-.4f, -.4f, 0), new(.4f, .4f, 1.8f), true, 0, .3f, true, new(i / 5f, 1, 0), new(0, 0, 1), new(0, 0, 1), 0, 0, null, null)).ToList();
            store.Save(new MotionAttempt { Id = "0000000000000000000000000000000" + j, Name = j == 0 ? "Successful route" : "Comparison route", Process = 1234, Map = "preview/arena", Profile = "preview", Started = epoch, Finished = true, Outcome = j == 0 ? AttemptOutcome.Success : AttemptOutcome.Failure, Samples = samples });
        }
    }
    internal static async Task Run(LabPlusForm form, LabPlusSettings settings, string output)
    {
        if (form is ResearchForm research)
        {
            await Wait(() => Field<UiTable>(research, "_grid").RowCount == 250); await Sync(form);
            await Script(form, "[...document.querySelectorAll('button')].find(b=>b.textContent.startsWith('Show more')).click()"); await Sync(form);
            Check(JsonSerializer.Deserialize<int>(await Script(form, "document.querySelectorAll('tbody tr').length")) == 250, "Research pagination lost events.");
            await Input(form, "Search event", "Lift gate");
            Check(Field<UiTable>(research, "_grid").RowCount == 1, "Browser search did not filter the virtual research table.");
            await Input(form, "Search event", "");
            await Script(form, "[...document.querySelectorAll('.control-check')].find(e=>e.textContent==='Include position samples').querySelector('input').click()"); await Sync(form);
            Check(Field<UiTable>(research, "_grid").RowCount == 251, "Include samples did not update native data.");
            await Script(form, "[...document.querySelectorAll('button')].find(b=>b.textContent==='Start recording').click()");
            await Wait(() => Application.OpenForms.OfType<WorkspaceHost>().Select(h=>h.Screen).OfType<WorkspaceMessageForm>().Any());
            using var dialog = Application.OpenForms.OfType<WorkspaceHost>().Select(h=>h.Screen).OfType<WorkspaceMessageForm>().Single();
            await dialog.Workspace!.Ready.WaitAsync(TimeSpan.FromSeconds(20)); await Sync(dialog);
            Check(JsonSerializer.Deserialize<bool>(await Script(dialog, "document.body.innerText.includes('Launch the selected game through LAB Plus first.')")), "Themed modal lost its backend validation message.");
            await Script(dialog, "document.body.dispatchEvent(new KeyboardEvent('keydown',{key:'Enter',bubbles:true}))");
            await Wait(() => dialog.IsDisposed || !dialog.Visible);
            await Sync(form);
            Console.WriteLine("PASS: research pagination, native search, sample filter and modal validation/Enter.");
        }
        else if (form is TriggerTypesForm trigger)
        {
            await Input(form, "Search class", "idEncounterTrigger_Commit");
            await Script(form, "document.querySelector('.control-tree .control-check input').click()"); await Sync(form);
            Check(!settings.IsTriggerClassVisible("idEncounterTrigger_Commit"), "Tree checkbox did not persist trigger visibility.");
            await Input(form, "Search class", "");
            Console.WriteLine("PASS: trigger tree search and settings persistence.");
        }
        else if (form is MovementLabForm movement)
        {
            await Script(form, "document.querySelectorAll('.control-list input').forEach(input=>input.click())"); await Sync(form);
            Check(Field<UiCheckList>(movement, "_attempts").CheckedItems.Count == 2, "Checked attempts did not reach comparison model.");
            await Script(form, "(()=>{const e=[...document.querySelectorAll('select')].find(s=>[...s.options].some(o=>o.text==='XZ'));e.selectedIndex=1;e.dispatchEvent(new Event('change',{bubbles:true}));})()"); await Sync(form);
            Check(Field<UiSelect>(movement, "_projection").SelectedIndex == 1 && Field<UiTable>(movement, "_comparison").RowCount == 2, "Projection/comparison controls did not update.");
            Console.WriteLine("PASS: movement checked attempts, projection and populated comparison table.");
        }
        else if (form is EntityGraphForm graph) await Graph(graph);
    }
    private static async Task Graph(EntityGraphForm form)
    {
        Field<System.Windows.Forms.Timer>(form, "_timer").Stop();
        var nodes = new Dictionary<string, EntityGraphNode> { ["100:1"] = new("100:1", "Arena manager", "idEncounterManager", "") };
        var edges = new List<EntityGraphEdge>();
        for (int i = 0; i < 6; i++) { string key = $"{200 + i}:1"; nodes[key] = new(key, "Gate trigger " + (i + 1), "idTrigger", ""); edges.Add(new("100:1", key, "target", "Entity pointer")); }
        Set(form, "_snapshot", new EntityGraphSnapshot(0, DateTimeOffset.UtcNow, "live", "preview/arena", "1:2", 0, 0, 0, "100:1", nodes, edges));
        Set(form, "_selected", "100:1"); Invoke(form, "Populate"); Invoke(form, "DrawSelection"); await Sync(form);
        var canvas = Field<EntityGraphCanvas>(form, "_canvas");
        float Zoom() => Field<float>(canvas, "_zoom"); PointF Offset() => Field<PointF>(canvas, "_offset");
        float zoom = Zoom();
        await Pointer(form, canvas.Width / 2f, canvas.Height / 2f, "mouseWheel", deltaY: -120); await Sync(form);
        Check(Zoom() > zoom, "Graph wheel zoom failed through browser pointer bridge.");
        var offset = Offset();
        await Pointer(form, 100, 80, "mousePressed", "right");
        await Pointer(form, 155, 100, "mouseMoved", "right");
        await Pointer(form, 155, 100, "mouseReleased", "right"); await Sync(form);
        Check(Offset().X > offset.X + 40 && !Field<bool>(canvas, "_panning"), "Graph pan/release failed through browser pointer bridge.");
        await Script(form, "[...document.querySelectorAll('.canvas-actions button')].find(b=>b.textContent==='Fit graph').click()"); await Sync(form);
        Check(Field<bool>(canvas, "_autoFit"), "Fit graph button in canvas did not reset the view.");
        var target = ((IEnumerable)Field<object>(canvas, "_targets")).Cast<object>().First(t => (string)t.GetType().GetProperty("Key")!.GetValue(t)! == "205:1");
        var box = (RectangleF)target.GetType().GetProperty("Bounds")!.GetValue(target)!;
        await Click(form, Offset().X + (box.X + box.Width / 2) * Zoom(), Offset().Y + (box.Y + box.Height / 2) * Zoom()); await Sync(form);
        Check(Field<string>(form, "_selected") == "205:1", "Graph node selection failed through browser click.");
        Set(form, "_selected", "100:1"); Invoke(form, "DrawSelection"); await Sync(form);
        await Script(form, "document.querySelector('tbody td').click()"); await Sync(form);
        await Script(form, "document.querySelector('tbody td').dispatchEvent(new MouseEvent('dblclick',{bubbles:true}))"); await Sync(form);
        Check(Field<string>(form, "_selected") != "100:1", "Graph table double-click did not explore related entity.");
        Set(form, "_selected", "100:1"); Invoke(form, "DrawSelection"); await Sync(form);
        Console.WriteLine("PASS: populated entity graph, zoom, pan/release, fit, node selection and table double-click.");
    }
    private static async Task Click(EntityGraphForm form, float x, float y)
    { await Pointer(form, x, y, "mousePressed", "left"); await Pointer(form, x, y, "mouseReleased", "left"); }
    private static async Task Pointer(EntityGraphForm form, float x, float y, string type, string button = "none", int deltaY = 0)
    {
        string raw = JsonSerializer.Deserialize<string>(await Script(form, "JSON.stringify((()=>{const r=document.querySelector('.control-canvas img').getBoundingClientRect();return {x:r.x,y:r.y,w:r.width,h:r.height}})())"))!;
        using var document = JsonDocument.Parse(raw); var r = document.RootElement;
        var canvas = Field<EntityGraphCanvas>(form, "_canvas");
        double px = r.GetProperty("x").GetDouble() + x / canvas.Width * r.GetProperty("w").GetDouble();
        double py = r.GetProperty("y").GetDouble() + y / canvas.Height * r.GetProperty("h").GetDouble();
        await form.Workspace!.Browser.CoreWebView2.CallDevToolsProtocolMethodAsync("Input.dispatchMouseEvent", JsonSerializer.Serialize(new { type, x = px, y = py, button, buttons = type == "mouseReleased" ? 0 : button == "right" ? 2 : button == "left" ? 1 : 0, clickCount = type is "mousePressed" or "mouseReleased" ? 1 : 0, deltaX = 0, deltaY }));
        await Task.Delay(70);
    }
}
