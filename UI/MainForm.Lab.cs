using System.Diagnostics;
using System.Text.Json;

namespace LabPlus;

internal sealed partial class MainForm
{
    private Hud.Runtime.SettingsStore _hudStore = null!;
    private Hud.Runtime.GameMonitor _hudMonitor = null!;
    private bool _labClosing;

    private void InitializeLabFeatures()
    {
        _hudStore = new(Path.Combine(_settingsStore.SettingsDirectory, "hud"));
        SyncLabGamePath();
        Hud.Runtime.GameLauncher.WriteLightingControl(_hudStore, false);
        _hudMonitor = new(_hudStore, _preview || Environment.GetEnvironmentVariable("LABPLUS_STATE_TESTS") == "1");
        _hudMonitor.Updated += (_, _) =>
        {
            if (_labClosing || IsDisposed || !IsHandleCreated) return;
            try { BeginInvoke(() => { if (!_labClosing && !IsDisposed) Workspace?.Publish(); }); }
            catch (InvalidOperationException) { /* The native host has finished closing. */ }
        };
        FormClosing += (_, _) => _labClosing = true;
        FormClosed += (_, _) =>
        {
            _labClosing = true;
            _hudMonitor.Dispose();
            Hud.Runtime.GameLauncher.WriteLightingControl(_hudStore, false);
        };
    }

    private void SetWorkspaceSize(string? preset)
    {
        var size = preset switch { "mini" => new Size(680,580), "compact" => new Size(820,800), "comfortable" => new Size(920,860), _ => throw new ArgumentException("Unknown window size.") };
        WindowState = FormWindowState.Normal;
        var area = Host is null ? Screen.PrimaryScreen!.WorkingArea : Screen.FromControl(Host).WorkingArea;
        ClientSize = new Size(Math.Min(size.Width,area.Width-40),Math.Min(size.Height,area.Height-40));
        if (Host is not null) Host.Location = new Point(Math.Clamp(Host.Left,area.Left,Math.Max(area.Left,area.Right-Host.Width)),Math.Clamp(Host.Top,area.Top,Math.Max(area.Top,area.Bottom-Host.Height)));
    }

    private void SyncLabGamePath()
    {
        if (_hudStore.Snapshot().GamePath == _settings.GameExecutablePath) return;
        _hudStore.Update(s => { s.GamePath = _settings.GameExecutablePath; return true; });
    }

    private void ConfigureLabLaunch(ProcessStartInfo start)
    {
        // Keep lighting in one layer: the controllable LAB implementation.
        start.Environment[Game.LightingFixPolicy.EnvironmentVariable] = "0";
        var settings = _hudStore.Snapshot();
        start.Environment.TryGetValue("VK_INSTANCE_LAYERS", out string? inheritedLayers);
        start.Environment["VK_INSTANCE_LAYERS"] = string.Join(';',(inheritedLayers??"").Split(';',StringSplitOptions.RemoveEmptyEntries).Where(layer=>layer!="VK_LAYER_LABPLUS_lighting"));
        start.Environment["LABPLUS_HUD_LIGHTING_FIX"] = "0";
        if (Game.LightingFixPolicy.CanUse(settings.GamePath))
        {
            // Eligible NVIDIA builds can toggle the preloaded correction live.
            settings.LightingFix = true;
            string folder = Hud.Runtime.GameLauncher.PrepareLighting(_hudStore);
            var hudStart = Hud.Runtime.GameLauncher.CreateStartInfo(settings, _hudStore.DirectoryPath, folder);
            foreach (var key in new[] { "VK_INSTANCE_LAYERS", "VK_LAYER_PATH", "LABPLUS_HUD_DATA_DIRECTORY", "LABPLUS_HUD_LIGHTING_FIX", "SteamAppId" })
                if (hudStart.Environment.TryGetValue(key, out string? value)) start.Environment[key] = value;
        }
        if (_settings.CollisionViewerEnabled)
        {
            // VK_LAYER_PATH replaces the loader's normal explicit-layer search.
            // Include both owned manifests instead of hiding the registered viewer.
            _layerRegistration.PrepareRuntime();
            start.Environment["SteamAppId"] = "782330";
            string viewer = Hud.Runtime.GameLauncher.LoaderPath(_layerRegistration.RuntimeDirectory);
            start.Environment.TryGetValue("VK_LAYER_PATH", out string? directories);
            start.Environment["VK_LAYER_PATH"] = string.Join(';', (directories ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries).Append(viewer).Distinct(StringComparer.OrdinalIgnoreCase));
            start.Environment.TryGetValue("VK_INSTANCE_LAYERS", out string? layers);
            start.Environment["VK_INSTANCE_LAYERS"] = string.Join(';', (layers ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries).Append("VK_LAYER_LABPLUS_collision").Distinct(StringComparer.Ordinal));
        }
    }

    private void ApplyAppearance(JsonElement message)
    {
        if (!_settings.CollisionViewerEnabled) return;
        Configuration.VolumeAppearance? appearance = message.GetProperty("name").GetString() switch
        {
            "world" => _settings.WorldAppearance,
            "triggers" => _settings.TriggerAppearance,
            "walls" => _settings.WallAppearance,
            "inspector" => _settings.InspectorAppearance,
            _ => null
        };
        if (appearance is null) throw new ArgumentException("Unknown geometry type.");
        if (message.TryGetProperty("color", out var color))
        {
            if (!Configuration.VolumeAppearance.ValidColor(color.GetString())) throw new ArgumentException("Use a six-digit RGB color.");
            appearance.Color = color.GetString()!.ToLowerInvariant();
        }
        if (message.TryGetProperty("opacity", out var opacity))
        {
            if (!opacity.TryGetInt32(out int value) || value is < 0 or > 100) throw new ArgumentException("Fill opacity must be 0–100%.");
            appearance.FillOpacity = value;
        }
        SaveSettings();
    }
}
