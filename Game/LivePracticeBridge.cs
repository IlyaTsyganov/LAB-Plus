using System.Diagnostics;

namespace LabPlus.Game;

public sealed record LivePracticeResult(int Process, string Token, long Captured, string Phase, string Account, string Slot, string Map, string Checkpoint, string Detail, string Build = "6.66 Rev 3")
{
    public static LivePracticeResult? Parse(string text)
    {
        if (text.Length > 8192 || text.TrimEnd('\r', '\n').Contains('\n')) return null;
        string[] h = text.TrimEnd('\r', '\n').Split('\t');
        bool legacy = h.Length == 10 && h[0] == "#LABPLUS_LIVE_PRACTICE_V1";
        bool current = h.Length == 11 && h[0] == "#LABPLUS_LIVE_PRACTICE_V2";
        if ((!legacy && !current) || !int.TryParse(h[1], out int process) || process <= 0 || !LevelStateSnapshot.ValidToken(h[2])
            || !long.TryParse(h[3], out long epoch) || epoch is <= 0 or > 253402300799999 || h[4] is not ("waiting" or "ready" or "archived" or "loading" or "reloaded" or "error")
            || h[5].Length != 17 || !ulong.TryParse(h[5], out ulong account) || account <= 76561197960265728UL || account > 76561197960265728UL + uint.MaxValue
            || (legacy ? h[6] != "DLC1-AUTOSAVE5" || h[4] == "ready" : !PracticeSaveVault.ValidSlot(h[6]) && !(h[6] == "*" && h[4] == "error")) || h[7].Length > 512 || h[8].Length > 1023 || h[9].Length > 2048 || (current && h[10].Length is < 1 or > 64) || h.Any(s => s.Any(char.IsControl))) return null;
        return new(process, h[2], epoch, h[4], h[5], h[6], h[7], h[8], h[9], current ? h[10] : "6.66 Rev 3");
    }
    public string CaptureFolder(string directory)
    {
        if (Phase != "archived" || !LevelStateSnapshot.ValidToken(Token) || Process <= 0) throw new IOException("Not a completed live checkpoint capture.");
        return Path.Combine(directory, "live-practice", Process.ToString(System.Globalization.CultureInfo.InvariantCulture), Token, "slot");
    }
}
public static class LivePracticeBridge
{
    private static readonly System.Collections.Concurrent.ConcurrentDictionary<int, SemaphoreSlim> Requests = new();
    public static string RequestText(int process, string token, long now, string operation, SteamSaveLocation location, string slot, bool cloudDisabled)
    {
        if (!cloudDisabled && operation != "status") throw new IOException("Confirm Steam Cloud is disabled for DOOM Eternal first.");
        if (process <= 0 || !LevelStateSnapshot.ValidToken(token) || operation is not ("status" or "prepare" or "archive" or "reload")) throw new IOException("Invalid live checkpoint request.");
        if (operation == "status" ? slot != "*" : !PracticeSaveVault.ValidSlot(slot)) throw new IOException("Unsupported campaign slot.");
        if (location.Remote.Any(char.IsControl)) throw new IOException("Unsupported save-folder path.");
        return $"#LABPLUS_LIVE_PRACTICE_REQUEST_V2\t{process}\t{token}\t{now}\t{operation}\t{location.Account}\t{slot}\t{location.Remote}\t*\n";
    }
    public static async Task<LivePracticeResult> Execute(string directory, Func<int> gameProcess, string operation, string remote, string slot, bool cloudDisabled, Action<LivePracticeResult>? progress = null)
    {
        int process = gameProcess(); var location = SteamSaveLocation.Parse(remote); string token = Guid.NewGuid().ToString("N");
        string request = RequestText(process, token, DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(), operation, location, slot, cloudDisabled);
        return await Submit(directory, gameProcess, operation, location, slot, process, token, request, progress);
    }
    public static async Task<LivePracticeResult> ExecuteRestore(string directory, Func<int> gameProcess, LiveRestorePlan plan, bool cloudDisabled, Action<LivePracticeResult>? progress = null)
    {
        if (!cloudDisabled) throw new IOException("Confirm Steam Cloud is disabled before live restore.");
        if (gameProcess() != plan.Process) throw new IOException("Game process changed after live restore review.");
        var location = SteamSaveLocation.Parse(plan.Remote);
        if (location.Account != plan.Account) throw new IOException("Live restore owner mismatch. Account mixing is blocked.");
        _ = plan.Bundle(directory);
        string request = $"#LABPLUS_LIVE_PRACTICE_REQUEST_V2\t{plan.Process}\t{plan.Token}\t{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}\trestore\t{location.Account}\t{plan.Slot}\t{location.Remote}\t{plan.Build}\n";
        return await Submit(directory, gameProcess, "restore", location, plan.Slot, plan.Process, plan.Token, request, progress);
    }
    private static async Task<LivePracticeResult> Submit(string directory, Func<int> gameProcess, string operation, SteamSaveLocation location, string slot, int process, string token, string request, Action<LivePracticeResult>? progress)
    {
        var gate = Requests.GetOrAdd(process, _ => new SemaphoreSlim(1, 1));
        await gate.WaitAsync();
        try
        {
            if (gameProcess() != process) throw new IOException("Selected game process changed; live operation stopped.");
            // A status request may have waited behind a complete loading cycle.
            string[] fields = request.TrimEnd('\n').Split('\t'); fields[3] = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(System.Globalization.CultureInfo.InvariantCulture);
            return await SubmitCore(directory, gameProcess, operation, location, slot, process, token, string.Join('\t', fields) + "\n", progress);
        }
        finally { gate.Release(); }
    }
    private static async Task<LivePracticeResult> SubmitCore(string directory, Func<int> gameProcess, string operation, SteamSaveLocation location, string slot, int process, string token, string request, Action<LivePracticeResult>? progress)
    {
        Directory.CreateDirectory(directory); PracticeSaveVault.NoLinks(directory);
        string stem = Path.Combine(directory, $"practice-live-{process}"), path = stem + ".request";
        PracticeSaveVault.NoLinks(path); PracticeSaveVault.NoLinks(path + ".tmp");
        File.WriteAllText(path + ".tmp", request, new System.Text.UTF8Encoding(false));
        // The frame callback may briefly hold the previous request open.
        for (int attempt = 0; ; attempt++)
        {
            try { File.Move(path + ".tmp", path, true); break; }
            catch (Exception ex) when (attempt < 9 && ex is IOException or UnauthorizedAccessException)
            { await Task.Delay(20); PracticeSaveVault.NoLinks(path); }
        }
        IOException Interrupted(string reason) => new(operation == "restore"
            ? reason + " Loading was not confirmed. Files may already be installed; if installation started, the original checkpoint is retained at " + Path.Combine(location.AppRoot, ".labplus-live-original-" + token)
            : reason);
        long started = Environment.TickCount64, deadline = started + (operation == "status" ? 15000 : operation is "archive" or "prepare" ? 45000 : 360000), reported = 0; bool received = false;
        while (Environment.TickCount64 < deadline)
        {
            await Task.Delay(100);
            if (!received && Environment.TickCount64 - started >= 15000) break;
            try { using var game = Process.GetProcessById(process); if (game.HasExited) throw Interrupted("Game closed during live operation."); }
            catch (ArgumentException) { throw Interrupted("Game closed during live operation."); }
            if (gameProcess() != process) throw Interrupted("Selected game process changed; live operation stopped.");
            string response = stem + ".tsv"; PracticeSaveVault.NoLinks(response);
            if (!File.Exists(response) || new FileInfo(response).Length > 8192) continue;
            LivePracticeResult? result;
            await using (var stream = new FileStream(response, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete, 4096, FileOptions.Asynchronous))
            using (var reader = new StreamReader(stream)) result = LivePracticeResult.Parse(await reader.ReadToEndAsync());
            if (result is null || result.Process != process || result.Token != token) continue;
            long now = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(); if (result.Captured > now + 2000 || result.Captured < now - 90000) continue;
            if (result.Account != location.Account || operation != "status" && result.Slot != slot) throw new IOException("Live response owner/slot mismatch. Account mixing is blocked.");
            received = true; if (result.Phase == "error") throw new IOException(result.Detail);
            bool validPhase = operation switch
            {
                "status" => result.Phase == "ready",
                "prepare" => result.Phase is "waiting" or "ready",
                "archive" => result.Phase is "waiting" or "archived",
                _ => result.Phase is "waiting" or "loading" or "reloaded"
            };
            if (!validPhase) throw new IOException("Live response does not match the requested operation.");
            if (reported != result.Captured) { progress?.Invoke(result); reported = result.Captured; }
            if (result.Phase is "ready" or "archived" or "reloaded") return result;
        }
        throw new IOException(received ? operation == "restore" ? "Restore completion was not observed. Files may already be installed; original checkpoint retained at " + Path.Combine(location.AppRoot, ".labplus-live-original-" + token)
            : "Engine operation did not report completion. Continue in the game when loading reaches 100%."
            : "No response from the live-save bridge. The updated native layer must be loaded by restarting the game once through this LAB Plus build.");
    }
}
