using System.Text;

namespace LabPlus.Game;

public static class PracticeSaveCommand
{
    public const string Name = "labplus_load_last_save";
    public static bool TryParse(string[] args, out string token)
    {
        token = "";
        if (args.Length != 2 || args[0] != Name || !LevelStateSnapshot.ValidToken(args[1])) return false;
        token = args[1]; return true;
    }
    internal static bool Acknowledge(string directory, int process, string token) =>
        Write(directory, process, token, "accepted", "LAB Plus received the request.");

    public static bool Reply(string directory, int process, string token, bool success, string text) =>
        Write(directory, process, token, success ? "ok" : "error", text);

    private static bool Write(string directory, int process, string token, string phase, string text)
    {
        if (process <= 0 || !LevelStateSnapshot.ValidToken(token)) return false;
        string path = Path.Combine(directory, $"practice-command-{process}.tsv");
        try
        {
            PracticeSaveVault.NoLinks(path); PracticeSaveVault.NoLinks(path + ".tmp"); Directory.CreateDirectory(directory);
            text = new string(text.Select(c => char.IsControl(c) ? ' ' : c).Take(1024).ToArray());
            File.WriteAllText(path + ".tmp", $"#LABPLUS_SAVE_COMMAND_V1\t{process}\t{token}\t{phase}\t{text}\n", new UTF8Encoding(false));
            for (int attempt = 0; ; attempt++)
            {
                try { File.Move(path + ".tmp", path, true); return true; }
                catch (Exception ex) when (attempt < 9 && ex is IOException or UnauthorizedAccessException)
                { Thread.Sleep(20); PracticeSaveVault.NoLinks(path); }
            }
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { return false; }
    }
}
