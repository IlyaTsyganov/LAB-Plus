using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text;
using System.Globalization;

namespace LabPlus.Configuration;

public enum RenderStyle
{
    Outlines,
    Filled
}

public sealed class VolumeAppearance
{
    public string Color { get; set; } = "#26ccff";
    public int FillOpacity { get; set; } = 9;
    public static bool ValidColor(string? value) => value is { Length: 7 } && value[0] == '#' && value.Skip(1).All(Uri.IsHexDigit);
    internal void Normalize(string fallback, int opacity = 9) { if (!ValidColor(Color)) Color = fallback; FillOpacity = Math.Clamp(FillOpacity, 0, 100); }
}

public sealed class LabPlusSettings
{
    public string GameExecutablePath { get; set; } = string.Empty;
    public bool CollisionViewerEnabled { get; set; }
    public bool ShowTriggers { get; set; } = true;
    public bool ShowWorldCollision { get; set; } = true;
    public bool ShowShotCollision { get; set; }
    public bool TrimShotCollision { get; set; }
    public bool ShowSyncTarget { get; set; }
    public int SyncTargetAction { get; set; }
    public string SyncGloryKillAnimation { get; set; } = "";
    public string SyncChainsawAnimation { get; set; } = "";
    internal static string SyncAnimationKey(string? value) => value is { Length: <= 180 } && value.All(c => char.IsAsciiLetterOrDigit(c) || "_./-".Contains(c)) ? value : "";
    public VolumeAppearance ShotAppearance { get; set; } = new() { Color = "#cc59ff" };
    public bool ShowInvisibleWalls { get; set; } = true;
    public VolumeAppearance WorldAppearance { get; set; } = new() { Color = "#26ccff" };
    public VolumeAppearance TriggerAppearance { get; set; } = new() { Color = "#40f21f" };
    public VolumeAppearance WallAppearance { get; set; } = new() { Color = "#ff730d" };
    public VolumeAppearance InspectorAppearance { get; set; } = new() { Color = "#ffd91a", FillOpacity = 20 };
    public bool InspectVolumes { get; set; }
    public bool ShowDisabledVolumes { get; set; }
    public bool ShowVolumeHistory { get; set; }
    public bool ShowActiveEncountersHud { get; set; }
    public bool ShowEnemyHealthHud { get; set; }
    public bool ShowEnemyHealthBars { get; set; }
    public bool ShowGameTimersHud { get; set; }
    public bool ShowSpeedrunTechsHud { get; set; }
    [JsonIgnore]
    public bool NeedsNativeLayer => CollisionViewerEnabled || ShowPlayerPositionHud || ShowPlayerAnglesHud || ShowEnemyHealthHud || ShowEnemyHealthBars || ShowGameTimersHud || ShowSpeedrunTechsHud || ShowSyncTarget || ShowSupportHud || ShowActiveEncountersHud;
    [JsonNumberHandling(JsonNumberHandling.AllowNamedFloatingPointLiterals)]
    public double DebugHudScale { get; set; } = 1;
    public bool ShowResearchPath { get; set; } = true;
    public bool RecordResearch { get; set; }
    public int ResearchProcess { get; set; }
    public long ResearchReset { get; set; }
    public long ResearchMarker { get; set; }
    public string ResearchMarkerLabel { get; set; } = "";
    public int MotionProcess { get; set; }
    public bool ShowPlayerCollider { get; set; }
    public bool ShowPlayerPositionHud { get; set; }
    public bool ShowPlayerAnglesHud { get; set; }
    public bool ShowSupportHud { get; set; }
    public bool ShowMotionPaths { get; set; } = true;
    public bool MotionFollowTarget { get; set; } = true;
    public List<string> HiddenTriggerClasses { get; set; } = [];
    public List<TriggerPathInfo> HiddenTriggerPaths { get; set; } = [];
    public RenderStyle Style { get; set; } = RenderStyle.Filled;
    public int ViewDistanceMeters { get; set; } = 40;

    public bool IsTriggerClassVisible(string className) => !(HiddenTriggerClasses?.Contains(className, StringComparer.Ordinal) ?? false);

    public void SetTriggerClassVisible(string className, bool visible)
    {
        if (!TriggerClassCatalog.IsValidName(className)) return;
        HiddenTriggerClasses ??= [];
        HiddenTriggerClasses.RemoveAll(name => name == className);
        if (!visible) HiddenTriggerClasses.Add(className);
    }

    public bool IsTriggerPathExplicitlyVisible(string className, string prefix) => !(HiddenTriggerPaths ?? []).Contains(new(className, prefix));
    public bool IsTriggerPathVisible(string className, string prefix) => IsTriggerClassVisible(className)
        && TriggerPathCatalog.Ancestors(prefix).All(parent => IsTriggerPathExplicitlyVisible(className, parent));
    public void SetTriggerPathVisible(string className, string prefix, bool visible)
    {
        var item = new TriggerPathInfo(className, TriggerPathCatalog.Normalize(prefix));
        if (!TriggerPathCatalog.IsValid(item)) return;
        HiddenTriggerPaths ??= [];
        HiddenTriggerPaths.RemoveAll(existing => existing == item);
        if (!visible) HiddenTriggerPaths.Add(item);
    }

