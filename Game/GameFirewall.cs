using System.Collections;
using System.Globalization;
using System.Reflection;
using System.Runtime.InteropServices;

namespace LabPlus.Game;

public sealed record GameFirewallRule(string Name, string Application, bool Enabled, int Direction, int Action, int Profiles,
    int Protocol = 256, string LocalAddresses = "*", string RemoteAddresses = "*", string InterfaceTypes = "All", string Service = "", bool LimitedInterfaces = false);
public sealed record GameFirewallState(int ActiveProfiles, int EnabledProfiles, bool ExcludedInterfaces, bool LocalPolicyEffective, IReadOnlyList<GameFirewallRule> Rules);
public sealed record GameFirewallResult(bool Blocked, string Message, string Rule = "");

// Read-only Windows Firewall inspection. No rules/settings are created or changed.
public static class GameFirewall
{
    private static string Normalize(string path)
    {
        try
        {
            path = Environment.ExpandEnvironmentVariables(path.Trim().Trim('"'));
            return Path.IsPathFullyQualified(path) ? Path.GetFullPath(path) : "";
        }
        catch (Exception ex) when (ex is ArgumentException or NotSupportedException) { return ""; }
    }
    public static GameFirewallResult Evaluate(string executable, GameFirewallState state)
    {
        string path = Normalize(executable);
        if (path.Length == 0) return new(false, "Select the full path to DOOMEternalx64vk.exe.");
        int active = state.ActiveProfiles;
        if (active == 0 || (active & ~7) != 0 || (state.EnabledProfiles & active) != active)
            return new(false, "Launch blocked: Windows Firewall is disabled for an active network profile, or its state is unknown.");
        if (state.ExcludedInterfaces || !state.LocalPolicyEffective)
            return new(false, "Launch blocked: an effective blocking rule could not be verified for every active network interface.");
        int covered = 0; var names = new List<string>();
        foreach (var rule in state.Rules)
        {
            if (!rule.Enabled || rule.Direction != 2 || rule.Action != 0 || rule.Protocol != 256 || rule.LimitedInterfaces || !string.IsNullOrWhiteSpace(rule.Service)
                || !rule.InterfaceTypes.Equals("All", StringComparison.OrdinalIgnoreCase)
                || !rule.LocalAddresses.Equals("*", StringComparison.Ordinal) || !rule.RemoteAddresses.Equals("*", StringComparison.Ordinal)
                || !string.Equals(Normalize(rule.Application), path, StringComparison.OrdinalIgnoreCase)) continue;
            covered |= rule.Profiles; names.Add(rule.Name);
        }
        if ((covered & active) != active)
            return new(false, "Launch blocked: enable internet blocking for the selected DOOM Eternal version in DESRU. An effective outbound Block rule must cover this executable, every protocol and every address.\n\n" + path);
        return new(true, "Game internet access is blocked.", string.Join(", ", names));
    }
    private static object? Get(object value, string property, params object[] arguments) => value.GetType().InvokeMember(property,
        BindingFlags.GetProperty, null, value, arguments, CultureInfo.InvariantCulture);
    private static bool HasItems(object? value) => value is Array array ? array.Length > 0 : value is string text && text.Length > 0;
    private static void Release(object? value) { if (value is not null && Marshal.IsComObject(value)) Marshal.FinalReleaseComObject(value); }
    public static GameFirewallResult Inspect(string executable)
    {
        object? policy = null, rules = null;
        try
        {
            var type = Type.GetTypeFromProgID("HNetCfg.FwPolicy2") ?? throw new InvalidOperationException("Windows Firewall API unavailable.");
            policy = Activator.CreateInstance(type) ?? throw new InvalidOperationException("Windows Firewall API unavailable.");
            int active = Convert.ToInt32(Get(policy, "CurrentProfileTypes"), CultureInfo.InvariantCulture), enabled = 0; bool excluded = false;
            foreach (int profile in new[] { 1, 2, 4 }) if ((active & profile) != 0)
            {
                if (Convert.ToBoolean(Get(policy, "FirewallEnabled", profile), CultureInfo.InvariantCulture)) enabled |= profile;
                excluded |= HasItems(Get(policy, "ExcludedInterfaces", profile));
            }
            bool effective = Convert.ToInt32(Get(policy, "LocalPolicyModifyState"), CultureInfo.InvariantCulture) == 0;
            rules = Get(policy, "Rules") ?? throw new InvalidOperationException("Firewall rules unavailable.");
            var list = new List<GameFirewallRule>(); string selected = Normalize(executable);
            foreach (object rule in (IEnumerable)rules)
            {
                try
                {
                    string application = Convert.ToString(Get(rule, "ApplicationName"), CultureInfo.InvariantCulture) ?? "";
                    if (!string.Equals(Normalize(application), selected, StringComparison.OrdinalIgnoreCase)) continue;
                    string Text(string field) => Convert.ToString(Get(rule, field), CultureInfo.InvariantCulture) ?? "";
                    int Number(string field) => Convert.ToInt32(Get(rule, field), CultureInfo.InvariantCulture);
                    list.Add(new(Text("Name"), application, Convert.ToBoolean(Get(rule, "Enabled"), CultureInfo.InvariantCulture), Number("Direction"), Number("Action"), Number("Profiles"),
                        Number("Protocol"), Text("LocalAddresses"), Text("RemoteAddresses"), Text("InterfaceTypes"), Text("ServiceName"), HasItems(Get(rule, "Interfaces"))));
                }
                finally { Release(rule); }
            }
            return Evaluate(executable, new(active, enabled, excluded, effective, list));
        }
        catch (Exception ex) when (ex is COMException or TargetInvocationException or InvalidOperationException or UnauthorizedAccessException or ArgumentException or NotSupportedException or InvalidCastException or System.Security.SecurityException)
        {
            return new(false, "Launch blocked: Windows Firewall could not be inspected. Check its service and the blocking rule for the selected executable.\n\n" + ex.GetBaseException().Message);
        }
        finally { Release(rules); Release(policy); }
    }
    public static void RequireBlocked(GameFirewallResult result)
    {
        if (!result.Blocked) throw new InvalidOperationException(result.Message);
    }
}
