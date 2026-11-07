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
        if (args.Contains("--offscreen")) { main.StartPosition = FormStartPosition.Manual; main.Location = new Point(-12000, -12000); }
        main.Shown += async (_, _) =>
        {
            try
            {
                await main.Workspace!.Ready.WaitAsync(TimeSpan.FromSeconds(30));
                if(args.Contains("--numbers-only"))
                {
                    var numberSettings = new LabPlusSettings();
                    using var numbers = new InvisibleNumbersForm(numberSettings,data,()=>new SettingsStore(data).Save(numberSettings),()=>0);
                    numbers.ShowInTaskbar=false; numbers.Show(main); await WaitReady(numbers); await Settle(numbers);
                    await BrowserChecks.Run(numbers,numberSettings,output); await Capture(numbers,output,"invisible-numbers");
                    await CheckScale(numbers,output); numbers.Close();
                    Console.WriteLine("PASS: Invisible numbers controls, rows and DPI scaling.");exit=0;return;
                }
                if(args.Contains("--finisher-only"))
                {
                    await Script(main,"document.querySelector('[data-screen=Research]').click()");await Settle(main);
                    if(JsonSerializer.Deserialize<bool>(await Script(main,"[...document.querySelectorAll('[data-tool]')].some(b=>b.dataset.tool.startsWith('Finisher Visualizer'))"))!=true)throw new Exception("Research is missing Finisher Visualizer.");
                    await Capture(main,output,"research-finisher-entry");
                    await Script(main,"[...document.querySelectorAll('[data-tool]')].find(b=>b.dataset.tool.startsWith('Finisher Visualizer')).click()");await Settle(main);
                    var flags=System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic;
                    var finisherSettings=(LabPlusSettings)typeof(MainForm).GetField("_settings",flags)!.GetValue(main)!;
                    using var finisher=(FinisherVisualizerForm)typeof(MainForm).GetField("_finisherVisualizer",flags)!.GetValue(main)!;
                    await WaitReady(finisher);await Settle(finisher);
                    await Capture(finisher,output,"finisher-glory-kill");
                    await BrowserChecks.Run(finisher,finisherSettings,output);await Capture(finisher,output,"finisher-chainsaw-guide");
                    await CheckScale(finisher,output);
                    await Script(finisher,"document.querySelector('[data-presentation=finisher-guide]').scrollIntoView({block:'start'})");await Settle(finisher);await Capture(finisher,output,"finisher-full-guide");
                    finisher.Close();exit=0;return;
                }
                if (args.Contains("--saves-only"))
                {
                    await SaveWorkflow(main, data, output); exit = 0; return;
                }
                await Settle(main); await CompactChecks.Run(main,data,output); await Capture(main, output, "main-viewer");
                var checks = new List<object>();
                checks.Add(await Inspect(main, "main-viewer"));
                foreach (string page in new[] { "Research", "Saves", "LAB", "Viewer" })
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
                if(args.Contains("--main-only")){Console.WriteLine("PASS: focused main-window acceptance completed.");exit=0;return;}
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
                    () => new InvisibleNumbersForm(settings, data, save, noGame),
                    () => new FinisherVisualizerForm(settings, data, save, noGame),
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
                Console.WriteLine("PASS: browser controls persist settings; Viewer/Research/Saves/LAB and 11 child windows rendered with the shared CSS.");
                exit = 0;
            }
            catch(Exception ex) { Console.Error.WriteLine(ex); Console.Error.WriteLine("Browser stage: " + main.Workspace?.StartupStage); if(main.Workspace?.Browser.CoreWebView2 is not null)Console.Error.WriteLine(await main.Workspace.Browser.ExecuteScriptAsync("JSON.stringify({url:location.href,ready:document.readyState,visible:document.visibilityState,files:[...document.scripts].map(s=>s.src),body:document.body.innerText})")); }
            finally { main.Close(); Application.Exit(); }
        };
        main.FormClosed+=(_,_)=>context.ExitThread();main.Show();
        Application.Run(context);WorkspaceWindows.Shutdown();return exit;
    }
    private static async Task SaveWorkflow(MainForm main, string data, string output)
    {
        string remote = Path.Combine(data, "Steam", "userdata", "10001", "782330", "remote"); Directory.CreateDirectory(remote);
        var location = LabPlus.Game.SteamSaveLocation.Parse(remote); int process = Environment.ProcessId;
        var originals = LabPlus.Game.PracticeSaveVault.RequiredFiles.ToDictionary(n => n, n => LabPlus.Game.SteamSaveContainer.Seal(System.Text.Encoding.UTF8.GetBytes("fixture:" + n), location.Account, n));
        foreach (string slot in new[] { "GAME-AUTOSAVE0", "GAME-AUTOSAVE2", "DLC1-AUTOSAVE5", "DLC2-AUTOSAVE8" })
        { Directory.CreateDirectory(location.SlotPath(slot)); foreach (var f in originals) File.WriteAllBytes(Path.Combine(location.SlotPath(slot), f.Key), f.Value); }
        var vault = new LabPlus.Game.PracticeSaveVault(data, () => []);
        var source = vault.Capture(remote, "GAME-AUTOSAVE2", "From third slot", "Practice", build: LabPlus.Game.PracticeSaveSource.ArchiveBuild("1.1"));
        vault.Capture(remote, "GAME-AUTOSAVE2", "Another checkpoint", "Practice", build: LabPlus.Game.PracticeSaveSource.ArchiveBuild("1.1"));
        vault.Capture(remote, "DLC1-AUTOSAVE5", "TAG1 checkpoint", "DLC", build: LabPlus.Game.PracticeSaveSource.ArchiveBuild("3.0"));
        vault.Capture(remote, "DLC2-AUTOSAVE8", "TAG2 checkpoint", "DLC", build: LabPlus.Game.PracticeSaveSource.ArchiveBuild("5.0"));
        var flags = System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance;
        string cloudRoot = Path.Combine(data, "prelaunch-steam");
        string cloudFolder = Path.Combine(cloudRoot, "userdata", "10001", "7", "remote"); Directory.CreateDirectory(cloudFolder);
        string cloudFile = Path.Combine(cloudFolder, "sharedconfig.vdf");
        void CloudFile(int enabled) => File.WriteAllText(cloudFile, "\"UserRoamingConfigStore\" { \"Software\" { \"Valve\" { \"Steam\" { \"CloudEnabled\" \"" + enabled + "\" } } } }");
        CloudFile(0);
        using (var beforeStart = new PracticeSaveForm(data, vault: vault, gameProcess: () => 0, cloudSettings: _ => LabPlus.Game.SteamCloudSettings.Read(cloudRoot, 10001)) { TopLevel = false })
        {
            main.Controls.Add(beforeStart); typeof(MainForm).GetField("_practiceSaves", flags)!.SetValue(main, beforeStart); beforeStart.Show();
            T Before<T>(string name) => (T)typeof(PracticeSaveForm).GetField(name, flags)!.GetValue(beforeStart)!;
            Before<System.Windows.Forms.Timer>("_timer").Stop();
            await Script(main, "document.querySelector('[data-screen=Saves]').click()"); await Settle(main);
            await (Task)typeof(PracticeSaveForm).GetMethod("RefreshCloudSettings", flags)!.Invoke(beforeStart, null)!; await Settle(main);
            if (!Before<UiText>("_cloudNotice").Text.Contains("Steam Cloud: disabled.") || Before<UiInput>("_folder").Text.Length != 0
                || Before<UiAction>("_save").Enabled || Before<UiAction>("_load").Enabled) throw new Exception("Cloud OFF must be shown before game launch or folder selection without enabling checkpoint actions");
            if (Before<UiText>("_cloudNotice").Text.Contains("(?)") || !Before<UiText>("_sourceNotice").Text.Contains("(?)")) throw new Exception("Help markers must follow individual prerequisite readiness");
            await Capture(main, output, "saves-cloud-before-game");
            CloudFile(1);
            await (Task)typeof(PracticeSaveForm).GetMethod("RefreshCloudSettings", flags)!.Invoke(beforeStart, null)!;
            if (!Before<UiText>("_cloudNotice").Text.Contains("Steam Cloud: enabled.")) throw new Exception("Prelaunch Cloud setting did not refresh");
            if (!Before<UiText>("_cloudNotice").Text.Contains("(?)")) throw new Exception("Cloud ON is missing its help marker");
            beforeStart.Close();
        }
        using var form = new PracticeSaveForm(data, vault: vault, gameProcess: () => process) { TopLevel = false };
        main.Controls.Add(form); typeof(MainForm).GetField("_practiceSaves", flags)!.SetValue(main, form); form.Show();
        T Field<T>(string name) => (T)typeof(PracticeSaveForm).GetField(name, flags)!.GetValue(form)!;
        Field<System.Windows.Forms.Timer>("_timer").Stop();
        if (Field<UiInput>("_folder").Text.Length != 0 || Field<UiAction>("_save").Enabled || Field<UiAction>("_load").Enabled) throw new Exception("First-run Saves source must be empty and blocked");
        await Script(main, "document.querySelector('[data-screen=Saves]').click()"); await Settle(main);
        await Capture(main, output, "saves-first-run");
        if (!Field<UiText>("_sourceNotice").Text.Contains("not selected") || !Field<UiText>("_cloudNotice").Text.Contains("not checked")) throw new Exception("Saves must explain each missing prerequisite");
        typeof(PracticeSaveForm).GetMethod("UseSource", flags)!.Invoke(form, [remote]);
        using (var reopened = new PracticeSaveForm(data, vault: vault, gameProcess: () => 0))
        {
            ((System.Windows.Forms.Timer)typeof(PracticeSaveForm).GetField("_timer", flags)!.GetValue(reopened)!).Stop();
            if (((UiInput)typeof(PracticeSaveForm).GetField("_folder", flags)!.GetValue(reopened)!).Text != remote) throw new Exception("Explicit Saves folder was not retained across reopen");
        }
        await Script(main, "document.querySelector('[data-screen=Saves]').click()"); await Settle(main);
        async Task Click(LabPlusForm screen, string label) => await Script(screen, $"[...document.querySelectorAll('button')].find(e=>e.textContent.trim()==={JsonSerializer.Serialize(label)}).click()");
        async Task Wait(Func<bool> ready) { for (int i = 0; i < 200 && !ready(); i++) await Task.Delay(50); if (!ready()) throw new Exception("Saves browser workflow timed out"); }
        string stem = Path.Combine(data, $"practice-live-{process}");
        async Task<string[]> Request(string op)
        {
            await Wait(() => File.Exists(stem + ".request") && File.ReadAllText(stem + ".request").Contains("\t" + op + "\t"));
            return File.ReadAllText(stem + ".request").TrimEnd().Split('\t');
        }
        void Reply(string token, string phase, string cloud = "account=1, app=0.") => File.WriteAllText(stem + ".tsv", $"#LABPLUS_LIVE_PRACTICE_V2\t{process}\t{token}\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\t{phase}\t{location.Account}\tGAME-AUTOSAVE0\tgame/sp/e1m1_intro/e1m1_intro\tcp_test\tBrowser fixture{(cloud.Length == 0 ? "" : " Steam Cloud flags: " + cloud)}\t1.1\n");
        var status = (Task)typeof(PracticeSaveForm).GetMethod("RefreshRuntime", flags)!.Invoke(form, null)!;
        var request = await Request("status"); Reply(request[2], "ready", "account=1, app=1."); await status; await Settle(main);
        if (Field<UiAction>("_save").Enabled || Field<UiAction>("_load").Enabled || !Field<UiText>("_cloudNotice").Text.Contains("enabled")) throw new Exception("Steam Cloud ON did not block Saves");
        await Capture(main, output, "saves-cloud-enabled");
        status = (Task)typeof(PracticeSaveForm).GetMethod("RefreshRuntime", flags)!.Invoke(form, null)!;
        request = await Request("status"); Reply(request[2], "ready", ""); await status; await Settle(main);
        if (Field<UiAction>("_save").Enabled || Field<UiAction>("_load").Enabled) throw new Exception("Unknown Steam Cloud state did not block Saves");
        status = (Task)typeof(PracticeSaveForm).GetMethod("RefreshRuntime", flags)!.Invoke(form, null)!;
        request = await Request("status"); Reply(request[2], "ready"); await status; await Settle(main);
        if (!Field<UiAction>("_save").Enabled) throw new Exception("Selected folder and Steam Cloud OFF did not enable Save");
        if (Field<UiText>("_cloudNotice").Text.Contains("(?)") || Field<UiText>("_sourceNotice").Text.Contains("(?)")) throw new Exception("Configured prerequisites must not show question marks");
        if (JsonSerializer.Deserialize<bool>(await Script(main, @"[...document.querySelectorAll('[data-presentation=saves-step-ready]')].find(e=>e.textContent.startsWith('Save folder:')).title.includes('C:\\Program Files (x86)\\Steam\\userdata\\<SteamID>\\782330\\remote')")) != true) throw new Exception("Save folder prerequisite is missing its path hint");
        if (!Field<UiText>("_cloudNotice").ToolTipText!.Contains("1. In Steam's top-left corner") || !Field<UiText>("_cloudNotice").ToolTipText!.Contains("4. Turn off")) throw new Exception("Cloud instructions are missing");
        string unrelated = Path.Combine(data, "unrelated-folder"); Directory.CreateDirectory(unrelated);
        string previousRequest = File.ReadAllText(stem + ".request");
        typeof(PracticeSaveForm).GetMethod("UseSource", flags)!.Invoke(form, [unrelated]); await Settle(main);
        if (Field<UiInput>("_folder").Text != unrelated
            || JsonSerializer.Deserialize<string>(await Script(main, "document.querySelector('[data-presentation=saves-folder-row] input').value")) != unrelated)
            throw new Exception("An invalid folder choice must replace the previously displayed path");
        if (!Field<UiText>("_sourceNotice").Text.Contains("invalid") || !Field<UiText>("_sourceNotice").Text.Contains("(?)")
            || Field<UiText>("_sourceNotice").Presentation != "saves-step-required" || Field<UiTable>("_grid").RowCount != 0
            || Field<UiAction>("_save").Enabled || Field<UiAction>("_load").Enabled || Field<bool>("_runtimeReady"))
            throw new Exception("An invalid folder retained the previous folder's readiness or archives");
        if (JsonSerializer.Deserialize<bool>(await Script(main, @"[...document.querySelectorAll('[data-presentation=saves-step-required]')].find(e=>e.textContent.startsWith('Save folder:')).title.includes('Steam\\userdata\\<SteamID>\\782330\\remote')")) != true)
            throw new Exception("Invalid folder guidance must be on the Save folder prerequisite");
        await Capture(main, output, "saves-invalid-folder");
        await (Task)typeof(PracticeSaveForm).GetMethod("SaveCheckpoint", flags)!.Invoke(form, null)!;
        if (File.ReadAllText(stem + ".request") != previousRequest) throw new Exception("Invalid folder sent a live save request");
        using (var reopened = new PracticeSaveForm(data, vault: vault, gameProcess: () => 0))
        {
            ((System.Windows.Forms.Timer)typeof(PracticeSaveForm).GetField("_timer", flags)!.GetValue(reopened)!).Stop();
            if (((UiInput)typeof(PracticeSaveForm).GetField("_folder", flags)!.GetValue(reopened)!).Text != unrelated
                || ((UiAction)typeof(PracticeSaveForm).GetField("_save", flags)!.GetValue(reopened)!).Enabled)
                throw new Exception("Reopening must retain the invalid choice visibly without enabling saving");
        }
        typeof(PracticeSaveForm).GetMethod("UseSource", flags)!.Invoke(form, [remote]);
        status = (Task)typeof(PracticeSaveForm).GetMethod("RefreshRuntime", flags)!.Invoke(form, null)!;
        request = await Request("status"); Reply(request[2], "ready"); await status; await Settle(main);
        if (!Field<UiAction>("_save").Enabled || Field<UiText>("_sourceNotice").Text.Contains("(?)"))
            throw new Exception("A corrected save folder did not recover readiness and clear its help marker");
        async Task Row(string name) { await Script(main, $"[...document.querySelectorAll('[data-presentation=saves-table] tbody tr')].find(r=>r.cells[1].textContent.includes({JsonSerializer.Serialize(name)})).cells[1].click()"); await Settle(main); }
        await Row(source.Name); await Click(main, "Load"); request = await Request("prepare"); Reply(request[2], "ready"); request = await Request("restore");
        if (request[6] != "GAME-AUTOSAVE0") throw new Exception("Browser load targeted source slot 3 instead of current slot 1");
        string bundle = Path.Combine(data, "live-practice", "restore-pending", process.ToString(), request[2]);
        if (File.ReadAllLines(Path.Combine(bundle, "plan.tsv"))[0].Split('\t')[5] != source.Id) throw new Exception("Browser staged another checkpoint");
        Reply(request[2], "loading"); await Wait(() => Field<UiText>("_status").Text.Contains("100%")); Reply(request[2], "reloaded");
        await Wait(() => Field<UiAction>("_load").Enabled); await Settle(main);
        await Row("TAG1 checkpoint"); if (Field<UiAction>("_load").Enabled || !Field<UiAction>("_delete").Enabled) throw new Exception("TAG1 on 1.1 can load or cannot be deleted");
        await Row("TAG2 checkpoint"); if (Field<UiAction>("_load").Enabled || !Field<UiAction>("_delete").Enabled) throw new Exception("TAG2 on 1.1 can load or cannot be deleted");
        if (!JsonSerializer.Deserialize<bool>(await Script(main, "document.querySelector('[data-presentation=saves-table] tr.unavailable input').disabled===false"))) throw new Exception("Unavailable DLC row cannot be selected");
        // Real browser checkboxes must reach the controller, including select all.
        await Script(main, "document.querySelector('[data-presentation=saves-table] thead input').click()"); await Settle(main);
        if (Field<UiTable>("_grid").SelectedIndices.Count != 4) throw new Exception("Select all did not reach the controller");
        await Script(main, "document.querySelector('[data-presentation=saves-table] thead input').click()"); await Settle(main);
        string list = "[...document.querySelectorAll('[data-presentation=saves-table] tbody tr')].map(r=>r.cells[1].childNodes[0].textContent)";
        string[] before = JsonSerializer.Deserialize<string[]>(await Script(main, list))!;
        await Script(main, "document.querySelectorAll('[data-presentation=saves-table] tbody input')[0].click()"); await Settle(main);
        await Script(main, "document.querySelectorAll('[data-presentation=saves-table] tbody input')[1].click()"); await Settle(main);
        if (Field<UiAction>("_load").Enabled || !Field<UiAction>("_delete").Enabled) throw new Exception("Multi-selection allows load or prevents deletion");
        await Click(main, "Down"); await Settle(main);
        string[] after = JsonSerializer.Deserialize<string[]>(await Script(main, list))!;
        if (!after.Take(3).SequenceEqual(new[] { before[2], before[0], before[1] })) throw new Exception("Browser reorder lost selected block order");
        await Click(main, "Move to…"); await Settle(main);
        if (JsonSerializer.Deserialize<bool>(await Script(main, "document.querySelector('[data-presentation=saves-category-menu]').innerText.includes('All checkpoints')"))) throw new Exception("Aggregate view offered as transfer destination");
        await Script(main, "[...document.querySelectorAll('[data-presentation=saves-category-menu] button')].find(e=>e.textContent.startsWith('DLC (')).click()"); await Settle(main);
        var moved = new LabPlus.Game.PracticeSaveLibrary(data).Categorized(vault.List());
        if (Field<UiTable>("_grid").SelectedIndices.Count != 2 || moved.Count(s => s.Category == "DLC") < 2) throw new Exception("Browser batch category transfer lost selection or persistence");
        await Click(main, "Categories"); await Settle(main);
        await Script(main, "[...document.querySelectorAll('[data-presentation=saves-category-menu] button')].find(e=>e.textContent.startsWith('All checkpoints (')).click()"); await Settle(main);
        await Script(main, "document.querySelectorAll('[data-presentation=saves-table] tbody input')[1].click()"); await Settle(main);
        await Script(main, "document.querySelectorAll('[data-presentation=saves-table] tbody input')[2].click()"); await Settle(main);
        var modal = new Func<LabPlusForm?>(() => Application.OpenForms.OfType<WorkspaceHost>().Select(h => h.Screen).FirstOrDefault(s => s is WorkspaceMessageForm));
        int count = vault.List().Count; await Click(main, "Delete"); await Wait(() => modal()?.Workspace?.Ready.IsCompletedSuccessfully == true);
        var review = modal()!; await Settle(review); await Click(review, "Cancel"); await Wait(() => !Field<bool>("_busy"));
        if (vault.List().Count != count) throw new Exception("Cancelled deletion removed archives");
        await Click(main, "Delete"); await Wait(() => modal()?.Workspace?.Ready.IsCompletedSuccessfully == true); review = modal()!;
        await Settle(review); await Click(review, "Delete"); await Wait(() => vault.List().Count == count - 2 && !Field<bool>("_busy")); await Settle(main);
        if (Field<UiAction>("_delete").Enabled) throw new Exception("Deleted entries remain selected");
        // Categories are in the main screen; no settings/mode dialog is involved.
        await Click(main, "Categories"); await Settle(main);
        await Script(main, "(()=>{const e=document.querySelector('input[placeholder=\"New category\"]');e.value='My route';e.dispatchEvent(new Event('change',{bubbles:true}));})()"); await Settle(main);
        await Click(main, "+"); await Settle(main);
        if (Field<UiTable>("_grid").RowCount != 0 || new LabPlus.Game.PracticeSaveLibrary(data).Category != "My route") throw new Exception("Creating/selecting an empty category failed");
        File.Delete(stem + ".request"); await Click(main, "Save"); request = await Request("archive");
        string copy = Path.Combine(data, "live-practice", process.ToString(), request[2], "slot"); Directory.CreateDirectory(copy);
        foreach (var f in originals) File.WriteAllBytes(Path.Combine(copy, f.Key), f.Value); Reply(request[2], "archived");
        await Wait(() => vault.List().Any(s => s.Category == "My route") && !Field<bool>("_busy")); await Settle(main);
        await Click(main, "Move to…"); await Settle(main);
        await Script(main, "(()=>{const e=document.querySelector('input[placeholder=\"New category\"]');e.value='Moved route';e.dispatchEvent(new Event('change',{bubbles:true}));})()"); await Settle(main);
        await Click(main, "+"); await Settle(main);
        var transferred = new LabPlus.Game.PracticeSaveLibrary(data);
        if (transferred.Category != "Moved route" || Field<UiTable>("_grid").RowCount != 1 || transferred.Categorized(vault.List()).Count(s => s.Category == "Moved route") != 1) throw new Exception("Browser transfer into a new category failed");
        if (!JsonSerializer.Deserialize<bool>(await Script(main, "(()=>{const b=['Save','Load','Delete'].map(t=>[...document.querySelectorAll('button')].find(e=>e.textContent.trim()===t));const r=b.map(e=>e.getBoundingClientRect());return r.every(e=>Math.abs(e.width-r[0].width)<1&&Math.abs(e.height-r[0].height)<1)})()"))) throw new Exception("Save/Load/Delete rendered sizes differ");
        string audit = await Script(main, "JSON.stringify([...document.querySelectorAll('#labplus-refined *')].filter(e=>e.getClientRects().length&&([...e.childNodes].some(n=>n.nodeType===3&&n.textContent.trim())||e.matches('input'))).map(e=>getComputedStyle(e).fontFamily))");
        if (JsonSerializer.Deserialize<string[]>(JsonSerializer.Deserialize<string>(audit)!)!.Any(f => f is not ("\"LP UI\"" or "\"LP Battle\""))) throw new Exception("Non-asset font in Saves");
        if (JsonSerializer.Deserialize<bool>(await Script(main, "(()=>{const root=document.querySelector('[data-presentation=saves-layout]');return !!root.querySelector('details')||root.innerText.includes('Tools and diagnostics')||!!root.querySelector('select')})()"))) throw new Exception("Details, tools or mode picker visible in Saves");
        if (!JsonSerializer.Deserialize<bool>(await Script(main, "!!document.querySelector('[data-presentation=saves-folder-row] .lr-folder-icon')"))) throw new Exception("Folder icon differs from Viewer");
        await Inspect(main, "saves-library"); await Capture(main, output, "saves-library"); await CheckScale(main, output);
        foreach (var f in originals) if (!File.ReadAllBytes(Path.Combine(location.SlotPath("GAME-AUTOSAVE0"), f.Key)).SequenceEqual(f.Value)) throw new Exception("UI fixtures changed the game slot");
        Console.WriteLine("PASS: real main Saves browser: live current-slot protocol, slot 3 to 1 staging, DLC restrictions, multi-select, ordered moves, cancel/confirm bulk delete, categories, save, equal button sizes, asset fonts, folder icon, hidden tools and 125/150% scaling.");
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
        if(JsonSerializer.Deserialize<bool>(await Script(form,"document.activeElement.closest('.window-actions')!==null")))throw new Exception("Window activation focused the minimize/maximize controls in "+name);
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
