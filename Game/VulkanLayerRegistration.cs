using System.Security.Cryptography;
using System.Text.Json;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32;
using LabPlus.Configuration;

namespace LabPlus.Game;

/// <summary>
/// Configures Vulkan layers in the launched child's environment only.
/// Existing OBS registrations are read without changing their configuration.
/// </summary>
public sealed class VulkanLayerRegistration
{
    private const string LayerName = "VK_LAYER_LABPLUS_collision";
    private const string CaptureLayerDirectory = "capture";
    private const string EnableCaptureLayer = "LABPLUS_ENABLE_CAPTURE_LAYER";
    private const string DisableCaptureLayer = "LABPLUS_DISABLE_CAPTURE_LAYER";
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
        start.Environment.TryGetValue("DISABLE_VULKAN_OBS_CAPTURE", out string? obsDisabled);
        start.Environment.TryGetValue(DisableCaptureLayer, out string? captureLayerDisabled);
        bool beforeObs = obsDisabled != "1" && captureLayerDisabled != "1" &&
            HasObsVulkanCapture() && SupportsImplicitLayerPath(start.FileName);
        PrepareRuntime(beforeObs);
        var path = new StringBuilder(32768);
        uint count = GetShortPathName(RuntimeDirectory, path, (uint)path.Capacity);
        ConfigureEnvironment(start, count > 0 && count < path.Capacity ? path.ToString() : RuntimeDirectory, beforeObs);
    }

    internal static void ConfigureEnvironment(ProcessStartInfo start, string directory, bool beforeObs = false)
    {
        start.Environment.TryGetValue("VK_LAYER_PATH", out string? paths);
        start.Environment["VK_LAYER_PATH"] = string.Join(';', (paths ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries).Append(directory).Distinct(StringComparer.OrdinalIgnoreCase));
        start.Environment.TryGetValue("VK_INSTANCE_LAYERS", out string? layers);
        var requested = (layers ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries);
        var active = beforeObs ? requested.Where(layer => layer != LayerName) : requested.Append(LayerName);
        start.Environment["VK_INSTANCE_LAYERS"] = string.Join(';', active.Distinct(StringComparer.Ordinal));
        if (beforeObs)
        {
            // Explicit activation would move LAB Plus behind OBS again.
            const string overridePath = "VK_IMPLICIT_LAYER_PATH";
            start.Environment.TryGetValue(overridePath, out string? overrideDirectories);
            string searchPath = string.IsNullOrEmpty(overrideDirectories) ? "VK_ADD_IMPLICIT_LAYER_PATH" : overridePath;
            string captureDirectory = Path.Combine(directory, CaptureLayerDirectory);
            start.Environment.TryGetValue(searchPath, out string? existingDirectories);
            var inherited = (existingDirectories ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries);
            start.Environment[searchPath] = string.Join(';', inherited.Prepend(captureDirectory).Distinct(StringComparer.OrdinalIgnoreCase));
            start.Environment[EnableCaptureLayer] = "1";
        }
        else start.Environment.Remove(EnableCaptureLayer);
        // Prevent Steam from replacing the directly launched child and losing
        // the child-only layer configuration.
        start.Environment["SteamAppId"] = "782330";
    }

    private static bool HasObsVulkanCapture()
    {
        static bool Registered(RegistryKey root)
        {
            try
            {
                using var layers = root.OpenSubKey(ImplicitLayersKey);
                return layers is not null && layers.GetValueNames().Any(path =>
                    Path.GetFileName(path).Equals("obs-vulkan64.json", StringComparison.OrdinalIgnoreCase) &&
                    layers.GetValue(path) is 0 && File.Exists(path));
            }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException or System.Security.SecurityException)
            {
                return false;
            }
        }
        return Registered(Registry.CurrentUser) || Registered(Registry.LocalMachine);
    }

    private static bool SupportsImplicitLayerPath(string executable)
    {
        string? directory = Path.GetDirectoryName(executable);
        string loader = Path.Combine(directory ?? "", "vulkan-1.dll");
        if (!File.Exists(loader)) loader = Path.Combine(Environment.SystemDirectory, "vulkan-1.dll");
        try
        {
            var version = FileVersionInfo.GetVersionInfo(loader);
            // Vulkan Loader introduced the per-process implicit search paths in 1.3.295.
            return new Version(version.FileMajorPart, version.FileMinorPart, version.FileBuildPart) >= new Version(1, 3, 295);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or System.ComponentModel.Win32Exception)
        {
            return false;
        }
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

    internal void PrepareRuntime(bool beforeObs = false)
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
        WriteManifest(LayerManifestPath, $@".\{runtimeDllName}", false);
        if (beforeObs)
        {
            string captureDirectory = Path.Combine(RuntimeDirectory, CaptureLayerDirectory);
            Directory.CreateDirectory(captureDirectory);
            WriteManifest(Path.Combine(captureDirectory, "labplus-layer.json"), $@"..\{runtimeDllName}", true);
        }
    }

    private static void WriteManifest(string path, string library, bool beforeObs)
    {
        File.WriteAllText(path, JsonSerializer.Serialize(new
        {
            file_format_version = "1.2.0",
            layer = new
            {
                name = LayerName,
                type = "GLOBAL",
                library_path = library,
                enable_environment = beforeObs ? new Dictionary<string, string> { [EnableCaptureLayer] = "1" } : null,
                disable_environment = beforeObs ? new Dictionary<string, string> { [DisableCaptureLayer] = "1" } : null,
                api_version = "1.3.0",
                implementation_version = "1",
                description = "LAB Plus collision viewer for DOOM Eternal",
                functions = new Dictionary<string, string> { ["vkNegotiateLoaderLayerInterfaceVersion"] = "labplus_negotiate_layer_interface_version" }
            }
        }, new JsonSerializerOptions
        {
            WriteIndented = true,
            DefaultIgnoreCondition = System.Text.Json.Serialization.JsonIgnoreCondition.WhenWritingNull
        }));
    }

}
