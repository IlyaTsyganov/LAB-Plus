using LabPlus.Game;

internal static class SteamCloudSettingsChecks
{
    internal static void Run(string root)
    {
        static void Check(bool value, string message) { if (!value) throw new Exception(message); }
        static string Shared(string body) => "\"UserRoamingConfigStore\" { \"Software\" { \"Valve\" { \"Steam\" { " + body + " } } } }";
        static string Local(string body) => "\"UserLocalConfigStore\" { \"Software\" { \"Valve\" { \"Steam\" { " + body + " } } } }";
        foreach (var (shared, local, expected) in new (string?, string?, bool?)[] {
            (Shared("\"CloudEnabled\" \"0\""), null, true),
            (Shared("\"CloudEnabled\" \"0\""), Local("\"apps\" { \"782330\" { \"CloudEnabled\" \"1\" } }"), true),
            (Shared("\"CloudEnabled\" \"1\""), null, false),
            (Shared("\"CloudEnabled\" \"1\" \"apps\" { \"782330\" { \"CloudEnabled\" \"0\" } }"), null, true),
            (Shared("\"CloudEnabled\" \"1\""), Local("\"apps\" { \"782330\" { \"CloudEnabled\" \"0\" } }"), true),
            (Shared("\"CloudEnabled\" \"1\""), Local("\"apps\" { \"730\" { \"CloudEnabled\" \"0\" } }"), false),
            (Shared("\"CloudEnabled\" \"1\""), Local("\"apps\" { \"782330\" { \"cloud\" { \"last_sync_state\" \"0\" } } }"), false),
            (Shared("\"CloudEnabled\" \"1\" \"CloudEnabled\" \"0\""), null, null),
            (Shared("\"CloudEnabled\" \"1\" \"apps\" { \"782330\" { \"CloudEnabled\" \"0\" } }"), Local("\"apps\" { \"782330\" { \"CloudEnabled\" \"1\" } }"), null),
            (Shared("\"CloudEnabled\" \"2\""), null, null),
            ("\"Unrelated\" { \"CloudEnabled\" \"0\" }", null, null),
            ("\"UserRoamingConfigStore\" { \"CloudEnabled\" \"0\"", null, null),
            (null, null, null),
            (Shared("// comment\n\"CloudEnabled\" \"0\" \"Other\" \"escaped \\\"quote\\\"\""), null, true)
        }) Check(SteamCloudSettings.Evaluate(shared, local).Disabled == expected, "Cloud preference scope/default/ambiguity check failed");
        string steamRoot = Path.Combine(root, "steam-cloud-prelaunch");
        string folder = Path.Combine(steamRoot, "userdata", "10001", "7", "remote"); Directory.CreateDirectory(folder);
        string file = Path.Combine(folder, "sharedconfig.vdf");
        string off = Shared("\"CloudEnabled\" \"0\""); File.WriteAllText(file, off);
        Check(SteamCloudSettings.Read(steamRoot, 10001).Disabled == true, "Cloud OFF must be known without a game process");
        Check(File.ReadAllText(file) == off, "Cloud check modified Steam preferences");
        File.WriteAllText(file, Shared("\"CloudEnabled\" \"1\""));
        Check(SteamCloudSettings.Read(steamRoot, 10001).Disabled == false, "Cloud setting changes must not remain cached");
        Check(SteamCloudSettings.Read(steamRoot, 10002).Disabled is null, "A different Steam account inherited Cloud preferences");
        Console.WriteLine("PASS: read-only prelaunch Steam Cloud settings, global/per-game scope, account isolation, malformed/ambiguous input and updates.");
    }
}
