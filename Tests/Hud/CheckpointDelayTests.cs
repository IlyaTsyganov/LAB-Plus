using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using LabPlus.Hud.Memory;
using LabPlus.Hud.Runtime;

internal static class CheckpointDelayTests
{
    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate int CheckTimer(IntPtr notificationManager);

    internal static void Run(Action<bool, string> check)
    {
        using var game = Process.GetCurrentProcess();
        IntPtr memory = game.AllocateMemory(0x4000);
        try
        {
            check(memory != IntPtr.Zero, "Checkpoint delay fixture allocated");
            IntPtr branch = memory + 11;
            IntPtr cooldown = memory + 0x1000;
            IntPtr cvar = cooldown + 0x80;
            IntPtr timestamp = memory + 0x2000;
            const long original = 2499900000;
            // Use the game's comparison and short jump. Return 1 if the timer
            // ran, or 0 if it was unset or the branch bypassed it.
            byte[] code = new byte[49];
            Convert.FromHexString("488BC14883B8E0150000007421BA88130000B801000000C3").CopyTo(code, 0);
            Convert.FromHexString("33C0C3").CopyTo(code, 46);
            Marshal.Copy(code, 0, memory, code.Length);
            Marshal.WriteInt64(timestamp + 0x15e0, 1);
            Marshal.WriteIntPtr(cooldown, cvar);
            Marshal.WriteIntPtr(cvar + 0x28, cvar + 0x80);
            Marshal.WriteInt64(cvar + 0x10, original);
            byte[] name = Encoding.ASCII.GetBytes("pauseMenu_delayTimeAfterSave\0");
            Marshal.Copy(name, 0, cvar + 0x80, name.Length);
            check(game.VirtualProtect(memory, 0x1000, MemPageProtect.PAGE_EXECUTE_READ), "Checkpoint fixture is executable");
            var timer = Marshal.GetDelegateForFunctionPointer<CheckTimer>(memory);

            using (var patch = new NativeCheckpointDelay(game, branch, cooldown))
            {
                check(patch.Update(false, 0) == "Off" && timer(timestamp) == 1 && Marshal.ReadInt64(cvar + 0x10) == original,
                    "Disabled preference leaves both timers intact");
                check(patch.Update(true, 1) == "Active" && timer(timestamp) == 0 && Marshal.ReadInt64(cvar + 0x10) == -1,
                    "Enabling bypasses the checkpoint timer and the shared cooldown");
                check(patch.Update(false, 2) == "Off" && timer(timestamp) == 1 && Marshal.ReadInt64(cvar + 0x10) == original,
                    "Disabling restores the original branch and cooldown");
                check(patch.Update(true, 3) == "Active", "Checkpoint delay removal can be re-enabled");
            }
            check(game.ReadBytes(memory, code.Length)!.SequenceEqual(code) && Marshal.ReadInt64(cvar + 0x10) == original,
                "Disposal restores the exact code and original cooldown");

            using var unknown = new NativeCheckpointDelay(game, branch + 1, cooldown);
            check(unknown.Update(true, 0) == "Unavailable for this game build" && Marshal.ReadInt64(cvar + 0x10) == original,
                "An unrecognized branch is rejected and the cooldown is restored");
            using var old = new NativeCheckpointDelay(game, IntPtr.Zero, IntPtr.Zero, hasNoDelay: true);
            check(old.Update(true, 0) == "No delay in this game version", "Early patches need no modification");
        }
        finally
        {
            if (memory != IntPtr.Zero) game.FreeMemory(memory);
        }
    }
}
