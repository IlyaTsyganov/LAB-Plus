using System.Diagnostics;
using System.Runtime.InteropServices;
using LabPlus.Game;
using LabPlus.Hud.Memory;

namespace LabPlus.Hud.Runtime;

internal sealed class NativeCheckpointDelay(Process game, IntPtr site, IntPtr cooldown, bool hasNoDelay = false) : IDisposable
{
    private const byte OriginalBranch = 0x74;
    private const byte BypassBranch = 0xEB;
    private static readonly byte[][] TimerSignatures =
    [
        Convert.FromHexString("4C39B8E01500007423BA88130000"),
        Convert.FromHexString("4C39B0E0150000741FBA88130000"),
        Convert.FromHexString("4C39A0E0150000741EBA88130000"),
        Convert.FromHexString("4C39B8E0150000741EBA88130000"),
        Convert.FromHexString("83B8E0150000007421BA88130000")
    ];

    private const long NoCooldown = -1;
    private IntPtr _cooldownValue;
    private long _originalCooldown;
    private bool _patched;
    private bool _unsupported;
    private long _retryAt;
    private string? _failure;

    internal static NativeCheckpointDelay Attach(Process game, GameExecutableInfo? executable)
    {
        var image = game.MainModule!.BaseAddress;
        IntPtr Address(int rva) => rva == 0 ? IntPtr.Zero : image + rva;
        return new(game, Address(executable?.CheckpointDelayBranch ?? 0), Address(executable?.PauseMenuDelayCvar ?? 0),
            executable?.Version is "1.0" or "1.1");
    }

    internal string Update(bool enabled, long now)
    {
        if (site == IntPtr.Zero)
            return !enabled ? "Off" : hasNoDelay ? "No delay in this game version" : "Unavailable for this game build";
        if (_unsupported && enabled) return "Unavailable for this game build";

        bool cooldownApplied = cooldown == IntPtr.Zero || _cooldownValue != IntPtr.Zero;
        if (_failure is null && (enabled ? _patched && cooldownApplied : !_patched && _cooldownValue == IntPtr.Zero))
            return enabled ? "Active" : "Off";
        if (now < _retryAt) return _failure!;

        try
        {
            if (enabled)
            {
                ApplyCooldown();
                SetCheckpointBypass(true);
            }
            else
                Restore();
            _failure = null;
            return enabled ? "Active" : "Off";
        }
        catch (NotSupportedException error)
        {
            _unsupported = true;
            Restore();
            _failure = null;
            Log.Warning(error, "Checkpoint delay patch unavailable");
            return "Unavailable for this game build";
        }
        catch (Exception error)
        {
            _retryAt = now + 3000;
            _failure = "Waiting · could not update game";
            Log.Warning(error, "Checkpoint delay patch will retry");
            return _failure;
        }
    }

    private void ApplyCooldown()
    {
        if (cooldown == IntPtr.Zero || _cooldownValue != IntPtr.Zero) return;

        IntPtr data = game.ReadPointer(cooldown);
        if (data == IntPtr.Zero) throw new IOException("Pause menu cooldown cvar is not ready.");
        IntPtr name = game.ReadPointer(data + 0x28);
        const string expectedName = "pauseMenu_delayTimeAfterSave";
        if (game.ReadString(name, ReadStringType.ASCII, expectedName.Length + 1) != expectedName)
            throw new NotSupportedException("Unexpected pause menu cooldown cvar.");

        IntPtr value = data + 0x10;
        if (!game.ReadValue(value, out long original) || !game.WriteBytes(value, BitConverter.GetBytes(NoCooldown)))
            throw new IOException("Cannot update the pause menu cooldown.");
        // The comparison is inclusive. Use one tick below zero so the menu
        // also skips the delay when opened in the same tick as the request.
        _originalCooldown = original;
        _cooldownValue = value;
    }

    private void Restore()
    {
        try
        {
            if (_patched) SetCheckpointBypass(false);
        }
        finally
        {
            if (_cooldownValue != IntPtr.Zero)
            {
                // Keep a newer value if the game or user changed the cvar.
                IntPtr data = game.ReadPointer(cooldown);
                if (data == IntPtr.Zero) throw new IOException("Cannot read the pause menu cooldown cvar.");
                if (data + 0x10 == _cooldownValue)
                {
                    if (!game.ReadValue(_cooldownValue, out long current) ||
                        (current == NoCooldown && !game.WriteBytes(_cooldownValue, BitConverter.GetBytes(_originalCooldown))))
                        throw new IOException("Cannot restore the pause menu cooldown.");
                }
                _cooldownValue = IntPtr.Zero;
            }
        }
    }

    private void SetCheckpointBypass(bool enabled)
    {
        var bytes = game.ReadBytes(site - 7, 14) ?? throw new IOException("Cannot read checkpoint delay branch.");
        byte expected = _patched ? BypassBranch : OriginalBranch;
        if (bytes[7] != expected)
            throw new NotSupportedException("Unexpected checkpoint delay branch.");
        bytes[7] = OriginalBranch;
        if (!TimerSignatures.Any(signature => bytes.AsSpan().SequenceEqual(signature)))
            throw new NotSupportedException("Unexpected checkpoint delay timer instructions.");

        if (_patched != enabled)
        {
            // Treat the timer as unset. The branch keeps its original target and
            // all other pause-menu checks still run.
            if (!game.VirtualProtect(site, 1, MemPageProtect.PAGE_EXECUTE_READWRITE, out var protection))
                throw new IOException("Cannot change checkpoint delay branch protection.");
            try
            {
                if (!game.WriteBytes(site, [enabled ? BypassBranch : OriginalBranch]))
                    throw new IOException("Cannot write checkpoint delay branch.");
                _patched = enabled;
            }
            finally
            {
                if (!game.VirtualProtect(site, 1, protection))
                    throw new IOException("Cannot restore checkpoint delay branch protection.");
            }
        }

        if (!FlushInstructionCache(game.Handle, site, (UIntPtr)1))
            throw new IOException("Cannot flush checkpoint delay branch.");
    }

    public void Dispose()
    {
        if ((_patched || _cooldownValue != IntPtr.Zero) && !game.HasExited) Restore();
    }

    [DllImport("kernel32.dll")]
    private static extern bool FlushInstructionCache(IntPtr process, IntPtr address, UIntPtr size);
}
