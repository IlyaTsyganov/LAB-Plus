using LabPlus;
using System.Drawing.Imaging;
using System.Reflection;
using System.Text;
using LabPlus.Game;

internal static class BookmarkUiChecks
{
    private const BindingFlags Private = BindingFlags.NonPublic | BindingFlags.Instance;
    private static T Field<T>(LabPlusForm form, string name) => (T)form.GetType().GetField(name, Private)!.GetValue(form)!;
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    private static void Pump(Func<bool> ready)
    {
        long deadline = Environment.TickCount64 + 6000;
        while (!ready() && Environment.TickCount64 < deadline) { Application.DoEvents(); Thread.Sleep(5); }
        Check(ready(), "Bookmark/save UI did not finish its operation");
    }
    private static IEnumerable<UiNode> Descendants(UiNode parent)
    {
        foreach (UiNode child in parent.Controls) { yield return child; foreach (var nested in Descendants(child)) yield return nested; }
    }


    internal static void Run(string parent, string previews)
    {
        string directory = Path.Combine(parent, "bookmarks-ui"); Directory.CreateDirectory(directory); int process = 1234;
        var type = Assembly.Load("labplus").GetType("LabPlus.ResearchPointsForm", true)!;
        using var form = (LabPlusForm)Activator.CreateInstance(type, directory, (Func<int>)(() => process))!;
        form.Show(); Application.DoEvents(); Field<System.Windows.Forms.Timer>(form, "_timer").Stop();
        void Poll() { var task = (Task)type.GetMethod("Poll", Private)!.Invoke(form, null)!; Pump(() => task.IsCompleted); task.GetAwaiter().GetResult(); }
        void Reply(string token, uint entity, string identity = "500:600", float x = 1)
        {
            File.WriteAllLines(BookmarkObservation.FilePath(directory, process), [
                $"#LABPLUS_POINT_V1\t{process}\t{token}\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\tready\tgame/dlc/e4m1_rig/e4m1_rig\tDOOM Eternal 6.66 Rev 3\t{identity}\tknown\tcp_01a",
                FormattableString.Invariant($"P\t0\t0\t0\t{x}\t2\t3\t0\t1\t0\t1\t0\t0\t-1\t0\t0\t1\t-3\t0\t0\t0\t0\t0\t1\t0\t-2"),
                $"E\t{entity}\t3\ttrigger/interact/{entity}\tidTrigger\tenabled\t200"]);
        }
        Field<UiInput>(form, "_name").Text = "Landing pad — before the route trick"; Field<UiSelect>(form, "_category").Text = "Any% Restricted";
        Field<UiInput>(form, "_note").Text = "Hypothesis: skirt the trigger boundary. Keep the prepared checkpoint archive separate from position-only returns.";
        Field<UiAction>(form, "_capture").PerformClick();
        string token = Field<string>(form, "_pending"); Check(!Field<UiList>(form, "_points").Enabled, "Point selection allowed while capture pending");
        Reply(new('b', 32), 100); Poll(); Check(Field<string?>(form, "_pending") == token, "Stale token satisfied point capture");
        Reply(token, 100); Poll(); var saved = Field<ResearchBookmark>(form, "_selected");
        Check(saved.Name.StartsWith("Landing pad") && saved.Observation.Body.Z == 0 && saved.Observation.Eye.Z == 3 && saved.Observation.Entities.Count == 1, "Captured point missing metadata/body/eye/selection");
        Field<UiAction>(form, "_attach").PerformClick(); Reply(Field<string>(form, "_pending"), 101, x: 50); Poll();
        saved = Field<ResearchBookmark>(form, "_selected"); Check(saved.Observation.Entities.Count == 2 && saved.Observation.Eye.X == 1, "Attaching volume changed saved location");
        Field<UiAction>(form, "_attach").PerformClick(); Reply(Field<string>(form, "_pending"), 102, "700:800"); Poll();
        Check(Field<ResearchBookmark>(form, "_selected").Observation.Entities.Count == 2 && Field<UiText>(form, "_status").Text.Contains("original point"), "Attachment crossed map instance");
        Field<UiInput>(form, "_note").Text += " Edited note."; type.GetMethod("SaveEdits", Private)!.Invoke(form, null);
        Check(new BookmarkStore(directory).Load().Single().Note.EndsWith("Edited note."), "Point edit not saved");
        Field<UiInput>(form, "_search").Text = "does not exist"; Check(Field<UiList>(form, "_points").Items.Count == 0, "Point search failed");
        Field<UiInput>(form, "_search").Clear(); Field<UiList>(form, "_points").SelectedIndex = 0;

        Field<UiAction>(form, "_capture").PerformClick(); process = 5678; Poll(); Check(Field<string?>(form, "_pending") is null && Field<UiText>(form, "_status").Text.Contains("canceled"), "Process change retained pending point");
        form.Close();
        SaveUi(directory, previews);
        SimpleSaveUiChecks.Run(directory, previews);
        Console.WriteLine("PASS: research-point UI capture/token/PID scope, persisted names/categories/notes, selected volumes, attachment map scope, search; save UI full capture/validation/linking and blocked restore guards.");
    }
    private static void SaveUi(string directory, string previews)
    {
        string remote = Path.Combine(directory, "Steam", "userdata", "10001", "782330", "remote"); Directory.CreateDirectory(remote);
        var location = SteamSaveLocation.Parse(remote); string slot = location.SlotPath("GAME-AUTOSAVE0"); Directory.CreateDirectory(slot);
        foreach (string name in PracticeSaveVault.RequiredFiles) File.WriteAllBytes(Path.Combine(slot, name), SteamSaveContainer.Seal(Encoding.UTF8.GetBytes("fixture " + name), location.Account, name));
        IReadOnlyList<string> running = []; var vault = new PracticeSaveVault(directory, () => running); PracticeSave? linked = null;
        var type = Assembly.Load("labplus").GetType("LabPlus.PracticeSaveAdvancedForm", true)!;
        using var form = (LabPlusForm)Activator.CreateInstance(type, directory, (Action<PracticeSave>)(save => linked = save), vault, null, null)!;
        form.Show(); Application.DoEvents();
        void Invoke(string name) { var task = (Task)type.GetMethod(name, Private)!.Invoke(form, null)!; Pump(() => task.IsCompleted); task.GetAwaiter().GetResult(); }
        Field<UiSelect>(form, "_remote").Text = remote; Field<UiInput>(form, "_name").Text = "Landing pad practice checkpoint";
        Field<UiInput>(form, "_category").Text = "Any% Restricted"; Field<UiInput>(form, "_map").Text = "UAC Atlantica — user annotation";
        Invoke("Inspect"); Check(Field<UiText>(form, "_status").Text.Contains("All four containers"), "Save inspection failed");
        Invoke("CaptureSlot"); Check(Field<UiTable>(form, "_grid").RowCount == 1 && vault.List().Count == 1, "Save UI did not create full archive");
        Invoke("Verify"); Check(Field<UiText>(form, "_status").Text.Contains("Verified 4"), "Save verification status incorrect");
        Invoke("Link"); Check(linked?.Name == "Landing pad practice checkpoint", "Verified archive was not linked");
        Invoke("Restore"); Check(Field<UiText>(form, "_status").Text.Contains("Steam Cloud"), "Save UI restore did not require Cloud guard");
        Field<UiToggle>(form, "_cloud").Checked = true; running = ["steam"]; Invoke("Restore");
        Check(Field<UiText>(form, "_status").Text.Contains("Close DOOM Eternal and Steam"), "Save UI allowed a running client"); running = [];
        Invoke("CaptureLive"); Check(Field<UiText>(form, "_status").Text.Contains("Invalid live checkpoint request"), "Live capture accepted a missing selected game");
        Invoke("RestoreLive"); Check(Field<UiText>(form, "_status").Text.Contains("Select the running game"), "Live restore accepted a missing selected game");
        Field<UiInput>(form, "_search").Text = "other category"; Check(Field<UiTable>(form, "_grid").RowCount == 0, "Practice-save search failed"); Field<UiInput>(form, "_search").Clear();

        form.Close();
    }
}
