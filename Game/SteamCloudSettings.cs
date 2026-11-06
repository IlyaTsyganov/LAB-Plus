using Microsoft.Win32;
using System.Text;

namespace LabPlus.Game;

public sealed record SteamCloudPreference(bool? Disabled, string Detail)
{
    public static SteamCloudPreference Unknown(string reason) => new(null, reason);
}

// Read-only Steam preferences for the account selected by Steam. The native
// RemoteStorage API remains authoritative for actual checkpoint operations.
public static class SteamCloudSettings
{
    private const int MaxBytes = 8 * 1024 * 1024;
    private static readonly string[] SharedSteam = ["UserRoamingConfigStore", "Software", "Valve", "Steam"];
    private static readonly string[] LocalSteam = ["UserLocalConfigStore", "Software", "Valve", "Steam"];

    public static SteamCloudPreference Read(string remote = "")
    {
        try
        {
            const string steamKey = @"HKEY_CURRENT_USER\Software\Valve\Steam";
            string? root = Registry.GetValue(steamKey, "SteamPath", null) as string;
            if (string.IsNullOrWhiteSpace(root) && Registry.GetValue(steamKey, "SteamExe", null) is string exe) root = Path.GetDirectoryName(exe);
            object? active = Registry.GetValue(steamKey + @"\ActiveProcess", "ActiveUser", null);
            uint account = active is int number ? unchecked((uint)number) : 0;
            if (account == 0 && Path.IsPathFullyQualified(remote))
            {
                var location = SteamSaveLocation.Parse(remote);
                account = checked((uint)(ulong.Parse(location.Account, System.Globalization.CultureInfo.InvariantCulture) - 76561197960265728UL));
                root ??= Directory.GetParent(location.Remote)?.Parent?.Parent?.Parent?.FullName;
            }
            if (account == 0 || string.IsNullOrWhiteSpace(root) || !Path.IsPathFullyQualified(root))
                return SteamCloudPreference.Unknown("Sign in to Steam or select your Steam save folder to check Cloud settings.");
            return Read(root, account);
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or System.Security.SecurityException or ArgumentException or OverflowException or FormatException)
        { return SteamCloudPreference.Unknown("Steam Cloud settings could not be read. Sign in to Steam and select the matching save folder."); }
    }

    public static SteamCloudPreference Read(string steamRoot, uint account)
    {
        try
        {
            string user = Path.Combine(steamRoot, "userdata", account.ToString(System.Globalization.CultureInfo.InvariantCulture));
            string? shared = ReadFile(Path.Combine(user, "7", "remote", "sharedconfig.vdf"));
            if (shared is not null)
            {
                var global = Flag(Parse(shared), [.. SharedSteam, "CloudEnabled"]);
                if (global is { Value: false, Invalid: false }) return new(true, "Steam Cloud is disabled in Steam settings.");
            }
            return Evaluate(shared, ReadFile(Path.Combine(user, "config", "localconfig.vdf")));
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or ArgumentException)
        { return SteamCloudPreference.Unknown("Steam Cloud settings are unavailable or are being updated. Retry after Steam finishes saving its settings."); }
    }

    public static SteamCloudPreference Evaluate(string? shared, string? local)
    {
        try
        {
            var sharedNodes = shared is null ? [] : Parse(shared);
            var global = Flag(sharedNodes, [.. SharedSteam, "CloudEnabled"]);
            if (!global.Invalid && global.Value == false) return new(true, "Steam Cloud is disabled in Steam settings.");
            var sharedApp = Flag(sharedNodes, [.. SharedSteam, "apps", "782330", "CloudEnabled"]);
            var localApp = Flag(local is null ? [] : Parse(local), [.. LocalSteam, "apps", "782330", "CloudEnabled"]);
            if (sharedApp.Invalid || localApp.Invalid || sharedApp.Value is not null && localApp.Value is not null && sharedApp.Value != localApp.Value)
                return SteamCloudPreference.Unknown("Steam contains conflicting Cloud settings. Check DOOM Eternal's Steam Cloud setting.");
            bool? app = localApp.Value ?? sharedApp.Value;
            if (app == false) return new(true, "Steam Cloud is disabled for DOOM Eternal in Steam settings.");
            // A valid Steam configuration without an app-specific override
            // uses the client's enabled-by-default application setting.
            if (!global.Invalid && global.Value == true) return new(false, "Steam Cloud is enabled in Steam settings.");
            return SteamCloudPreference.Unknown("Steam Cloud settings are not confirmed. Check Steam's Cloud settings.");
        }
        catch (IOException) { return SteamCloudPreference.Unknown("Steam Cloud settings could not be parsed. Retry after Steam finishes saving its settings."); }
    }

