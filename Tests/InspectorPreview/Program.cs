using LabPlus;
using System.Drawing.Imaging;
using System.Reflection;
using LabPlus.Configuration;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        Application.SetUnhandledExceptionMode(UnhandledExceptionMode.ThrowException);
        Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);WorkspaceWindows.Initialize();
        using var context=new ApplicationContext();int result=1;
        WorkspaceWindows.Dispatch(()=>{try{Run(args);result=0;}catch(Exception ex){Console.Error.WriteLine(ex);}finally{context.ExitThread();}});
        Application.Run(context);return result;
    }
    private static void Run(string[] args)
    {
        Environment.SetEnvironmentVariable("LABPLUS_STATE_TESTS", "1");
        Environment.SetEnvironmentVariable("LABPLUS_PREVIEW", "1");
        Environment.SetEnvironmentVariable("LABPLUS_DATA_DIRECTORY", Path.Combine(Path.GetTempPath(), "labplus-ui-" + Guid.NewGuid().ToString("N")));
        var type = Assembly.Load("labplus").GetType("LabPlus.MainForm", true)!;
        using var form = (LabPlusForm)Activator.CreateInstance(type)!;
        form.ShowInTaskbar = false;
        form.Show();
        Application.DoEvents();
        IEnumerable<UiNode> Descendants(UiNode parent)
        {
            foreach (UiNode child in parent.Controls)
            {
                yield return child;
                foreach (UiNode nested in Descendants(child)) yield return nested;
            }
        }
        WorkspaceUiChecks.Run(form, Path.GetDirectoryName(Path.GetFullPath(args[0]))!);

        string isolated = Path.Combine(Path.GetTempPath(), "labplus-TriggerUi-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(isolated);
        try
        {
            string discovery = Path.Combine(isolated, "trigger-classes.txt");
            File.WriteAllText(discovery, "idTrigger_NewClass\ninvalid\nidTrigger_NewClass\n");
            var settings = new LabPlusSettings();
            settings.SetTriggerClassVisible("idEncounterTrigger_Commit", false);
            settings.SetTriggerPathVisible("idTrigger", "trigger/props/", false);
            File.WriteAllText(Path.Combine(isolated, "trigger-paths.tsv"), "#LABPLUS_TRIGGER_PATHS_V1\nidTrigger\ttrigger/props/\nidTrigger\ttrigger/props/small/\nidTrigger\ttrigger/interact/\nidTrigger\t@no_path\nidTrigger_Teleporter\ttrigger/props/\ninvalid\tbad/\n");
            int writes = 0;
            var filterType = Assembly.Load("labplus").GetType("LabPlus.TriggerTypesForm", true)!;
            using var filter = (LabPlusForm)Activator.CreateInstance(filterType, settings, discovery, (Action)(() => ++writes))!;
            filter.Show(); Application.DoEvents();
            var list = Descendants(filter).OfType<UiTree>().Single();
            var search = Descendants(filter).OfType<UiInput>().Single();
            IEnumerable<UiTreeNode> Nodes(UiTreeNodes nodes)
            {
                foreach (UiTreeNode node in nodes) { yield return node; foreach (var child in Nodes(node.Nodes)) yield return child; }
            }
            UiTreeNode Root(string name) => list.Nodes.Cast<UiTreeNode>().Single(node => node.Text.StartsWith(name + " —"));
            UiTreeNode PathNode(string typeName, string path) => Nodes(list.Nodes).Single(node => node.ToolTipText == typeName + " · " + path);
            if (list.Nodes.Count != TriggerClassCatalog.Known.Count + 1 || writes != 0)
                throw new Exception("Discovery population lost a class or changed settings");
            if (Nodes(list.Nodes).Any(node => node.Text.Contains("No directory") || node.ToolTipText.Contains("@no_path")))
                throw new Exception("Retired no-directory bucket is still displayed");
            if (!PathNode("idTrigger_Hurt", "trigger/ai/").Checked || !PathNode("idTrigger", "trigger/lootdrop/").Checked
                || !PathNode("idTrigger_GorillaBar", "interact/vault/").Checked || !PathNode("idTrigger", "trigger/props/weapons/").Checked)
                throw new Exception("Resource categories absent from current map discovery were not loaded");
            var commitNode = Root("idEncounterTrigger_Commit");
            if (commitNode.Checked) throw new Exception("Saved hidden class was not restored in UI");
            commitNode.Checked = true;
            if (!settings.IsTriggerClassVisible("idEncounterTrigger_Commit") || writes != 1)
                throw new Exception("Checkbox did not apply its preference immediately");
            if (PathNode("idTrigger", "trigger/props/").Checked || !PathNode("idTrigger", "trigger/interact/").Checked)
                throw new Exception("Saved path subtype states were not restored independently");
            Root("idTrigger").Checked = false; Root("idTrigger").Checked = true;
            if (PathNode("idTrigger", "trigger/props/").Checked || !settings.IsTriggerPathVisible("idTrigger", "trigger/interact/"))
                throw new Exception("Parent class toggle lost its path choices");
            PathNode("idTrigger", "trigger/interact/").Checked = false;
            if (settings.IsTriggerPathVisible("idTrigger", "trigger/interact/") || !settings.IsTriggerPathVisible("idTrigger_Teleporter", "trigger/props/"))
                throw new Exception("Path checkbox changed a different class");
            PathNode("idTrigger", "trigger/interact/").Checked = true;
            int beforeBulk = writes;
            search.Text = "trigger/interact/";
            if (list.Nodes.Count != 1 || Nodes(list.Nodes).Any(node => node.ToolTipText.Contains("props/"))) throw new Exception("Path search leaked sibling folders");
            Descendants(filter).OfType<UiAction>().Single(button => button.Text == "Hide listed").PerformClick();
            if (!settings.IsTriggerClassVisible("idTrigger") || settings.IsTriggerPathVisible("idTrigger", "trigger/interact/") || writes != beforeBulk + 1)
                throw new Exception("Path search bulk hide disabled its parent class or saved multiple times");
            Descendants(filter).OfType<UiAction>().Single(button => button.Text == "Show listed").PerformClick();
            search.Text = "Teleporter";
            if (list.Nodes.Count != 2) throw new Exception("Search did not narrow exact classes");
            beforeBulk = writes;
            Descendants(filter).OfType<UiAction>().Single(button => button.Text == "Hide listed").PerformClick();
            if (settings.IsTriggerClassVisible("idTrigger_Teleporter") || settings.IsTriggerClassVisible("idTrigger_Teleporter_Fade")
                || !settings.IsTriggerClassVisible("idTrigger") || writes != beforeBulk + 1)
                throw new Exception("Bulk hide modified classes outside the search or saved more than once");
            search.Text = "no matching class";
            Descendants(filter).OfType<UiAction>().Single(button => button.Text == "Hide listed").PerformClick();
            if (writes != beforeBulk + 1) throw new Exception("Empty search changed preferences");
            search.Text = "";
            File.AppendAllText(discovery, "idEncounterTrigger_NewClass\n");
            filterType.GetMethod("ReadDiscoveredClasses", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(filter, null);
            filterType.GetMethod("Populate", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(filter, null);
            if (list.Nodes.Count != TriggerClassCatalog.Known.Count + 2 || settings.IsTriggerClassVisible("idTrigger_Teleporter"))
                throw new Exception("Live discovery reset the current filter");
            File.AppendAllText(Path.Combine(isolated, "trigger-paths.tsv"), "idTrigger\ttrigger/new_group/\n");
            filterType.GetMethod("ReadDiscoveredClasses", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(filter, null);
            filterType.GetMethod("Populate", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(filter, null);
            if (!PathNode("idTrigger", "trigger/new_group/").Checked || PathNode("idTrigger", "trigger/props/").Checked)
                throw new Exception("Live path discovery lost saved choices or hid a new subtype");
            filter.Close();
            var researchType = Assembly.Load("labplus").GetType("LabPlus.ResearchForm", true)!;
            using var research = (LabPlusForm)Activator.CreateInstance(researchType, settings, isolated, (Action)(() => ++writes), (Func<int>)(() => 1234))!;
            research.Show(); Application.DoEvents();
            Descendants(research).OfType<UiAction>().Single(button => button.Text == "Start recording").PerformClick();
            if (!settings.RecordResearch || settings.ResearchProcess != 1234) throw new Exception("Research UI did not start a manual recording");
            Descendants(research).OfType<UiAction>().Single(button => button.Text == "Stop recording").PerformClick();
            if (settings.RecordResearch) throw new Exception("Research UI did not stop recording");
            Descendants(research).OfType<UiToggle>().Single(check => check.Text == "Show path in game").Checked = false;
            if (settings.ShowResearchPath) throw new Exception("Path checkbox did not update its independent setting");
            Descendants(research).OfType<UiAction>().Single(button => button.Text == "New session").PerformClick();
            if (settings.ResearchReset == 0) throw new Exception("New-session button did not reset the timeline");

            research.Close();
            string[] encounterLines = [$"#LABPLUS_ENCOUNTERS_V1\t1234\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\tlive\tgame/dlc/e4m1_rig/e4m1_rig\t0",
                "id\tgeneration\tmanager\tlabel\tthinkFlags\tenabled\tpaused\tspawnPaused\twaitingExit\tscript\tcommands",
                "100\t3\tstarting_platform_encounter_manager_1\tUAC Atlantica / Combat 1 Landing Pad 1\t1\t1\t0\t0\t0\t0\t1",
                "101\t3\tunlisted_manager\t\t0\t1\t0\t0\t0\t-1\t0"];
            string encounterFile = Path.Combine(isolated, "active-encounters-1234.tsv"); File.WriteAllLines(encounterFile, encounterLines);
            var encountersType = Assembly.Load("labplus").GetType("LabPlus.ActiveEncountersForm", true)!;
            using var encounters = (LabPlusForm)Activator.CreateInstance(encountersType, settings, isolated, (Action)(() => ++writes), (Func<int>)(() => 1234))!;
            encounters.Show();
            var grid = Descendants(encounters).OfType<UiTable>().Single();
            var deadline = DateTime.UtcNow.AddSeconds(3);
            while (grid.RowCount == 0 && DateTime.UtcNow < deadline) { Application.DoEvents(); Thread.Sleep(5); }
            ((System.Windows.Forms.Timer)encountersType.GetField("_timer", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(encounters)!).Stop();
            if (grid.RowCount != 1 || !grid.Rows[0].Cells[0].Value!.ToString()!.Contains("Landing Pad")) throw new Exception("Active encounter window did not show the reference name");
            Descendants(encounters).OfType<UiToggle>().Single(check => check.Text == "Show inactive managers").Checked = true;
            if (grid.RowCount != 2) throw new Exception("Inactive-manager checkbox did not reveal inactive rows");
            var encounterSearch = Descendants(encounters).OfType<UiInput>().Single(); encounterSearch.Text = "unlisted";
            if (grid.RowCount != 1 || grid.Rows[0].Cells[0].Value!.ToString() != "No name in reference") throw new Exception("Technical-name search or unknown-name fallback failed");
            encounterSearch.Clear();
            Descendants(encounters).OfType<UiToggle>().Single(check => check.Text == "Show in-game HUD").Checked = true;
            if (!settings.ShowActiveEncountersHud) throw new Exception("Encounter HUD checkbox did not persist its state");

            encounters.Close();
            string[] graphLines = [$"#LABPLUS_GRAPH_V1\t1234\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\tlive\tgame/dlc/e4m1_rig/e4m1_rig\t500:600\t0\t0\t0\t100:3",
                "N\t100:3\tarena_trigger\tidEncounterTrigger_Commit\t", "N\t101:3\tstarting_platform_encounter_manager_1\tidEncounterManager\tEncounter: UAC Atlantica / Combat 1 Landing Pad 1",
                "N\t102:3\tgate_entity\tidEntity\t", "N\t103:3\tunlinked_entity\tidEntity\t",
                "E\t100:3\t101:3\towners[0]\tManaged pointer / map slot checked", "E\t101:3\t102:3\ttarget\tEntity pointer"];
            string graphFile = Path.Combine(isolated, "entity-graph-1234.tsv"); File.WriteAllLines(graphFile, graphLines);
            var graphType = Assembly.Load("labplus").GetType("LabPlus.EntityGraphForm", true)!;
            using var graphForm = (LabPlusForm)Activator.CreateInstance(graphType, isolated, (Func<int>)(() => 1234))!;
            graphForm.Show();
            var graphGrid = Descendants(graphForm).OfType<UiTable>().Single();
            deadline = DateTime.UtcNow.AddSeconds(3);
            while (graphGrid.RowCount == 0 && DateTime.UtcNow < deadline) { Application.DoEvents(); Thread.Sleep(5); }
            ((System.Windows.Forms.Timer)graphType.GetField("_timer", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(graphForm)!).Stop();
            if (graphGrid.RowCount != 1 || graphGrid.Rows[0].Cells[2].Value?.ToString() != "owners[0]") throw new Exception("Crosshair graph did not show exact owner evidence");
            var nodeList = Descendants(graphForm).OfType<UiList>().Single();
            int managerIndex = nodeList.Items.Cast<LabPlus.Game.EntityGraphNode>().ToList().FindIndex(node => node.Key == "101:3");
            nodeList.SelectedIndex = managerIndex;
            if (graphGrid.RowCount != 2 || Descendants(graphForm).OfType<UiToggle>().Single(check => check.Text == "Follow crosshair").Checked)
                throw new Exception("Selecting a manager did not navigate incoming/outgoing links or freeze following");
            var graphCanvas = Descendants(graphForm).Single(control => control.GetType().Name == "EntityGraphCanvas");
            var gatePoint = GraphNavigationChecks.NodePoint(graphCanvas, "102:3");
            GraphNavigationChecks.Mouse(graphCanvas, "OnMouseClick", MouseButtons.Right, gatePoint);
            if (graphType.GetField("_selected", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(graphForm) as string != "101:3")
                throw new Exception("RMB selected a graph node");
            GraphNavigationChecks.Mouse(graphCanvas, "OnMouseClick", MouseButtons.Left, gatePoint);
            if (graphType.GetField("_selected", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(graphForm) as string != "102:3"
                || graphGrid.RowCount != 1 || nodeList.SelectedItem is not LabPlus.Game.EntityGraphNode { Key: "102:3" })
                throw new Exception("LMB on graph did not select the entity and update its links/list");
            nodeList.SelectedIndex = managerIndex;
            GraphNavigationChecks.Run(Path.GetDirectoryName(args[0])!, args.Length > 1 ? args[1] : null);

            Descendants(graphForm).OfType<UiToggle>().Single(check => check.Text == "Linked entities only").Checked = false;
            if (nodeList.Items.Count != 4) throw new Exception("Graph filter did not reveal isolated entities");
            Descendants(graphForm).OfType<UiInput>().Single().Text = "gate_entity";
            if (nodeList.Items.Count != 1) throw new Exception("Graph search failed");
            graphForm.ClientSize = new Size(1050, 660); Application.DoEvents();
            // A new map may reuse every numeric ID. It must not retain the previous selection.
            File.WriteAllLines(graphFile, graphLines.Select(line => line.Replace("500:600", "700:800")).Where(line => !line.StartsWith("E\t")));
            graphType.GetMethod("RefreshSnapshot", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(graphForm, null);
            deadline = DateTime.UtcNow.AddSeconds(2);
            while (graphType.GetField("_busy", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(graphForm) is true && DateTime.UtcNow < deadline) { Application.DoEvents(); Thread.Sleep(5); }
            if (graphGrid.RowCount != 0 || graphType.GetField("_selected", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(graphForm) is not null)
                throw new Exception("Map change retained a previous graph selection");
            graphForm.Close();
            MovementUiChecks.Run(isolated, Path.GetDirectoryName(args[0])!);
            LevelStateUiChecks.Run(isolated, Path.GetDirectoryName(args[0])!);
            BookmarkUiChecks.Run(isolated, Path.GetDirectoryName(args[0])!);
        }
        finally { Directory.Delete(isolated, recursive: true); }
        Console.WriteLine("PASS: screen state, trigger filters, encounters and entity graph navigation; all neighbors fit, cursor zoom/RMB pan/LMB selection, view persistence and map reset. No native widgets were created.");
        form.Close();
    }
}
