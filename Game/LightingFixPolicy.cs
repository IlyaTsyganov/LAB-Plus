namespace LabPlus.Game;

internal static class LightingFixPolicy
{
    internal const string EnvironmentVariable = "LABPLUS_LIGHTING_FIX";

    internal static bool NeedsFix(string gamePath) => GameExecutableInfo.Read(gamePath)?.NeedsLightingFix == true;
}
