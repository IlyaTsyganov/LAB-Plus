using LabPlus;
using System.Drawing.Imaging;
using System.Reflection;
using LabPlus.Game;

internal static class LevelStateUiChecks
{
    private const BindingFlags Private = BindingFlags.NonPublic | BindingFlags.Instance;
    private static T Field<T>(LabPlusForm form, string name) => (T)form.GetType().GetField(name, Private)!.GetValue(form)!;
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    private static void Pump(Func<bool> ready)
    {
        long deadline = Environment.TickCount64 + 5000;
        while (!ready() && Environment.TickCount64 < deadline) { Application.DoEvents(); Thread.Sleep(5); }
        Check(ready(), "State UI operation did not complete");
    }
    private static IEnumerable<UiNode> Descendants(UiNode parent)
    {
        foreach (UiNode child in parent.Controls) { yield return child; foreach (var nested in Descendants(child)) yield return nested; }
    }
    internal static void Run(string parentDirectory, string previews)
    {
        string directory = Path.Combine(parentDirectory, "states"); Directory.CreateDirectory(directory);
        var type = Assembly.Load("labplus").GetType("LabPlus.LevelStateForm", true)!;
        int process = 1234;
        using var form = (LabPlusForm)Activator.CreateInstance(type, directory, (Func<int>)(() => process))!;
        form.Show(); Application.DoEvents(); Field<System.Windows.Forms.Timer>(form, "_timer").Stop();
        void Poll()
        {
            var task = (Task)type.GetMethod("Poll", Private)!.Invoke(form, null)!; Pump(() => task.IsCompleted); task.GetAwaiter().GetResult();
        }
        string[] Capture(string role, bool after)
        {
            Field<UiAction>(form, role == "before" ? "_captureBefore" : "_captureAfter").PerformClick();
            string[] request = File.ReadAllText(Path.Combine(directory, "level-state-1234.request")).Trim().Split('\t');
            Check(request[3] == role && LevelStateSnapshot.ValidToken(request[2]), "Capture button did not create scoped request");
            long epoch = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
            string[] rows = [
                $"#LABPLUS_LEVEL_STATE_V1\t1234\t{request[2]}\t{epoch}\tready\tgame/dlc/e4m1_rig/e4m1_rig\tDOOM Eternal 6.66 Rev 3\t500:600\tknown\tcp_01a\t0\t1\tlive",
                "I\t101\t3", "I\t102\t3", "I\t103\t3",
                $"V\t101\t3\ttrigger/interact/arena_exit\tidTrigger\ttrigger\t{(after ? "disabled" : "enabled")}\texact\t{(after ? 3 : 16384)}\t0\t0\t0\t1\t1\t1\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\tabcd",
                "V\t102\t3\tplayerclip/arena_wall\tidDynamicEntity\twall\tenabled\texact\t8\t2\t0\t0\t3\t1\t1\t1\t0\t0\t2\t0\t1\t0\t0\t0\t0\t1\t0\tabcd",
                $"M\t103\t3\tstarting_platform_encounter_manager_1\tLanding Pad 1\t{(after ? 0 : 1)}\t1\t0\t0\t0\t2\t{(after ? 0 : 3)}"
            ];
            if (after) rows = rows.Where(l => !l.StartsWith("V\t102") && !l.StartsWith("I\t102")).ToArray();
            File.WriteAllLines(LevelStateSnapshot.FilePath(directory, 1234, role), rows); Poll();
            Check(Field<LevelStateSnapshot?>(form, after ? "_after" : "_before")?.Token == request[2], "Capture response not accepted");
            return rows;
        }
        Check(!Field<UiAction>(form, "_captureAfter").Enabled, "After enabled without Before");
        Capture("before", false); Check(Field<UiAction>(form, "_captureAfter").Enabled, "Before did not enable After");
        var afterRows = Capture("after", true); var grid = Field<UiTable>(form, "_grid");
        Check(grid.RowCount == 4 && Field<UiText>(form, "_summary").Text.Contains("removed 1") && Field<UiText>(form, "_summary").Text.Contains("disabled 1"), "State summary/grid incorrect");
        string highlight = Path.Combine(directory, "state-highlight-1234.request");
        Check(File.ReadAllText(highlight).Contains("V\t101\t3"), "Selecting volume did not request outline");
        Field<UiToggle>(form, "_all").Checked = true;
        Check(File.ReadAllLines(highlight).Count(l => l.StartsWith("V\t")) == 2, "Highlight-all duplicated state rows or lost deleted identity");
        Field<UiInput>(form, "_search").Text = "Manager";
        Check(grid.RowCount == 1 && !Field<UiInput>(form, "_detail").Text.Contains("Before: exact"), "Manager search/detail mixed volume geometry");
        Field<UiToggle>(form, "_all").Checked = false;
        Check(!File.ReadAllText(highlight).Contains("V\t"), "Logical manager change requested a fabricated volume");
        File.WriteAllText(Path.Combine(directory, "state-highlight-1234.status"), $"#LABPLUS_STATE_HIGHLIGHT_STATUS_V1\t1234\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\tsnapshot_not_cached\t0\t0\t0\n");
        Poll(); Check(Field<UiText>(form, "_status").Text.Contains("no longer cached"), "Missing native geometry cache was not explained");
        Field<UiInput>(form, "_search").Clear();
        Field<UiToggle>(form, "_all").Checked = true;
        form.Size = form.MinimumSize; Application.DoEvents();
                        var after = Field<LevelStateSnapshot>(form, "_after");
        type.GetField("_after", Private)!.SetValue(form, after with { Identity = "700:800" }); type.GetMethod("RefreshComparison", Private)!.Invoke(form, null);
        Check(grid.RowCount == 0 && Field<UiText>(form, "_summary").Text.Contains("Different game process") && !File.Exists(highlight), "Map mismatch retained comparison/highlights");
        type.GetField("_after", Private)!.SetValue(form, after with { Profile = "different" }); type.GetMethod("RefreshComparison", Private)!.Invoke(form, null);
        Check(grid.RowCount == 0 && !File.Exists(highlight), "Different profile with reused map address published highlights");
        Capture("before", false); Check(Field<LevelStateSnapshot?>(form, "_after") is null && !File.Exists(highlight), "New Before retained After/highlight");
        // A previous token must not satisfy a new pending request.
        Field<UiAction>(form, "_captureAfter").PerformClick(); Poll();
        Check(Field<string?>(form, "_pendingToken") is not null, "Stale capture satisfied a new token");
        process = 5678; Poll();
        Check(Field<string?>(form, "_pendingToken") is null && Field<UiText>(form, "_status").Text.Contains("canceled"), "Process change did not cancel capture");
        form.Close(); Check(!File.Exists(highlight), "Closing state window retained outline request");
        Console.WriteLine("PASS: state UI before/action/after flow, summary, selection/all highlights, search, cache errors, map/PID/token scope, reset.");
    }
}
