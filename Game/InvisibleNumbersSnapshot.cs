using System.Globalization;

namespace LabPlus.Game;

public sealed record EnemyNumber(uint EncounterId, uint EncounterGeneration, string Encounter, uint Id, uint Generation,
    string ClassName, string Name, double Health, double? MaximumHealth);
public sealed record TimerNumber(string Key, string Group, string Label, string State, double? Seconds, string Detail, double? MaximumSeconds = null);
public sealed record TechNumber(string Key, string Label, string State, string Detail);

public sealed record InvisibleNumbersSnapshot(int Process, DateTimeOffset Captured, string Phase, string Map,
    string EnemiesState, string TimersState, string Detail, int Unavailable, IReadOnlyList<EnemyNumber> Enemies, IReadOnlyList<TimerNumber> Timers, IReadOnlyList<TechNumber> Techs)
{
    public bool IsFresh(DateTimeOffset now) => Captured <= now.AddSeconds(5) && now - Captured <= TimeSpan.FromSeconds(2);
    public bool Ready => Phase is "live" or "paused";
    public static InvisibleNumbersSnapshot? Read(string directory, int process)
    {
        if (process <= 0) return null;
        string path = Path.Combine(directory, $"invisible-numbers-{process}.tsv");
        if (!File.Exists(path) || new FileInfo(path).Length > 2_000_000) return null;
        var result = Parse(File.ReadAllLines(path));
        return result?.Process == process ? result : null;
    }
    public static InvisibleNumbersSnapshot? Parse(string[] lines)
    {
        if (lines.Length is < 1 or > 8193) return null;
        var head = lines[0].Split('\t');
        if (head.Length != 9 || head[0] != "#LABPLUS_NUMBERS_V1" || !int.TryParse(head[1], out int process) || process <= 0 ||
            !long.TryParse(head[2], out long epoch) || !int.TryParse(head[8], out int unavailable) || unavailable < 0) return null;
        DateTimeOffset captured;
        try { captured = DateTimeOffset.FromUnixTimeMilliseconds(epoch); }
        catch (ArgumentOutOfRangeException) { return null; }
        if (head.Any(cell => cell.Length > 1024)) return null;
        var enemies = new List<EnemyNumber>(); var timers = new List<TimerNumber>(); var techs = new List<TechNumber>();
        var identities = new HashSet<(uint, uint, uint, uint)>(); var keys = new HashSet<string>(StringComparer.Ordinal);
        foreach (string line in lines.Skip(1))
        {
            var cell = line.Split('\t');
            if (cell.Any(value => value.Length > 1024)) return null;
            if (cell[0] == "E")
            {
                if (cell.Length != 10 || !uint.TryParse(cell[1], out uint encounter) || !uint.TryParse(cell[2], out uint encounterGeneration) ||
                    !uint.TryParse(cell[4], out uint id) || !uint.TryParse(cell[5], out uint generation) || !Number(cell[8], out double hp) || hp <= 0 ||
                    !Optional(cell[9], out double? max) || max is <= 0 || !identities.Add((encounter, encounterGeneration, id, generation))) return null;
                enemies.Add(new(encounter, encounterGeneration, cell[3], id, generation, cell[6], cell[7], hp, max));
            }
            else if (cell[0] == "T")
            {
                if (cell.Length is not (7 or 8) || string.IsNullOrWhiteSpace(cell[1]) || !keys.Add(cell[1]) || !Optional(cell[5], out double? seconds) || seconds < 0) return null;
                if (cell[4] is not ("ready" or "cooldown" or "active" or "inactive" or "not_acquired" or "unavailable")) return null;
                if ((cell[4] is "cooldown" or "active") && seconds is null) return null;
                double? maximum = null;
                if (cell.Length == 8 && (!Optional(cell[7], out maximum) || maximum is <= 0)) return null;
                timers.Add(new(cell[1], cell[2], cell[3], cell[4], seconds, cell[6], maximum));
            }
            else if (cell[0] == "S")
            {
                if (cell.Length != 5 || cell[1] is not ("bats" or "fast_dblade") || !keys.Add("tech:" + cell[1]) ||
                    cell[3] is not ("active" or "inactive" or "unavailable")) return null;
                string state = cell[1] == "bats" && cell[3] != "active" ? "inactive" : cell[3];
                techs.Add(new(cell[1], cell[2], state, cell[4]));
            }
            else return null;
        }
        return new(process, captured, head[3], head[4], head[5], head[6], head[7], unavailable, enemies, timers, techs);
    }
    private static bool Number(string text, out double number) => double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out number) && double.IsFinite(number);
    private static bool Optional(string text, out double? number)
    {
        number = null;
        if (text == "?") return true;
        if (!Number(text, out double value)) return false;
        number = value; return true;
    }
}
