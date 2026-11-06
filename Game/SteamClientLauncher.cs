using System.Diagnostics;
using Microsoft.Win32;

namespace LabPlus.Game;

internal static class SteamClientLauncher
{
    internal static Task EnsureRunningAsync(IProgress<string>? progress, CancellationToken cancellation) =>
        EnsureRunningAsync(IsRunning, IsReady, FindExecutable, info =>
        {
            using var process = Process.Start(info) ?? throw new IOException("Windows could not start Steam.");
        }, token => Task.Delay(250, token), progress, cancellation);

    internal static async Task EnsureRunningAsync(Func<bool> running, Func<bool> ready, Func<string> executable,
        Action<ProcessStartInfo> start, Func<CancellationToken, Task> wait, IProgress<string>? progress = null,
        CancellationToken cancellation = default)
    {
        cancellation.ThrowIfCancellationRequested();
        if (ready()) return;
        if (!running())
        {
            progress?.Report("Starting Steam…");
            start(CreateStartInfo(executable()));
        }
        else progress?.Report("Waiting for Steam…");
        for (int attempt = 0; attempt < 120; attempt++)
        {
            cancellation.ThrowIfCancellationRequested();
            if (ready()) return;
            await wait(cancellation).ConfigureAwait(false);
        }
        throw new IOException("Steam is not ready. Finish signing in to Steam, then click Launch game again.");
    }

    private static int[] RunningProcesses()
    {
        using var current = Process.GetCurrentProcess();
        var ids = new List<int>();
        foreach (var process in Process.GetProcessesByName("steam"))
        {
            using (process)
            {
                try { if (!process.HasExited && process.SessionId == current.SessionId) ids.Add(process.Id); }
                catch (Exception ex) when (ex is InvalidOperationException or System.ComponentModel.Win32Exception) { }
            }
        }
        return ids.ToArray();
    }
    private static bool IsRunning() => RunningProcesses().Length > 0;
    private static bool IsReady()
    {
        try
        {
            const string key = @"HKEY_CURRENT_USER\Software\Valve\Steam\ActiveProcess";
            return Registry.GetValue(key, "pid", null) is int pid && RunningProcesses().Contains(pid)
                && Registry.GetValue(key, "ActiveUser", null) is int user && user != 0;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or System.Security.SecurityException) { return false; }
    }
    private static string FindExecutable()
    {
        var paths = new List<string>();
        try
        {
            const string key = @"HKEY_CURRENT_USER\Software\Valve\Steam";
            if (Registry.GetValue(key, "SteamExe", null) is string executable) paths.Add(executable);
            if (Registry.GetValue(key, "SteamPath", null) is string directory) paths.Add(Path.Combine(directory, "steam.exe"));
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or System.Security.SecurityException) { }
        paths.Add(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Steam", "steam.exe"));
        return paths.FirstOrDefault(path => Path.IsPathFullyQualified(path) && File.Exists(path) &&
            Path.GetFileName(path).Equals("steam.exe", StringComparison.OrdinalIgnoreCase))
            ?? throw new FileNotFoundException("Steam was not found. Start Steam manually, then click Launch game again.");
    }
    internal static ProcessStartInfo CreateStartInfo(string executable)
    {
        if (!Path.IsPathFullyQualified(executable) || !File.Exists(executable) ||
            !Path.GetFileName(executable).Equals("steam.exe", StringComparison.OrdinalIgnoreCase))
            throw new FileNotFoundException("Select a valid Steam installation.", executable);
        var info = new ProcessStartInfo(Path.GetFullPath(executable))
        {
            WorkingDirectory = Path.GetDirectoryName(Path.GetFullPath(executable))!,
            UseShellExecute = false
        };
        info.ArgumentList.Add("-silent");
        // Steam is a separate client launch; game-only layers and opt-ins must
        // never be inherited by Steam or its subsequent child processes.
        foreach (string key in info.Environment.Keys.Where(key =>
            key.StartsWith("LABPLUS_", StringComparison.OrdinalIgnoreCase) ||
            key.StartsWith("VK_", StringComparison.OrdinalIgnoreCase) ||
            key.Equals("SteamAppId", StringComparison.OrdinalIgnoreCase) ||
            key.Equals("SteamGameId", StringComparison.OrdinalIgnoreCase)).ToArray())
            info.Environment.Remove(key);
        return info;
    }
}
