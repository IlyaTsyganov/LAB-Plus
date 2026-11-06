using LabPlus.Configuration;
using LabPlus.Game;
using System.Globalization;

internal static class InvisibleNumbersChecks
{
    internal static void Run(string directory)
    {
        static void Check(bool valid, string message) { if (!valid) throw new Exception(message); }
        string head = "#LABPLUS_NUMBERS_V1\t123\t1700000000000\tlive\tmap\tready\tpartial\tSome values unavailable\t1";
        string enemy = "E\t8\t2\tArena\t9\t3\tidAI2\timp\t125.5\t200";
        string timer = "T\tflame_belch\tEquipment\tFlame belch\tcooldown\t1.25\t";
        string tech = "S\tfast_dblade\tFast DBlade\tactive\tNext charge is accelerated";
        var culture = CultureInfo.CurrentCulture;
        try
        {
            CultureInfo.CurrentCulture = CultureInfo.GetCultureInfo("ru-RU");
            var parsed = InvisibleNumbersSnapshot.Parse([head, enemy, timer, tech]);
            var preview = InvisibleNumbersSnapshot.Parse([head, "P\t0\t9:3", "A\tkill_front_lower", "A\tkill_left_upper"]);
            Check(preview?.SyncPreview is { Action: 0, Target: "9:3" } && preview.SyncPreview.Animations.Count == 2, "Target-specific animation choices must survive the native protocol.");
            var automatic = InvisibleNumbersSnapshot.Parse([head, "P\t0\t9:3\tkill_front_chest", "A\tkill_above_back", "A\tkill_front_chest"]);
            Check(automatic?.SyncPreview is { AutomaticAnimation: "kill_front_chest" }, "The UI must receive the resolved automatic animation rather than assume alphabetical order.");
            Check(InvisibleNumbersSnapshot.Parse([head, "P\t0\t9:3\t", "A\tkill_front_chest"])?.SyncPreview?.AutomaticAnimation == "", "An automatic mode without a resolved area remains readable.");
            Check(InvisibleNumbersSnapshot.Parse([head, "P\t0\t9:3\tmissing", "A\tkill_front_chest"]) is null &&
                InvisibleNumbersSnapshot.Parse([head, "P\t0\t9:3\tbad=name", "A\tkill_front_chest"]) is null, "Invalid or foreign automatic selections must not leak into the target catalog.");
            Check(parsed?.SyncPreview is null, "Snapshots from older builds must remain readable.");
            var described = InvisibleNumbersSnapshot.Parse([head, "P\t0\t9:3\tkill_front_head", "A\tkill_front_head", "H\tkill_front_head\tHell Knight\tNo specific body part | top left / top right"]);
            Check(described?.SyncPreview?.Hints["kill_front_head"] is { Enemy: "Hell Knight", Aim: "No specific body part | top left / top right" }, "Per-enemy hints must not infer anatomy from animation names.");
            foreach (var rows in new[] { new[] { "H\tkill_front_head\tImp\tAim at head" }, new[] { "P\t0\t9:3", "A\tkill_front_head", "H\tforeign\tImp\tAim at head" }, new[] { "P\t0\t9:3", "A\tkill_front_head", "H\tkill_front_head\tImp\t" }, new[] { "P\t0\t9:3", "A\tkill_front_head", "H\tkill_front_head\tImp\tAim at head", "H\tkill_front_head\tImp\tAim at head" } })
                Check(InvisibleNumbersSnapshot.Parse(new[] { head }.Concat(rows).ToArray()) is null, "Orphaned, foreign, empty and duplicate aim hints must be rejected.");
            foreach (var rows in new[] { new[] { "A\torphan" }, new[] { "P\t2\t9:3" }, new[] { "P\t0\twrong" }, new[] { "P\t0\t9:3", "A\tkill_front", "A\tkill_front" }, new[] { "P\t0\t9:3", "A\tbad=name" } })
                Check(InvisibleNumbersSnapshot.Parse(new[] { head }.Concat(rows).ToArray()) is null, "Malformed animation choices must be rejected.");
            foreach (string state in new[] { "active", "inactive", "unavailable" })
            {
                var csc = InvisibleNumbersSnapshot.Parse([head, tech, $"S\tcsc\tCSC\t{state}\tCombo Swap Chain"]);
                Check(csc?.Techs.Count == 2 && csc.Techs[1].Label == "CSC" && csc.Techs[1].State == state, "Independent CSC state must survive the protocol");
            }
            Check(InvisibleNumbersSnapshot.Parse([head, "S\tcsc\tCSC\tBATS\twrong state"]) is null, "CSC cannot inherit BATS labels");
            foreach (string state in new[] { "active", "inactive", "unavailable" })
            {
                var bats = InvisibleNumbersSnapshot.Parse([head, $"S\tbats\tBATS\t{state}\tObserved state"]);
                Check(bats?.Techs[0].State == (state == "active" ? "BATS" : "inactive"), "BATS must display only active/inactive, including snapshots from an older native layer.");
            }
            foreach (string state in new[] { "BATS", "inverse BATS", "slower" }) {
                var observation = InvisibleNumbersSnapshot.Parse([head, $"S\tbats\tWeapon switch\t{state}\tObserved state"]);
                Check(observation?.Techs[0].State == state && observation.Techs[0].Label == "Weapon switch", "Weapon-switch state must survive protocol without becoming inactive");
            }
            Check(parsed is { Ready: true, Unavailable: 1 } && parsed.Enemies[0].Health == 125.5 && parsed.Timers[0].Seconds == 1.25 && parsed.Techs[0].State == "active", "Engine numbers must parse with invariant culture.");
            Check(parsed!.IsFresh(parsed.Captured.AddSeconds(1)) && !parsed.IsFresh(parsed.Captured.AddSeconds(3)), "Stale enemy and timer data must expire.");
            var maximum = InvisibleNumbersSnapshot.Parse([head, timer + "\t20", "T\tsticky_charge_1\tsticky_charges\tCharge 1\tcooldown\t0.75\tSequential recharge\t1.5"]);
            Check(maximum?.Timers[0].MaximumSeconds == 20 && maximum.Timers[1] is { Seconds: 0.75, MaximumSeconds: 1.5 }, "Maximum duration and individual Sticky charge countdowns must survive the native protocol.");
            Check(parsed.Timers[0].MaximumSeconds is null, "Older timer snapshots without maximum duration must remain readable.");
            Check(new LabPlusSettings { CollisionViewerEnabled = false, ShowSupportHud = true }.NeedsNativeLayer &&
                new LabPlusSettings { CollisionViewerEnabled = false, ShowActiveEncountersHud = true }.NeedsNativeLayer,
                "Support plane and encounter HUDs must request their Vulkan renderer independently of geometry visibility.");
            foreach (string invalid in new[] { "0", "-1", "NaN", "Infinity", "bad" })
                Check(InvisibleNumbersSnapshot.Parse([head, timer + "\t" + invalid]) is null, "Invalid maximum duration accepted: " + invalid);
            Check(InvisibleNumbersSnapshot.Parse([head, timer + "\t?"])?.Timers[0].MaximumSeconds is null, "Unknown maximum duration must remain explicit.");
            foreach (string bad in new[] { enemy.Replace("125.5", "NaN"), enemy.Replace("125.5", "Infinity"), enemy.Replace("125.5", "0"),
                timer.Replace("1.25", "-1"), timer.Replace("1.25", "?"), timer.Replace("cooldown", "guess") })
                Check(InvisibleNumbersSnapshot.Parse([head, bad]) is null, "Malformed or fabricated timer/health state accepted: " + bad);
            Check(InvisibleNumbersSnapshot.Parse([head, enemy, enemy]) is null && InvisibleNumbersSnapshot.Parse([head, timer, timer]) is null, "Duplicate entity generations or timer keys accepted.");
            Check(InvisibleNumbersSnapshot.Parse([head, tech, tech]) is null && InvisibleNumbersSnapshot.Parse([head, tech.Replace("\tactive\t", "\tguess\t")]) is null, "Duplicate or unverified technique states accepted.");
            var unowned = InvisibleNumbersSnapshot.Parse([head, timer.Replace("cooldown\t1.25", "not_acquired\t?")]);
            Check(unowned?.Timers[0] is { State: "not_acquired", Seconds: null }, "Uncollected equipment must preserve its explicit ownership state without a countdown.");
            Check(InvisibleNumbersSnapshot.Parse([head, enemy.Replace("\t200", "\t?"), timer.Replace("cooldown\t1.25", "unavailable\t?")]) is not null, "Unknown maximum HP and unavailable timers must stay explicit.");
            var settings = new LabPlusSettings();
            foreach (string command in new[] { "labplus_enemies", "labplus_enemy_hp_bars", "labplus_timers", "labplus_techs" })
                Check(LabCommands.Apply(settings, [command, "1"], 123, 1) && !LabCommands.Apply(settings, [command, "2"], 123, 1), "Number feature commands did not validate input.");
            Check(settings.ShowEnemyHealthHud && settings.ShowEnemyHealthBars && settings.ShowGameTimersHud && settings.ShowSpeedrunTechsHud, "Number settings did not apply.");
            Check(!settings.TrimShotCollision, "Projectile trim must default off.");
            Check(!settings.ShowSyncTarget, "Experimental enemy zones must default off.");
            Check(settings.SyncTargetAction == 0, "Sync preview must default to Glory Kill.");
            Check(new LabPlusSettings { ShowSyncTarget = true }.NeedsNativeLayer, "Enemy zones must load their renderer without Collision Viewer.");
            settings.TrimShotCollision = true;
            settings.ShowSyncTarget = true;
            settings.SyncTargetAction = 1;
            settings.SyncGloryKillAnimation = "kill_front_lower"; settings.SyncChainsawAnimation = "cut_back";
            var store = new SettingsStore(Path.Combine(directory, "invisible-numbers")); store.Save(settings);
            var loaded = store.Load();
            Check(loaded.ShowEnemyHealthHud && loaded.ShowEnemyHealthBars && loaded.ShowGameTimersHud && loaded.ShowSpeedrunTechsHud, "Number settings did not persist.");
            string ini = File.ReadAllText(store.ViewerSettingsPath);
            Check(loaded.TrimShotCollision && ini.Contains("TrimShotCollision=1"), "Experimental projectile trim must persist through JSON and the native INI transport.");
            Check(loaded.ShowSyncTarget && ini.Contains("ShowSyncTarget=1"), "Enemy hit zones must persist through JSON and native settings.");
            Check(loaded.SyncTargetAction == 1 && ini.Contains("SyncTargetAction=1"), "Chainsaw preview selection must persist through JSON and native settings.");
            Check(loaded.SyncGloryKillAnimation == "kill_front_lower" && loaded.SyncChainsawAnimation == "cut_back" && ini.Contains("SyncGloryKillAnimation=kill_front_lower") && ini.Contains("SyncChainsawAnimation=cut_back"), "Independent selected animations must persist to native settings.");
            Check(LabPlusSettings.SyncAnimationKey("cut_front\nEnabled=1") == "", "Animation names cannot introduce native settings lines.");
            Check(ini.Contains("ShowEnemyHealthHud=1") && ini.Contains("ShowEnemyHealthBars=1") && ini.Contains("ShowGameTimersHud=1"), "Number settings did not reach native INI transport.");
        }
        finally { CultureInfo.CurrentCulture = culture; }
    }
}
