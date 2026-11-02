using System.Text;
using LabPlus.Game;

internal static class SaveLibraryChecks
{
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    private static void Reject(Action action, string message) { try { action(); } catch (IOException) { return; } throw new Exception(message); }
    internal static void Run(string parent)
    {
        string root = Path.Combine(parent, "save-library"), remote = Path.Combine(root, "Steam", "userdata", "10001", "782330", "remote"); Directory.CreateDirectory(remote);
        var location = SteamSaveLocation.Parse(remote); var vault = new PracticeSaveVault(root, () => []);
        byte[] Bytes(string name, string content) => SteamSaveContainer.Seal(Encoding.UTF8.GetBytes(content), location.Account, name);
        void Write(string slot, string content) { Directory.CreateDirectory(location.SlotPath(slot)); foreach (string name in PracticeSaveVault.RequiredFiles) File.WriteAllBytes(Path.Combine(location.SlotPath(slot), name), Bytes(name, content + name)); }
        Write("GAME-AUTOSAVE2", "source slot 3"); Write("GAME-AUTOSAVE0", "target slot 1"); Write("DLC1-AUTOSAVE3", "TAG1 target");
        var source = vault.Capture(remote, "GAME-AUTOSAVE2", "Source three", "Practice", build: PracticeSaveSource.ArchiveBuild("1.1"));
        var sibling = vault.Capture(remote, "GAME-AUTOSAVE2", "Keep sibling", "Other", build: PracticeSaveSource.ArchiveBuild("1.1"));
        var third = vault.Capture(remote, "GAME-AUTOSAVE0", "Third", "Practice", build: PracticeSaveSource.ArchiveBuild("1.1"));
        string archivePath = Path.Combine(root, "practice-saves", source.Id, "slot");
        var originals = PracticeSaveVault.RequiredFiles.ToDictionary(n => n, n => File.ReadAllBytes(Path.Combine(location.SlotPath("GAME-AUTOSAVE0"), n)));
        var plan = vault.PlanLiveRestore(1234, source.Id, remote, "GAME-AUTOSAVE0", true); vault.StageLiveRestore(root, plan);
        Check(plan.Slot == "GAME-AUTOSAVE0" && vault.Verify(source.Id).SourceSlot == "GAME-AUTOSAVE2", "Cross-slot restore rewrote origin metadata or targeted wrong slot");
        foreach (string name in PracticeSaveVault.RequiredFiles)
        {
            Check(File.ReadAllBytes(Path.Combine(plan.Bundle(root), "incoming", name)).SequenceEqual(File.ReadAllBytes(Path.Combine(archivePath, name))), "Cross-slot staging changed authenticated source bytes");
            Check(File.ReadAllBytes(Path.Combine(location.SlotPath("GAME-AUTOSAVE0"), name)).SequenceEqual(originals[name]), "Staging changed the active slot");
        }
        Reject(() => vault.PlanLiveRestore(1234, source.Id, remote, "DLC1-AUTOSAVE3", true), "Cross-slot change allowed cross-campaign loading");
        Check(PracticeSaveSource.LoadBlockReason(source, "1.1", "GAME-AUTOSAVE0") is null, "Same-campaign slot 3 to 1 was blocked");
        Check(PracticeSaveSource.LoadBlockReason(source, "2.0", "GAME-AUTOSAVE0") is not null, "Cross-patch loading became allowed");
        var tag1 = source with { SourceSlot = "DLC1-AUTOSAVE5", Build = PracticeSaveSource.ArchiveBuild("3.0") };
        var tag2 = source with { SourceSlot = "DLC2-AUTOSAVE8", Build = PracticeSaveSource.ArchiveBuild("5.0") };
        Check(PracticeSaveSource.LoadBlockReason(tag1, "1.1", "GAME-AUTOSAVE0")!.Contains("3.0"), "TAG1 boundary lost");
        Check(PracticeSaveSource.LoadBlockReason(tag2, "3.0", "DLC1-AUTOSAVE3")!.Contains("5.0"), "TAG2 boundary lost");
        Check(PracticeSaveSource.LoadBlockReason(tag1, "3.0", "DLC1-AUTOSAVE3") is null && PracticeSaveSource.LoadBlockReason(tag2, "5.0", "DLC2-AUTOSAVE6") is null, "DLC transfer to another slot was blocked");
        var library = new PracticeSaveLibrary(root); library.Add("My route");
        var ordered = library.Ordered(vault.List()).ToArray(); var visible = ordered.Select(s => s.Id).ToArray(); var selected = visible.Skip(1).ToHashSet();
        library.Move(ordered, visible, selected, -1);
        var expected = new[] { visible[1], visible[2], visible[0] };
        Check(library.Ordered(ordered).Select(s => s.Id).SequenceEqual(expected), "Multi-selection order was not preserved when moving up");
        Check(!PracticeSaveLibrary.CanMove(expected, selected, -1), "Move up allowed selection past the boundary");
        library = new PracticeSaveLibrary(root);
        Check(library.Category == "My route" && library.Categories([]).Contains("My route") && library.Ordered(ordered).Select(s => s.Id).SequenceEqual(expected), "Categories/order did not survive reopening");
        library.Move(ordered, expected, selected, 1);
        Check(library.Ordered(ordered).Select(s => s.Id).SequenceEqual(visible), "Moving down did not preserve the block");
        var manifestBefore = File.ReadAllBytes(Path.Combine(root, "practice-saves", source.Id, "manifest.json"));
        library.Transfer(ordered, new HashSet<string> { source.Id, third.Id }, "My route");
        library = new PracticeSaveLibrary(root);
        var categorized = library.Categorized(vault.List());
        Check(categorized.Where(s => s.Id != sibling.Id).All(s => s.Category == "My route") && categorized.Single(s => s.Id == sibling.Id).Category == "Other", "Batch category transfer did not persist or changed unselected archives");
        Check(File.ReadAllBytes(Path.Combine(root, "practice-saves", source.Id, "manifest.json")).SequenceEqual(manifestBefore) && vault.Verify(source.Id).Category == "Practice", "Category transfer changed the authenticated archive manifest");
        Reject(() => library.Transfer(ordered, new HashSet<string> { source.Id, new string('f', 32) }, "Other"), "Transfer accepted a missing archive");
        Reject(() => library.Transfer(ordered, new HashSet<string> { source.Id }, PracticeSaveLibrary.All), "Transfer accepted the aggregate view as a category");
        Check(PracticeSaveSource.Campaign("GAME-AUTOSAVE0") == "Main", "Main campaign label did not change");
        // Prevalidation and rename rollback must keep a bulk failure from losing
        // an unrelated archive or changing game files.
        Reject(() => vault.DeleteArchives([source.Id, "../escape"]), "Deletion accepted a non-archive path");
        Check(vault.List().Count == 3, "Deletion preflight removed a valid earlier item");
        int moves = 0;
        var failing = new PracticeSaveVault(root, () => [], step => { if (step == "delete_moved" && ++moves == 2) throw new IOException("Injected rename failure"); });
        Reject(() => failing.DeleteArchives([source.Id, third.Id]), "Injected bulk failure did not fail");
        Check(vault.Verify(source.Id).Id == source.Id && vault.Verify(third.Id).Id == third.Id, "Bulk rename failure did not retain both archives");
        vault.DeleteArchives([source.Id, third.Id]);
        Check(vault.List().Single().Id == sibling.Id && !Directory.Exists(Path.Combine(root, "practice-saves", source.Id)), "Bulk delete affected an unselected archive or retained selected archive");
        foreach (string name in PracticeSaveVault.RequiredFiles) Check(File.ReadAllBytes(Path.Combine(location.SlotPath("GAME-AUTOSAVE0"), name)).SequenceEqual(originals[name]), "Deleting LAB Plus archives modified the game slot");
        var status = LivePracticeBridge.RequestText(Environment.ProcessId, new string('c', 32), DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(), "status", location, "*", false);
        Check(status.Contains("\tstatus\t") && status.Contains("\t*\t"), "Read-only current-slot request requires a preselected slot or Cloud acknowledgment");
        Reject(() => LivePracticeBridge.RequestText(1234, new string('c', 32), 1, "archive", location, "*", true), "Wildcard archival target accepted");
        // Two real production Submit calls share one mailbox. A queued status
        // probe must not overwrite a pending request or accept its stale reply.
        int pid = Environment.ProcessId; string stem = Path.Combine(root, $"practice-live-{pid}");
        var firstStatus = LivePracticeBridge.Execute(root, () => pid, "status", remote, "*", false);
        string firstToken = File.ReadAllText(stem + ".request").Split('\t')[2];
        var secondStatus = LivePracticeBridge.Execute(root, () => pid, "status", remote, "*", false);
        Check(File.ReadAllText(stem + ".request").Split('\t')[2] == firstToken, "Queued status overwrote an active mailbox request");
        void Reply(string token, string slot) => File.WriteAllText(stem + ".tsv", $"#LABPLUS_LIVE_PRACTICE_V2\t{pid}\t{token}\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\tready\t{location.Account}\t{slot}\tgame/sp/e1m1_intro/e1m1_intro\tcp_test\tRead only\t1.1\n");
        Reply(firstToken, "GAME-AUTOSAVE0");
        var firstResult = firstStatus.GetAwaiter().GetResult(); Check(firstResult.Phase == "ready" && firstResult.Slot == "GAME-AUTOSAVE0", "Read-only status did not resolve the actual current slot");
        string RequestToken() { using var file = new FileStream(stem + ".request", FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete); using var reader = new StreamReader(file); return reader.ReadToEnd().Split('\t')[2]; }
        Check(SpinWait.SpinUntil(() => secondStatus.IsFaulted || RequestToken() != firstToken, 2000), "Queued request did not resume");
        if (secondStatus.IsFaulted) secondStatus.GetAwaiter().GetResult();
        string secondToken = RequestToken(); Reply(secondToken, "GAME-AUTOSAVE2");
        Check(secondStatus.GetAwaiter().GetResult().Slot == "GAME-AUTOSAVE2", "Queued status reused an earlier response");
        Reject(() => firstResult.CaptureFolder(root), "Status was treated as an archive capture");
        var preparation = LivePracticeBridge.Execute(root, () => pid, "prepare", remote, "GAME-AUTOSAVE0", true);
        string prepareToken = RequestToken();
        Check(File.ReadAllText(stem + ".request").Contains("\tprepare\t"), "Load preparation did not request idle engine queues");
        File.WriteAllText(stem + ".tsv", $"#LABPLUS_LIVE_PRACTICE_V2\t{pid}\t{prepareToken}\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\twaiting\t{location.Account}\tGAME-AUTOSAVE0\t\t\tWaiting for autosave\t1.1\n");
        Thread.Sleep(180); Check(!preparation.IsCompleted, "Load preparation completed while the engine was busy");
        Reply(prepareToken, "GAME-AUTOSAVE0"); Check(preparation.GetAwaiter().GetResult().Phase == "ready", "Load preparation did not resume after the idle response");
        Console.WriteLine("PASS: library categories/order persistence, multi-row movement, scoped batch deletion/rollback, game-save preservation, cross-slot live staging and DLC/patch boundaries.");
    }
}
