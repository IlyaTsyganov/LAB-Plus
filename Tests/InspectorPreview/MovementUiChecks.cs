using LabPlus;
using System.Drawing.Imaging;
using System.Reflection;
using LabPlus.Configuration;
using LabPlus.Game;

internal static class MovementUiChecks
{
    private const BindingFlags Private = BindingFlags.NonPublic | BindingFlags.Instance;
    private static T Field<T>(LabPlusForm form, string name) => (T)form.GetType().GetField(name, Private)!.GetValue(form)!;
    private static void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
    private static void Pump(Func<bool> ready)
    {
        var deadline = DateTime.UtcNow.AddSeconds(4);
        while (!ready() && DateTime.UtcNow < deadline) { Application.DoEvents(); Thread.Sleep(5); }
        Check(ready(), "Movement UI did not finish loading its snapshot");
    }
    private static string Sample(long seq, long time, int segment, float x) => FormattableString.Invariant($"S\t{seq}\t{time}\t{segment}\t{x}\t2\t3\t3\t4\t0\t-0.4\t-0.4\t0\t0.4\t0.4\t1.8\t1\t0\t0.3\t1\t{x}\t2\t3\t0\t0\t1\t0\t0\t1\t7\t2\t2\t1.5");
    internal static void Run(string parentDirectory, string previewDirectory)
    {
        string directory = Path.Combine(parentDirectory, "motion-ui"); Directory.CreateDirectory(directory);
        long epoch = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() - 100;
        string file = Path.Combine(directory, "motion-1234.tsv");
        void Write(long captured, params string[] rows) => File.WriteAllLines(file, new[] { $"#LABPLUS_MOTION_V1\t1234\t{captured}\tlive\tmap_a\tprofile_a\t1:2" }.Concat(rows));
        Write(epoch, Sample(1, epoch, 1, 0));
        var store = new MotionStore(directory);
        for (int index = 0; index < 3; ++index)
        {
            var samples = Enumerable.Range(0, 40).Select(i => MotionSnapshot.Parse([$"#LABPLUS_MOTION_V1\t1234\t{epoch + i * 50}\tlive\tmap_a\tprofile_a\t1:2", Sample(i + 1, epoch + i * 50, i < 20 ? 1 : 2, i / 10f + index / 2f)])!.Samples.Single()
                with { Position = new(i / 10f, 2 + index * .25f, 3 + MathF.Sin(i / 10f) * (index + 1)) }).ToList();
            store.Save(new() { Name = index == 0 ? "Successful route" : index == 1 ? "Failed route" : "Another map", Process = 1234, Map = index == 2 ? "map_b" : "map_a", Profile = "profile_a", Started = epoch,
                Outcome = index == 0 ? AttemptOutcome.Success : AttemptOutcome.Failure, Finished = true, Samples = samples });
        }
        int process = 1234, writes = 0; var settings = new LabPlusSettings();
        var type = Assembly.Load("labplus").GetType("LabPlus.MovementLabForm", true)!;
        using var form = (LabPlusForm)Activator.CreateInstance(type, settings, directory, (Action)(() => ++writes), (Func<int>)(() => process))!;
        form.Show(); Pump(() => Field<MotionSnapshot?>(form, "_snapshot") is not null);
        Field<System.Windows.Forms.Timer>(form, "_timer").Stop();
        void Refresh()
        {
            type.GetMethod("RefreshMotion", Private)!.Invoke(form, null); Pump(() => !Field<bool>(form, "_busy"));
        }
        Field<UiToggle>(form, "_collider").Checked = true;
        Field<UiToggle>(form, "_follow").Checked = false;
        Check(settings.ShowPlayerCollider && !settings.MotionFollowTarget && settings.MotionProcess == 1234 && writes > 0, "Movement switches did not persist or scope selected PID");
        Field<UiInput>(form, "_name").Text = "New controlled attempt"; Field<UiAction>(form, "_record").PerformClick();
        Check(Field<MotionAttempt?>(form, "_active") is not null, "Start attempt did not arm manual recording");
        long live = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() + 10;
        Write(live + 50, Sample(1, epoch, 1, 0), Sample(2, live, 1, 1), Sample(3, live + 50, 1, 2)); Refresh();
        var active = Field<MotionAttempt>(form, "_active");
        Check(active.Samples.Count == 2 && active.Started == live, "Recording included pre-start samples or lost buffered current samples");
        Refresh(); Check(active.Samples.Count == 2, "Duplicate snapshot duplicated motion samples");
        Write(live + 1000, Sample(2, live, 1, 1), Sample(3, live + 50, 1, 2), Sample(4, live + 1000, 2, 4)); Refresh();
        Check(active.Samples.Count == 3 && active.Samples[2].Segment != active.Samples[1].Segment, "Pause/gap connected movement attempts");
        Field<UiSelect>(form, "_outcome").SelectedIndex = (int)AttemptOutcome.Success;
        Field<UiAction>(form, "_record").PerformClick();
        Check(Field<MotionAttempt?>(form, "_active") is null && store.Load().Any(attempt => attempt.Name == "New controlled attempt" && attempt.Finished && attempt.Outcome == AttemptOutcome.Success), "Stop/result did not save a finished attempt");
        var list = Field<UiCheckList>(form, "_attempts");
        for (int i = 0; i < list.Items.Count; ++i) list.SetItemChecked(i, true);
        Application.DoEvents(); list.SelectedItem = list.Items.Cast<MotionAttempt>().Single(attempt => attempt.Name == "Successful route");
        Field<UiRange>(form, "_time").Value = 500; Field<UiSelect>(form, "_projection").SelectedIndex = 1;
        var table = Field<UiTable>(form, "_comparison");
        Check(table.RowCount == 3 && table.Rows.Cast<UiTableRow>().All(row => row.Cells[0].Value as string != "Another map"), "Comparison mixed maps or lost checked compatible attempts");
        Check(table.Rows.Cast<UiTableRow>().Single(row => row.Cells[0].Value as string == "New controlled attempt").Cells[1].Value as string == "No sample / gap", "Comparison invented data in a gap");
        form.ClientSize = new(1100, 720); Application.DoEvents();
                SurfaceChecks(form, settings, previewDirectory);
        // New process cannot inherit a recording; previously saved attempts stay available.
        Field<UiAction>(form, "_record").PerformClick(); process = 5678; Refresh();
        Check(Field<MotionAttempt?>(form, "_active") is null && settings.MotionProcess == 5678, "New game process inherited an active attempt");
        form.Close();
        Console.WriteLine("PASS: movement UI manual recording, buffered samples, gaps, outcomes/save, PID changes, comparison scope, controls.");
    }
    private static void SurfaceChecks(LabPlusForm movement, LabPlusSettings settings, string previewDirectory)
    {
        var surfaceButton = movement.Controls.Cast<UiNode>().SelectMany(Descendants).OfType<UiAction>().Single(button => button.Text == "Surface projections…");
        surfaceButton.PerformClick(); Application.DoEvents();
        using var surface = Field<LabPlusForm>(movement, "_surface");
        var status = Field<UiText>(surface, "_status");
        Check(status.Text.Contains("last gameplay sample") && status.Text.Contains("Surface slope"), "Surface window lost its live source or slope");
        var normal = new MotionVector(MathF.Sqrt(.5f), 0, MathF.Sqrt(.5f));
        var sample = new MotionSample(1, DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(), 1, new(0, 0, 1), new(0, 0, 0), new(-.4f, -.4f, 0), new(.4f, .4f, 1.8f), true, 0, .3f, true, new(0, 0, 0), normal, new(0, 0, 1), 0, 0, null, null);
        var set = surface.GetType().GetMethod("SetSources")!;
        set.Invoke(surface, [sample, "live test slope", sample with { Normal = new(0, 0, 1) }, "Recorded floor · t = 1.000s"]);
        Check(status.Text.Contains("45"), "Surface angle did not update from normal");
        Field<UiToggle>(surface, "_hold").Checked = true;
        set.Invoke(surface, [sample with { Contact = false }, "airborne", sample with { Normal = new(0, 0, 1) }, "Recorded floor · t = 1.000s"]);
        Check(status.Text.StartsWith("HELD") && status.Text.Contains("45"), "Held plane was overwritten by live air state");
        Field<UiToggle>(surface, "_hold").Checked = false;
        Check(status.Text.Contains("No support plane"), "Airborne view retained old tangent angles");
        Field<UiSelect>(surface, "_source").SelectedIndex = 1;
        var xz = ((Array)surface.GetType().GetField("_views", Private)!.GetValue(surface)!).GetValue(1)!;
        var cut = (SupportPlaneCut?)xz.GetType().GetField("_cut", Private)!.GetValue(xz);
        Check(status.Text.Contains("Recorded floor") && cut?.TangentAngle == 0, "Recorded-time plane did not select the recorded sample");
        Field<UiToggle>(surface, "_hud").Checked = true; Check(settings.ShowSupportHud, "Surface HUD toggle was not saved");
        Field<UiSelect>(surface, "_source").SelectedIndex = 0;
        set.Invoke(surface, [sample, "Live support plane · synthetic 45 degree incline", sample, "Recorded support"]);
        surface.ClientSize = new(960, 440); Application.DoEvents();
                surface.Close();
    }
    private static IEnumerable<UiNode> Descendants(UiNode parent)
    {
        yield return parent;
        foreach (UiNode child in parent.Controls) foreach (var nested in Descendants(child)) yield return nested;
    }
}
