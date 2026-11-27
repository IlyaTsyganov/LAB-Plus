using LabPlus.Configuration;
using System.Globalization;

namespace LabPlus.Game;

public static class LabCommands
{
    public static bool Apply(LabPlusSettings settings, string[] args, int gameProcess, long timestamp)
    {
        if (args.Length == 0) return false;
        if (args[0] == "labplus_clear" && args.Length == 1) { settings.ResearchReset = timestamp; return true; }
        if (args[0] == "labplus_mark" && args.Length > 1)
        {
            settings.ResearchMarkerLabel = SettingsStore.CleanMarker(string.Join(' ', args.Skip(1)));
            settings.ResearchMarker = timestamp; return true;
        }
        if (args[0] == "labplus_trigger" && args.Length == 3 && TriggerClassCatalog.IsValidName(args[1]) && args[2] is "0" or "1" or "toggle")
        { settings.SetTriggerClassVisible(args[1], args[2] == "toggle" ? !settings.IsTriggerClassVisible(args[1]) : args[2] == "1"); return true; }
        if (args[0] == "labplus_debughud_scale")
        {
            if (args.Length != 2 || !double.TryParse(args[1], NumberStyles.Float, CultureInfo.InvariantCulture, out double scale) ||
                !double.IsFinite(scale) || scale is < 0.5 or > 3) return false;
            settings.DebugHudScale = scale; return true;
        }
        if (args.Length != 2) return false;
        bool toggle = args[1] == "toggle";
        int number = 0;
        if (!toggle && !int.TryParse(args[1], out number)) return false;
        if (args[0] == "labplus_distance")
        {
            if (toggle || number is < 20 or > 200) return false;
            settings.ViewDistanceMeters = number; return true;
        }
        if (!toggle && number is not (0 or 1)) return false;
        // Toggle the live preference when the queued request is applied, so
        // consecutive key presses never derive their value from a stale INI.
        bool Value(bool current) => toggle ? !current : number == 1;
        switch (args[0])
        {
            case "labplus_viewer": settings.CollisionViewerEnabled = Value(settings.CollisionViewerEnabled); break;
            case "labplus_triggers": settings.ShowTriggers = Value(settings.ShowTriggers); break;
            case "labplus_collision": settings.ShowWorldCollision = Value(settings.ShowWorldCollision); break;
            case "labplus_shots": settings.ShowShotCollision = Value(settings.ShowShotCollision); break;
            case "labplus_walls": settings.ShowInvisibleWalls = Value(settings.ShowInvisibleWalls); break;
            case "labplus_inspector": settings.InspectVolumes = Value(settings.InspectVolumes); break;
            case "labplus_disabled": settings.ShowDisabledVolumes = Value(settings.ShowDisabledVolumes); break;
            case "labplus_history": settings.ShowVolumeHistory = Value(settings.ShowVolumeHistory); break;
            case "labplus_encounters": settings.ShowActiveEncountersHud = Value(settings.ShowActiveEncountersHud); break;
            case "labplus_enemies": settings.ShowEnemyHealthHud = Value(settings.ShowEnemyHealthHud); break;
            case "labplus_enemy_hp_bars": settings.ShowEnemyHealthBars = Value(settings.ShowEnemyHealthBars); break;
            case "labplus_timers": settings.ShowGameTimersHud = Value(settings.ShowGameTimersHud); break;
            case "labplus_techs": settings.ShowSpeedrunTechsHud = Value(settings.ShowSpeedrunTechsHud); break;
            case "labplus_fill": settings.Style = Value(settings.Style == RenderStyle.Filled) ? RenderStyle.Filled : RenderStyle.Outlines; break;
            case "labplus_path": settings.ShowResearchPath = Value(settings.ShowResearchPath); break;
            case "labplus_player": settings.ShowPlayerCollider = Value(settings.ShowPlayerCollider); settings.MotionProcess = gameProcess; break;
            case "labplus_position": settings.ShowPlayerPositionHud = Value(settings.ShowPlayerPositionHud); break;
            case "labplus_angles": settings.ShowPlayerAnglesHud = Value(settings.ShowPlayerAnglesHud); break;
            case "labplus_surface": settings.ShowSupportHud = Value(settings.ShowSupportHud); settings.MotionProcess = gameProcess; break;
            case "labplus_attempts": settings.ShowMotionPaths = Value(settings.ShowMotionPaths); settings.MotionProcess = gameProcess; break;
            case "labplus_record": settings.RecordResearch = Value(settings.RecordResearch); settings.ResearchProcess = gameProcess; break;
            default: return false;
        }
        return true;
    }
}
