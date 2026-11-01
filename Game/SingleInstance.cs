namespace LabPlus.Game;

internal sealed class SingleInstance : IDisposable
{
    internal const string OwnerName = @"Local\LABPlus.App.Owner";
    internal const string Warning = "LAB Plus is already running. Close the existing instance before opening another one.";
    private readonly Mutex _mutex;
    private bool _owned;

    private SingleInstance(Mutex mutex) { _mutex = mutex; _owned = true; }

    internal static SingleInstance? TryAcquire(string name = OwnerName)
    {
        var mutex = new Mutex(false, name);
        bool acquired;
        try { acquired = mutex.WaitOne(0); }
        catch (AbandonedMutexException) { acquired = true; }
        if (acquired) return new SingleInstance(mutex);
        mutex.Dispose(); return null;
    }

    internal static bool OlderInstanceRunning()
    {
        using var current = System.Diagnostics.Process.GetCurrentProcess();
        foreach (var process in System.Diagnostics.Process.GetProcessesByName("LABPlus"))
        {
            using (process)
            {
                try { if (process.Id != current.Id && !process.HasExited && process.SessionId == current.SessionId && process.StartTime < current.StartTime) return true; }
                catch (Exception ex) when (ex is InvalidOperationException or System.ComponentModel.Win32Exception) { }
            }
        }
        return false;
    }

    public void Dispose()
    {
        if (!_owned) return;
        _owned = false; _mutex.ReleaseMutex(); _mutex.Dispose();
    }
}