    internal void NormalizeTriggerClasses()
    {
        if (!double.IsFinite(DebugHudScale) || DebugHudScale is < 0.5 or > 3) DebugHudScale = 1;
        ShotAppearance ??= new() { Color = "#cc59ff" }; ShotAppearance.Normalize("#cc59ff");
        WorldAppearance ??= new() { Color = "#26ccff" }; WorldAppearance.Normalize("#26ccff");
        TriggerAppearance ??= new() { Color = "#40f21f" }; TriggerAppearance.Normalize("#40f21f");
        WallAppearance ??= new() { Color = "#ff730d" }; WallAppearance.Normalize("#ff730d");
        InspectorAppearance ??= new() { Color = "#ffd91a", FillOpacity = 20 }; InspectorAppearance.Normalize("#ffd91a", 20);
        HiddenTriggerClasses = (HiddenTriggerClasses ?? []).Where(TriggerClassCatalog.IsValidName).Distinct(StringComparer.Ordinal).Order(StringComparer.Ordinal).ToList();
        HiddenTriggerPaths = (HiddenTriggerPaths ?? []).Where(item => item is not null)
            .Select(item => item with { Prefix = TriggerPathCatalog.Normalize(item.Prefix ?? "") })
            .Where(TriggerPathCatalog.IsValid).Distinct().OrderBy(item => item.ClassName, StringComparer.Ordinal)
            .ThenBy(item => item.Prefix, StringComparer.Ordinal).Take(2048).ToList();
    }
}

