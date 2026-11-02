using LabPlus;
using LabPlus.Game;
using System.Reflection;
using System.Text;

internal static class SimpleSaveUiChecks
{
    private const BindingFlags Private = BindingFlags.NonPublic | BindingFlags.Instance;
    private static T Field<T>(LabPlusForm form, string name) => (T)form.GetType().GetField(name, Private)!.GetValue(form)!;
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    private static void Pump(Func<bool> ready)
    {
        long end = Environment.TickCount64 + 6000;
        while (!ready() && Environment.TickCount64 < end) { Application.DoEvents(); Thread.Sleep(5); }
        Check(ready(), "Saves fixture timed out");
    }
    internal static void Run(string parent, string previews)
    {
        string directory = Path.Combine(parent, "simple-save-ui"), remote = Path.Combine(directory, "Steam", "userdata", "10001", "782330", "remote"); Directory.CreateDirectory(remote);
        var location = SteamSaveLocation.Parse(remote); int process = Environment.ProcessId;
        var originals = PracticeSaveVault.RequiredFiles.ToDictionary(n => n, n => SteamSaveContainer.Seal(Encoding.UTF8.GetBytes("fixture:" + n), location.Account, n));
        foreach (string slot in new[] { "GAME-AUTOSAVE0", "GAME-AUTOSAVE2", "DLC1-AUTOSAVE5", "DLC2-AUTOSAVE8" })
        { Directory.CreateDirectory(location.SlotPath(slot)); foreach (var f in originals) File.WriteAllBytes(Path.Combine(location.SlotPath(slot), f.Key), f.Value); }
        var vault = new PracticeSaveVault(directory, () => []);
        var source = vault.Capture(remote, "GAME-AUTOSAVE2", "From slot three", "Practice", build: PracticeSaveSource.ArchiveBuild("1.1"));
        var tag1 = vault.Capture(remote, "DLC1-AUTOSAVE5", "TAG1 route", "DLC", build: PracticeSaveSource.ArchiveBuild("3.0"));
        var tag2 = vault.Capture(remote, "DLC2-AUTOSAVE8", "TAG2 route", "DLC", build: PracticeSaveSource.ArchiveBuild("5.0"));
        // Old offline preferences must not reintroduce a mode switch or slot filter.
        File.WriteAllText(Path.Combine(directory, "practice-ui.json"), System.Text.Json.JsonSerializer.Serialize(new { Remote = remote, Slot = "DLC1-AUTOSAVE5", Live = false }));
        var type = Assembly.Load("labplus").GetType("LabPlus.PracticeSaveForm", true)!;
        using var form = (LabPlusForm)Activator.CreateInstance(type, directory, null, vault, null, (Func<int>)(() => process))!;
        form.Show(); Application.DoEvents(); Field<System.Windows.Forms.Timer>(form, "_timer").Stop(); type.GetMethod("UseSource", Private)!.Invoke(form, [remote]);
        var save = Field<UiAction>(form, "_save"); var load = Field<UiAction>(form, "_load"); var delete = Field<UiAction>(form, "_delete"); var grid = Field<UiTable>(form, "_grid");
        Check(save.Size == load.Size && save.Size == delete.Size && delete.Primary, "Save/Load/Delete size or style differs");
        Check(grid.RowCount == 3 && !load.Enabled && !delete.Enabled && !save.Enabled, "Initial list filtered slots, selected an archive or skipped runtime discovery");
        Check(!Descendants(form).OfType<UiSelect>().Any() && !Field<UiAction>(form, "_tools").Visible && type.GetMethod("OpenSettings", Private) is null, "Settings/mode picker or tools remain visible");
        Check(Field<UiAction>(form, "_chooseFolder").Icon == "folder", "Folder button does not use Viewer folder icon");
        string stem = Path.Combine(directory, $"practice-live-{process}"), request = stem + ".request";
        Task Call(string method) => (Task)type.GetMethod(method, Private)!.Invoke(form, null)!;
        string[] Request(string op) { Pump(() => File.Exists(request) && File.ReadAllText(request).Contains("\t" + op + "\t")); return File.ReadAllText(request).TrimEnd().Split('\t'); }
        void Reply(string token, string phase) => File.WriteAllText(stem + ".tsv", $"#LABPLUS_LIVE_PRACTICE_V2\t{process}\t{token}\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\t{phase}\t{location.Account}\tGAME-AUTOSAVE0\tgame/sp/e1m1_intro/e1m1_intro\tcp_01a\tFixture result\t1.1\n");
        var status = Call("RefreshRuntime"); var h = Request("status"); Check(h[6] == "*", "Status did not discover current slot"); Reply(h[2], "ready"); Pump(() => status.IsCompleted); status.GetAwaiter().GetResult();
        Check(save.Enabled && Field<string>(form, "_slot") == "GAME-AUTOSAVE0", "In-game default did not resolve current slot");
        int Row(string name) => Enumerable.Range(0, grid.RowCount).Single(i => grid.Rows[i].Cells[0].Value?.ToString() == name);
        Check(grid.Rows[Row(source.Name)].Cells[1].Value?.ToString() == "Main", "Main campaign label is incorrect");
        grid.CurrentCell = grid.Rows[Row(source.Name)].Cells[0]; Check(load.Enabled && delete.Enabled, "Archive from slot 3 cannot load into slot 1");
        grid.CurrentCell = grid.Rows[Row(tag1.Name)].Cells[0]; Check(!load.Enabled && delete.Enabled && grid.Rows[Row(tag1.Name)].Unavailable, "TAG1 on 1.1 can load or cannot be deleted");
        grid.CurrentCell = grid.Rows[Row(tag2.Name)].Cells[0]; Check(!load.Enabled && delete.Enabled && grid.Rows[Row(tag2.Name)].Unavailable, "TAG2 on 1.1 can load or cannot be deleted");
        grid.SetSelectedRows([Row(source.Name), Row(tag1.Name)]); Check(!load.Enabled && delete.Enabled, "Multiple selection allows loading or prevents deletion");
        Field<UiInput>(form, "_search").Text = "no match"; Check(grid.RowCount == 0 && !load.Enabled && !delete.Enabled, "Search retained an invisible selection"); Field<UiInput>(form, "_search").Clear();
        grid.CurrentCell = grid.Rows[Row(source.Name)].Cells[0]; File.Delete(request);
        load.PerformClick(); h = Request("prepare"); Reply(h[2], "waiting");
        Pump(() => Field<UiText>(form, "_status").Text.StartsWith("Fixture result")); Reply(h[2], "ready"); h = Request("restore");
        Check(h[6] == "GAME-AUTOSAVE0", "Load targeted the archive's original slot instead of the current game slot");
        string bundle = Path.Combine(directory, "live-practice", "restore-pending", process.ToString(), h[2]); var plan = File.ReadAllLines(Path.Combine(bundle, "plan.tsv"));
        Check(plan[0].Split('\t')[2] == "GAME-AUTOSAVE0" && plan[0].Split('\t')[5] == source.Id, "Cross-slot load staged another archive or target");
        Reply(h[2], "loading"); Pump(() => Field<UiText>(form, "_status").Text.Contains("100%")); Reply(h[2], "reloaded"); Pump(() => load.Enabled);
        Check(Field<UiText>(form, "_status").Text.StartsWith("Loaded:"), "Load did not finish");
        File.Delete(request); var capture = Call("SaveCheckpoint"); h = Request("archive");
        Check(!save.Enabled && !delete.Enabled && !grid.Enabled, "Busy operation left actions enabled");
        string copy = Path.Combine(directory, "live-practice", process.ToString(), h[2], "slot"); Directory.CreateDirectory(copy);
        foreach (var f in originals) File.WriteAllBytes(Path.Combine(copy, f.Key), f.Value);
        Reply(h[2], "archived"); Pump(() => capture.IsCompleted); capture.GetAwaiter().GetResult();
        Check(vault.List().Count == 4 && grid.RowCount == 4 && load.Enabled, "Capture did not use live mode or select the new archive");
        Check(vault.List().Single(s => s.Id != source.Id && s.Id != tag1.Id && s.Id != tag2.Id).Category == "Uncategorized", "All-checkpoints view did not assign the default category");
        grid.SetSelectedRows([Row(source.Name), Row(tag1.Name)]);
        Check(Field<UiAction>(form, "_moveCategory").Enabled, "Multiple selection cannot move between categories");
        var transfer = (Task)type.GetMethod("TransferSelected", Private)!.Invoke(form, ["Moved route"])!; Pump(() => transfer.IsCompleted); transfer.GetAwaiter().GetResult();
        Check(grid.RowCount == 2 && grid.SelectedIndices.Count == 2 && !load.Enabled && delete.Enabled, "Transferred rows did not appear selected in their destination category");
        Check(new PracticeSaveLibrary(directory).Categorized(vault.List()).Count(s => s.Category == "Moved route") == 2, "UI transfer did not persist");
        var last = vault.List().Single(s => s.Id != source.Id && s.Id != tag1.Id && s.Id != tag2.Id);
        Task<(bool Success, string Message)> Command() => (Task<(bool, string)>)type.GetMethod("LoadLastCheckpointFromCommand", Private)!.Invoke(form, [process])!;
        File.Delete(request); var command = Command(); h = Request("status"); Reply(h[2], "ready"); h = Request("prepare"); Reply(h[2], "ready"); h = Request("restore");
        string commandPlan = Path.Combine(directory, "live-practice", "restore-pending", process.ToString(), h[2], "plan.tsv");
        Check(File.ReadAllLines(commandPlan)[0].Split('\t')[5] == last.Id && h[6] == "GAME-AUTOSAVE0", "Console command used UI selection/category instead of the last compatible archive and current slot");
        Reply(h[2], "reloaded"); Pump(() => command.IsCompleted); Check(command.GetAwaiter().GetResult().Success, "Valid console load failed");
        vault.DeleteArchives([last.Id]); File.Delete(request); command = Command(); h = Request("status"); Reply(h[2], "ready"); Pump(() => command.IsCompleted);
        var missing = command.GetAwaiter().GetResult();
        Check(!missing.Success && missing.Message.Contains("deleted") && File.ReadAllText(request).Contains("\tstatus\t"), "Deleted latest archive sent another load request or silently selected an older save");
        File.Delete(request); var menuStatus = Call("RefreshRuntime"); h = Request("status");
        File.WriteAllText(stem + ".tsv", $"#LABPLUS_LIVE_PRACTICE_V2\t{process}\t{h[2]}\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\terror\t{location.Account}\t*\t\t\tLoad the selected campaign slot first.\t1.1\n");
        Pump(() => menuStatus.IsCompleted); menuStatus.GetAwaiter().GetResult();
        Check(!save.Enabled && !load.Enabled && !grid.Rows[Row(source.Name)].Unavailable && grid.Rows[Row(tag1.Name)].Unavailable
            && Field<UiText>(form, "_connection").Text.Contains("destination"), "Main-menu target discovery failure mislabels compatible old saves as incompatible");
        foreach (var f in originals) Check(File.ReadAllBytes(Path.Combine(location.SlotPath("GAME-AUTOSAVE0"), f.Key)).SequenceEqual(f.Value), "UI fixture modified game files");
        process = 0; type.GetMethod("RefreshState", Private)!.Invoke(form, null);
        Check(!save.Enabled && !load.Enabled && delete.Enabled, "Closed-game state blocks archive deletion or permits live operations");
        form.Close();
        using var reopened = (LabPlusForm)Activator.CreateInstance(type, directory, null, vault, null, (Func<int>)(() => 0))!;
        Check(Field<UiTable>(reopened, "_grid").RowCount == 2 && !Field<UiAction>(reopened, "_save").Enabled, "Reopening lost transferred category or restored obsolete filtering");
        Console.WriteLine("PASS: Saves UI discovers the live target, restores slot 3 into slot 1, preserves all categories/slots, disables unavailable DLC and multi-load, retains Delete without a game, and shares action button sizes.");
    }
    private static IEnumerable<UiNode> Descendants(UiNode parent)
    { foreach (UiNode child in parent.Controls) { yield return child; foreach (var nested in Descendants(child)) yield return nested; } }
}
