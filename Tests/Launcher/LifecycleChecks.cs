using LabPlus.Game;

internal static class LifecycleChecks
{
    internal static void Run()
    {
        static void Check(bool valid, string message) { if (!valid) throw new Exception(message); }
        string name = @"Local\labplus-instance-test-" + Guid.NewGuid().ToString("N");
        using var ready = new ManualResetEventSlim(); using var finish = new ManualResetEventSlim();
        var owner = new Thread(() => { using var lease = SingleInstance.TryAcquire(name); Check(lease is not null, "First instance rejected"); ready.Set(); finish.Wait(); });
        owner.Start(); Check(ready.Wait(2000), "Instance owner did not start");
        try { using var duplicate = SingleInstance.TryAcquire(name); Check(duplicate is null, "A second instance acquired ownership"); }
        finally { finish.Set(); owner.Join(); }
        using (var restarted = SingleInstance.TryAcquire(name)) Check(restarted is not null, "Normal shutdown prevented restart");
        using var retained = new Mutex(false, name);
        var crashed = new Thread(() => { _ = SingleInstance.TryAcquire(name) ?? throw new Exception("Crash fixture cannot own mutex"); });
        crashed.Start(); crashed.Join();
        using (var recovered = SingleInstance.TryAcquire(name)) Check(recovered is not null, "Abandoned mutex prevented recovery");

        string pipe = "labplus-lifecycle-test-" + Guid.NewGuid().ToString("N"), presenceName = @"Local\" + pipe;
        var server = new LabCommandServer((_, _) => { }, pipe, presenceName);
        using var reader = EventWaitHandle.OpenExisting(presenceName);
        Check(reader.WaitOne(0), "Running server is unavailable");
        server.Stop(); Check(!reader.WaitOne(0), "Stopping left a retained presence handle signaled");
        server.Dispose(); server.Dispose(); Check(!reader.WaitOne(0), "Disposal reactivated commands");
        using (var next = new LabCommandServer((_, _) => { }, pipe, presenceName)) Check(reader.WaitOne(0), "Restart failed to signal presence");
        Check(!reader.WaitOne(0), "Restarted server failed to reset the same retained event");
        Console.WriteLine("PASS: single instance, normal/crash recovery, command shutdown with retained handles, restart and idempotent disposal.");
    }
}