public sealed class SettingsStore
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        Converters = { new JsonStringEnumConverter() }
    };

    public SettingsStore(string? settingsDirectory = null)
    {
        string? overrideDirectory = Environment.GetEnvironmentVariable("LABPLUS_DATA_DIRECTORY");
        SettingsDirectory = settingsDirectory ?? overrideDirectory ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "labplus");
        if (!Path.IsPathFullyQualified(SettingsDirectory)) throw new ArgumentException("The LAB Plus data directory must be an absolute path.");
        if (settingsDirectory is null && overrideDirectory is null)
        {
            try { Migrated = LegacyDataMigration.CopyTo(SettingsDirectory); }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { MigrationNotice = "Previous data could not be fully imported: " + ex.Message; }
        }
    }

    public bool Migrated { get; }
    public string MigrationNotice { get; } = "";

    public string SettingsDirectory { get; }

    public string SettingsPath => Path.Combine(SettingsDirectory, "settings.json");
    public string ViewerSettingsPath => Path.Combine(SettingsDirectory, "viewer.ini");
    public string TriggerClassesPath => Path.Combine(SettingsDirectory, "trigger-classes.txt");
    public string TriggerPathsPath => Path.Combine(SettingsDirectory, "trigger-paths.tsv");
    public string ResearchCurrentPath => Path.Combine(SettingsDirectory, "research-current.txt");

    public LabPlusSettings Load()
    {
        try
        {
            if (!File.Exists(SettingsPath)) return new LabPlusSettings();
            var settings = JsonSerializer.Deserialize<LabPlusSettings>(File.ReadAllText(SettingsPath), JsonOptions)
                ?? new LabPlusSettings();
            settings.NormalizeTriggerClasses();
            return settings;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException)
        {
            return new LabPlusSettings();
        }
    }

    public void Save(LabPlusSettings settings)
    {
        settings.NormalizeTriggerClasses();
        Directory.CreateDirectory(SettingsDirectory);
        string temporaryPath = SettingsPath + ".tmp";
        File.WriteAllText(temporaryPath, JsonSerializer.Serialize(settings, JsonOptions));
        File.Move(temporaryPath, SettingsPath, overwrite: true);
        string viewerIni = $"[Viewer]{Environment.NewLine}" +
            $"Enabled={(settings.CollisionViewerEnabled ? 1 : 0)}{Environment.NewLine}" +
            $"ShowTriggers={(settings.ShowTriggers ? 1 : 0)}{Environment.NewLine}" +
            $"ShowCollision={(settings.ShowWorldCollision ? 1 : 0)}{Environment.NewLine}" +
            $"ShowShotCollision={(settings.ShowShotCollision ? 1 : 0)}{Environment.NewLine}" +
            $"TrimShotCollision={(settings.TrimShotCollision ? 1 : 0)}{Environment.NewLine}" +
            $"ShowSyncTarget={(settings.ShowSyncTarget ? 1 : 0)}{Environment.NewLine}" +
            $"SyncTargetAction={(settings.SyncTargetAction == 1 ? 1 : 0)}{Environment.NewLine}" +
            $"SyncGloryKillAnimation={LabPlusSettings.SyncAnimationKey(settings.SyncGloryKillAnimation)}{Environment.NewLine}" +
            $"SyncChainsawAnimation={LabPlusSettings.SyncAnimationKey(settings.SyncChainsawAnimation)}{Environment.NewLine}" +
            $"ShowWalls={(settings.ShowInvisibleWalls ? 1 : 0)}{Environment.NewLine}" +
            $"InspectVolumes={(settings.InspectVolumes ? 1 : 0)}{Environment.NewLine}" +
            $"ShowDisabledVolumes={(settings.ShowDisabledVolumes ? 1 : 0)}{Environment.NewLine}" +
            $"ShowVolumeHistory={(settings.ShowVolumeHistory ? 1 : 0)}{Environment.NewLine}" +
            $"ShowActiveEncountersHud={(settings.ShowActiveEncountersHud ? 1 : 0)}{Environment.NewLine}" +
            $"ShowEnemyHealthHud={(settings.ShowEnemyHealthHud ? 1 : 0)}{Environment.NewLine}" +
            $"ShowEnemyHealthBars={(settings.ShowEnemyHealthBars ? 1 : 0)}{Environment.NewLine}" +
            $"ShowGameTimersHud={(settings.ShowGameTimersHud ? 1 : 0)}{Environment.NewLine}" +
            $"ShowSpeedrunTechsHud={(settings.ShowSpeedrunTechsHud ? 1 : 0)}{Environment.NewLine}" +
            $"DebugHudScale={settings.DebugHudScale.ToString("R", CultureInfo.InvariantCulture)}{Environment.NewLine}" +
            $"ShowResearchPath={(settings.ShowResearchPath ? 1 : 0)}{Environment.NewLine}" +
            $"RecordResearch={(settings.RecordResearch ? 1 : 0)}{Environment.NewLine}" +
            $"ResearchProcess={settings.ResearchProcess}{Environment.NewLine}" +
            $"ResearchReset={settings.ResearchReset}{Environment.NewLine}" +
            $"ResearchMarker={settings.ResearchMarker}{Environment.NewLine}" +
            $"ResearchMarkerLabel={CleanMarker(settings.ResearchMarkerLabel)}{Environment.NewLine}" +
            $"MotionProcess={settings.MotionProcess}{Environment.NewLine}" +
            $"ShowPlayerCollider={(settings.ShowPlayerCollider ? 1 : 0)}{Environment.NewLine}" +
            $"ShowPlayerPositionHud={(settings.ShowPlayerPositionHud ? 1 : 0)}{Environment.NewLine}" +
            $"ShowPlayerAnglesHud={(settings.ShowPlayerAnglesHud ? 1 : 0)}{Environment.NewLine}" +
            $"ShowSupportHud={(settings.ShowSupportHud ? 1 : 0)}{Environment.NewLine}" +
            $"ShowMotionPaths={(settings.ShowMotionPaths ? 1 : 0)}{Environment.NewLine}" +
            $"MotionFollowTarget={(settings.MotionFollowTarget ? 1 : 0)}{Environment.NewLine}" +
            $"CollisionMode={(settings.Style == RenderStyle.Filled ? 1 : 0)}{Environment.NewLine}" +
            $"Distance={Math.Clamp(settings.ViewDistanceMeters, 20, 200)}{Environment.NewLine}";
        foreach (var (key, appearance) in new[] { ("Shot", settings.ShotAppearance), ("World", settings.WorldAppearance), ("Trigger", settings.TriggerAppearance), ("Wall", settings.WallAppearance), ("Inspector", settings.InspectorAppearance) })
            viewerIni += $"{key}Color={appearance.Color}{Environment.NewLine}{key}FillOpacity={appearance.FillOpacity}{Environment.NewLine}";
        viewerIni += $"{Environment.NewLine}[TriggerClasses]{Environment.NewLine}";
        foreach (string className in settings.HiddenTriggerClasses)
            viewerIni += $"{className}=0{Environment.NewLine}";
        viewerIni += $"{Environment.NewLine}[TriggerPaths]{Environment.NewLine}";
        foreach (var path in settings.HiddenTriggerPaths)
            viewerIni += $"{path.ClassName}|{path.Prefix}=0{Environment.NewLine}";
        string viewerTemporaryPath = ViewerSettingsPath + ".tmp";
        File.WriteAllText(viewerTemporaryPath, viewerIni, Encoding.Unicode);
        File.Move(viewerTemporaryPath, ViewerSettingsPath, overwrite: true);
    }

    public static string CleanMarker(string? text) => new((text ?? "").Where(c => !char.IsControl(c)).Take(160).ToArray());
}
