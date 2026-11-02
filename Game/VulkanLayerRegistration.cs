using System.Security.Cryptography;
using System.Text.Json;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32;
using LabPlus.Configuration;

namespace LabPlus.Game;

/// <summary>
/// Prepares explicit Vulkan layers in the launched child's environment only.
/// Registry access is limited to retiring registrations made by older builds.
/// </summary>
public sealed class VulkanLayerRegistration
{
    private const string LayerName = "VK_LAYER_LABPLUS_collision";
    private const string ExplicitLayersKey = @"Software\Khronos\Vulkan\ExplicitLayers";
    private const string ImplicitLayersKey = @"Software\Khronos\Vulkan\ImplicitLayers";

    private readonly SettingsStore _settingsStore;

    public VulkanLayerRegistration(SettingsStore settingsStore) => _settingsStore = settingsStore;

    public string RuntimeDirectory => Path.Combine(_settingsStore.SettingsDirectory, "viewer-runtime");
    public string ExpectedRuntimeLibraryName
    {
        get
        {
            string dllPath = Path.Combine(AppContext.BaseDirectory, "Native", "CollisionViewer", "bin", "labplus-layer.dll");
            byte[] hash = SHA256.HashData(File.ReadAllBytes(dllPath));
            return $"labplus-layer-{Convert.ToHexString(hash)[..16].ToLowerInvariant()}.dll";
        }
    }
    private string LayerManifestPath => Path.Combine(RuntimeDirectory, "labplus-layer.json");
    private string OverrideManifestPath => Path.Combine(RuntimeDirectory, "labplus-override.json");

    public void ConfigureLaunch(ProcessStartInfo start)
    {
        PrepareRuntime();
        start.Environment["LABPLUS_FUN_AUDIO"] = Path.Combine(AppContext.BaseDirectory, "Assets", "audio", "labplus_fun.wav");
        var path = new StringBuilder(32768);
        uint count = GetShortPathName(RuntimeDirectory, path, (uint)path.Capacity);
        ConfigureEnvironment(start, count > 0 && count < path.Capacity ? path.ToString() : RuntimeDirectory);
    }

    internal static void ConfigureEnvironment(ProcessStartInfo start, string directory)
    {
        start.Environment.TryGetValue("VK_LAYER_PATH", out string? paths);
        start.Environment["VK_LAYER_PATH"] = string.Join(';', (paths ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries).Append(directory).Distinct(StringComparer.OrdinalIgnoreCase));
        start.Environment.TryGetValue("VK_INSTANCE_LAYERS", out string? layers);
        start.Environment["VK_INSTANCE_LAYERS"] = string.Join(';', (layers ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries).Append(LayerName).Distinct(StringComparer.Ordinal));
        // Prevent Steam from replacing the directly launched child and losing
        // the child-only layer configuration.
        start.Environment["SteamAppId"] = "782330";
    }

    [DllImport("kernel32.dll", EntryPoint = "GetShortPathNameW", CharSet = CharSet.Unicode)]
    private static extern uint GetShortPathName(string path, StringBuilder output, uint capacity);

    public void Disable()
    {
        using RegistryKey? explicitLayers = Registry.CurrentUser.OpenSubKey(ExplicitLayersKey, writable: true);
        using RegistryKey? implicitLayers = Registry.CurrentUser.OpenSubKey(ImplicitLayersKey, writable: true);
        explicitLayers?.DeleteValue(LayerManifestPath, throwOnMissingValue: false);
        implicitLayers?.DeleteValue(OverrideManifestPath, throwOnMissingValue: false);
        RetirePreviousRegistration(explicitLayers, implicitLayers);
    }

    private static void RetirePreviousRegistration(RegistryKey? explicitLayers, RegistryKey? implicitLayers)
    {
        if (LegacyDataMigration.OwnsPreviousManifest(LegacyDataMigration.PreviousLayerManifest, false))
            explicitLayers?.DeleteValue(LegacyDataMigration.PreviousLayerManifest, throwOnMissingValue: false);
        if (LegacyDataMigration.OwnsPreviousManifest(LegacyDataMigration.PreviousOverrideManifest, true))
            implicitLayers?.DeleteValue(LegacyDataMigration.PreviousOverrideManifest, throwOnMissingValue: false);
    }

    internal void PrepareRuntime()
    {
        string dllPath = Path.Combine(AppContext.BaseDirectory, "Native", "CollisionViewer", "bin", "labplus-layer.dll");
        if (!File.Exists(dllPath))
            throw new FileNotFoundException("Collision Viewer has not been built yet. Build Native\\CollisionViewer\\build-viewer.cmd.", dllPath);

        Directory.CreateDirectory(RuntimeDirectory);
        string runtimeDllName = ExpectedRuntimeLibraryName;
        string destinationDll = Path.Combine(RuntimeDirectory, runtimeDllName);
        // Keep each built layer version side by side. Windows cannot replace a
        // DLL while a game process has it loaded; the manifest can safely move
        // to the next version without touching the running game's module.
        if (!File.Exists(destinationDll))
            File.Copy(dllPath, destinationDll, overwrite: false);
        File.WriteAllText(LayerManifestPath, JsonSerializer.Serialize(new
        {
            file_format_version = "1.2.0",
            layer = new
            {
                name = LayerName,
                type = "GLOBAL",
                library_path = $@".\{runtimeDllName}",
                api_version = "1.3.0",
                implementation_version = "1",
                description = "LAB Plus collision viewer for DOOM Eternal",
                functions = new Dictionary<string, string> { ["vkNegotiateLoaderLayerInterfaceVersion"] = "labplus_negotiate_layer_interface_version" }
            }
        }, new JsonSerializerOptions { WriteIndented = true }));
    }

}
