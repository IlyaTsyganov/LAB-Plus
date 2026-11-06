using System.Diagnostics;
using LabPlus.Configuration;

namespace LabPlus.Game;

public sealed class GameLauncher
{
    private readonly VulkanLayerRegistration _layerRegistration;
    private readonly Func<string, GameFirewallResult> _firewall;
    private readonly Func<ProcessStartInfo, Process?> _start;
    private readonly Func<IProgress<string>?, CancellationToken, Task> _ensureSteam;
    public GameLauncher(SettingsStore settingsStore) : this(settingsStore, GameFirewall.Inspect) { }
    internal GameLauncher(SettingsStore settingsStore, Func<string, GameFirewallResult> firewall, Func<ProcessStartInfo, Process?>? start = null,
        Func<IProgress<string>?, CancellationToken, Task>? ensureSteam = null)
    { _layerRegistration = new(settingsStore); _firewall = firewall; _start = start ?? Process.Start; _ensureSteam = ensureSteam ?? SteamClientLauncher.EnsureRunningAsync; }

    public string RuntimeDirectory => _layerRegistration.RuntimeDirectory;

    public Process Start(LabPlusSettings settings, Action<ProcessStartInfo>? configure = null)
        => StartAsync(settings, configure).GetAwaiter().GetResult();

    public async Task<Process> StartAsync(LabPlusSettings settings, Action<ProcessStartInfo>? configure = null,
        IProgress<string>? progress = null, CancellationToken cancellation = default)
    {
        cancellation.ThrowIfCancellationRequested();
        var startInfo = CreateStartInfo(settings.GameExecutablePath);
        string target = startInfo.FileName;
        GameFirewall.RequireBlocked(_firewall(startInfo.FileName));
        configure?.Invoke(startInfo);
        RequireVerifiedTarget(startInfo, target);
        // Checkpoint operations are available even when every overlay is off.
        _layerRegistration.ConfigureLaunch(startInfo);
        startInfo.Environment["LABPLUS_LAUNCHED"] = "1";
        // Launch ownership belongs to this exact process, even when only the
        // managed HUD is enabled. Do not let Steam replace it on startup.
        startInfo.Environment["SteamAppId"] = "782330";
        await _ensureSteam(progress, cancellation).ConfigureAwait(false);
        cancellation.ThrowIfCancellationRequested();
        RequireVerifiedTarget(startInfo, target);

        // Preparation can take time. Re-read the rule immediately before launch,
        // including when the viewer and lighting correction are both disabled.
        GameFirewall.RequireBlocked(_firewall(startInfo.FileName));
        progress?.Report("Launching game…");
        return _start(startInfo) ?? throw new InvalidOperationException("Windows could not start DOOM Eternal.");
    }
    private static void RequireVerifiedTarget(ProcessStartInfo info, string target)
    {
        if (info.UseShellExecute || !Path.IsPathFullyQualified(info.FileName) ||
            !string.Equals(Path.GetFullPath(info.FileName), target, StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Launch blocked: the configured process must use the verified DOOM Eternal executable directly.");
    }

    internal static string ValidateGamePath(string gamePath)
    {
        if (string.IsNullOrWhiteSpace(gamePath) || !File.Exists(gamePath))
            throw new FileNotFoundException("Select an existing DOOM Eternal executable in the game path section.", gamePath);

        string executableName = Path.GetFileName(gamePath);
        if (!executableName.Equals("DOOMEternalx64vk.exe", StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Select DOOMEternalx64vk.exe to launch the Collision Viewer.");
        return Path.GetFullPath(gamePath);
    }

    internal static ProcessStartInfo CreateStartInfo(string gamePath, uint? gpuVendor = null)
    {
        gamePath = ValidateGamePath(gamePath);

        var startInfo = new ProcessStartInfo
        {
            FileName = gamePath,
            WorkingDirectory = Path.GetDirectoryName(gamePath)!,
            UseShellExecute = false
        };
        startInfo.ArgumentList.Add("+com_skipIntroVideo");
        startInfo.ArgumentList.Add("1");

        // Child process only: manual/Steam launches keep their existing behavior.
        // Explicitly clear an inherited opt-in for all other game versions.
        bool lightingFix = LightingFixPolicy.CanUse(gamePath, gpuVendor);
        startInfo.Environment[LightingFixPolicy.EnvironmentVariable] = lightingFix ? "1" : "0";
        // SteamAPI_RestartAppIfNecessary can otherwise restart a retail build
        // through Steam and discard the child-only lighting-fix environment.
        // Match the upstream doomfix launcher; do not write steam_appid.txt.
        if (lightingFix)
            startInfo.Environment["SteamAppId"] = "782330";
        return startInfo;
    }
}