    private static string? ReadFile(string path)
    {
        if (!File.Exists(path)) return null;
        using var file = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        if (file.Length > MaxBytes) throw new IOException("Steam settings are too large.");
        using var reader = new StreamReader(file, new UTF8Encoding(false, true), true);
        string text = reader.ReadToEnd();
        if (text.Length > MaxBytes) throw new IOException("Steam settings are too large.");
        return text;
    }

    private sealed record Entry(string Name, string? Value, List<Entry>? Children);
    private readonly record struct FlagValue(bool? Value, bool Invalid = false);
    private static FlagValue Flag(List<Entry> nodes, string[] path)
    {
        for (int i = 0; i < path.Length; i++)
        {
            var matches = nodes.Where(n => n.Name.Equals(path[i], StringComparison.OrdinalIgnoreCase)).Take(2).ToArray();
            if (matches.Length == 0) return new(null);
            if (matches.Length != 1) return new(null, true);
            if (i == path.Length - 1) return matches[0].Value switch { "0" => new(false), "1" => new(true), _ => new(null, true) };
            if (matches[0].Children is not { } children) return new(null, true);
            nodes = children;
        }
        return new(null, true);
    }
    private static List<Entry> Parse(string text)
    {
        if (text.Length > MaxBytes) throw new IOException("Steam settings are too large.");
        int at = 0, count = 0;
        void Skip()
        {
            while (at < text.Length)
            {
                if (char.IsWhiteSpace(text[at]) || text[at] == '\ufeff') { at++; continue; }
                if (at + 1 < text.Length && text[at] == '/' && text[at + 1] == '/')
                { while (at < text.Length && text[at] != '\n') at++; continue; }
                break;
            }
        }
        string Token()
        {
            Skip(); if (at >= text.Length || text[at] is '{' or '}') throw new IOException("Malformed Steam settings.");
            var token = new StringBuilder();
            if (text[at] == '"')
            {
                at++;
                while (at < text.Length)
                {
                    char c = text[at++];
                    if (c == '"') return token.ToString();
                    if (c == '\\')
                    {
                        if (at >= text.Length) throw new IOException("Malformed Steam settings.");
                        c = text[at++];
                        c = c switch { 'n' => '\n', 't' => '\t', 'r' => '\r', _ => c };
                    }
                    token.Append(c); if (token.Length > 32768) throw new IOException("Malformed Steam settings.");
                }
                throw new IOException("Malformed Steam settings.");
            }
            while (at < text.Length && !char.IsWhiteSpace(text[at]) && text[at] is not ('{' or '}')) token.Append(text[at++]);
            if (token.Length == 0) throw new IOException("Malformed Steam settings.");
            return token.ToString();
        }
        List<Entry> Object(int depth, bool nested)
        {
            if (depth > 64) throw new IOException("Steam settings nesting is too deep.");
            var entries = new List<Entry>();
            for (;;)
            {
                Skip();
                if (at == text.Length) { if (nested) throw new IOException("Incomplete Steam settings."); return entries; }
                if (text[at] == '}') { if (!nested) throw new IOException("Malformed Steam settings."); at++; return entries; }
                string key = Token(); Skip();
                if (++count > 250000 || at == text.Length) throw new IOException("Malformed Steam settings.");
                if (text[at] == '{') { at++; entries.Add(new(key, null, Object(depth + 1, true))); }
                else entries.Add(new(key, Token(), null));
            }
        }
        return Object(0, false);
    }
}
