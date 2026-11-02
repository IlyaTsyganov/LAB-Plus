using System.Diagnostics;
using LabPlus.Configuration;

namespace LabPlus.Game;

public sealed class GameLauncher
{
    private readonly VulkanLayerRegistration _layerRegistration;
    private readonly Func<string, GameFirewallResult> _firewall;
    private readonly Func<ProcessStartInfo, Process?> _start;
    public GameLauncher(SettingsStore settingsStore) : this(settingsStore, GameFirewall.Inspect) { }
    internal GameLauncher(SettingsStore settingsStore, Func<string, GameFirewallResult> firewall, Func<ProcessStartInfo, Process?>? start = null)
    { _layerRegistration = new(settingsStore); _firewall = firewall; _start = start ?? Process.Start; }

    public string RuntimeDirectory => _layerRegistration.RuntimeDirectory;

    public Process Start(LabPlusSettings settings, Action<ProcessStartInfo>? configure = null)
    {
        var startInfo = CreateStartInfo(settings.GameExecutablePath);
        string target = startInfo.FileName;
        GameFirewall.RequireBlocked(_firewall(startInfo.FileName));
        configure?.Invoke(startInfo);
        if (startInfo.UseShellExecute || !Path.IsPathFullyQualified(startInfo.FileName) ||
            !string.Equals(Path.GetFullPath(startInfo.FileName), target, StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Launch blocked: the configured process must use the verified DOOM Eternal executable directly.");
        if (settings.NeedsNativeLayer || startInfo.Environment[LightingFixPolicy.EnvironmentVariable] == "1" ||
            File.Exists(Path.Combine(AppContext.BaseDirectory, "Assets", "audio", "labplus_fun.wav")))
            _layerRegistration.ConfigureLaunch(startInfo);
        startInfo.Environment["LABPLUS_LAUNCHED"] = "1";
        // Launch ownership belongs to this exact process, even when only the
        // managed HUD is enabled. Do not let Steam replace it on startup.
        startInfo.Environment["SteamAppId"] = "782330";

        // Preparation can take time. Re-read the rule immediately before launch,
        // including when the viewer and lighting correction are both disabled.
        GameFirewall.RequireBlocked(_firewall(startInfo.FileName));
        return _start(startInfo) ?? throw new InvalidOperationException("Windows could not start DOOM Eternal.");
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
