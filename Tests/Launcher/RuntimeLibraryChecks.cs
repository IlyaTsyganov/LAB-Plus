using System.Security.Cryptography;
using System.Text.Json;
using LabPlus.Configuration;
using LabPlus.Game;

internal static class RuntimeLibraryChecks
{
    internal static void Run(string directory)
    {
        UnchangedStatusReusesHash(directory);
        MetadataChangesRefreshHash(directory);
        MissingFileClearsCache(directory);
        FailedHashClearsCache(directory);
        ConcurrentStatusSharesHash(directory);
        LaunchRechecksContentAndPreservesOldCopies(directory);
        LockedSourceBlocksPreparation(directory);
        DifferentSourcesKeepSeparateIdentities(directory);
        Console.WriteLine("PASS: runtime DLL identity: 100 unchanged status reads use one streamed hash; metadata/error invalidation, concurrency, forced launch verification, source locking and versioned copies.");
    }

    private static void UnchangedStatusReusesHash(string directory)
    {
        var fixture = new LibraryFixture(directory);
        string expected = Name(fixture.Bytes);
        for (int i = 0; i < 100; i++)
            Require(fixture.Registration.ExpectedRuntimeLibraryName == expected, "Runtime filename format changed.");
        Require(fixture.HashCalls == 1, "Unchanged status checks re-read and re-hashed the DLL.");
    }

    private static void MetadataChangesRefreshHash(string directory)
    {
        var fixture = new LibraryFixture(directory);
        _ = fixture.Registration.ExpectedRuntimeLibraryName;
        fixture.Write([9, 8, 7, 6]);
        File.SetLastWriteTimeUtc(fixture.SourcePath, LibraryFixture.Timestamp.AddMinutes(1));
        Require(fixture.Registration.ExpectedRuntimeLibraryName == Name(fixture.Bytes), "A same-size edit was not detected.");
        Require(fixture.HashCalls == 2, "A changed write timestamp did not refresh the hash.");

        File.SetCreationTimeUtc(fixture.SourcePath, LibraryFixture.Timestamp.AddMinutes(2));
        _ = fixture.Registration.ExpectedRuntimeLibraryName;
        Require(fixture.HashCalls == 3, "A changed creation timestamp did not refresh the hash.");

        var writeTime = File.GetLastWriteTimeUtc(fixture.SourcePath);
        fixture.Write([5, 4, 3, 2, 1]);
        File.SetLastWriteTimeUtc(fixture.SourcePath, writeTime);
        Require(fixture.Registration.ExpectedRuntimeLibraryName == Name(fixture.Bytes), "A size change with preserved timestamps was not detected.");
        Require(fixture.HashCalls == 4, "A changed file size did not refresh the hash.");
    }

    private static void MissingFileClearsCache(string directory)
    {
        var fixture = new LibraryFixture(directory);
        _ = fixture.Registration.ExpectedRuntimeLibraryName;
        var stamp = fixture.Stamp();
        File.Delete(fixture.SourcePath);
        Throws<IOException>(() => _ = fixture.Registration.ExpectedRuntimeLibraryName);
        fixture.Write([9, 9, 9, 9]); fixture.Restore(stamp);
        Require(fixture.Registration.ExpectedRuntimeLibraryName == Name(fixture.Bytes), "A missing file left a stale cached identity.");
        Require(fixture.HashCalls == 2, "A recreated source did not recover from the missing-file error.");
    }

    private static void FailedHashClearsCache(string directory)
    {
        var fixture = new LibraryFixture(directory);
        _ = fixture.Registration.ExpectedRuntimeLibraryName;
        var stamp = fixture.Stamp();
        fixture.Write([8, 8, 8, 8]);
        File.SetLastWriteTimeUtc(fixture.SourcePath, LibraryFixture.Timestamp.AddMinutes(1));
        fixture.FailNextHash = true;
        Throws<IOException>(() => _ = fixture.Registration.ExpectedRuntimeLibraryName);
        fixture.Restore(stamp);
        Require(fixture.Registration.ExpectedRuntimeLibraryName == Name(fixture.Bytes), "A failed read left an old identity usable after recovery.");
        Require(fixture.HashCalls == 3, "Failed hashing was cached instead of retried.");
    }

    private static void ConcurrentStatusSharesHash(string directory)
    {
        var fixture = new LibraryFixture(directory);
        var names = new string[64];
        Parallel.For(0, names.Length, i => names[i] = fixture.Registration.ExpectedRuntimeLibraryName);
        Require(names.All(name => name == Name(fixture.Bytes)), "Concurrent readers observed different identities.");
        Require(fixture.HashCalls == 1, "Concurrent readers hashed the same unchanged DLL repeatedly.");
    }

    private static void LaunchRechecksContentAndPreservesOldCopies(string directory)
    {
        var fixture = new LibraryFixture(directory);
        string oldName = fixture.Registration.ExpectedRuntimeLibraryName;
        byte[] oldBytes = fixture.Bytes;
        fixture.Registration.PrepareRuntime(beforeObs: true);
        Require(fixture.HashCalls == 2, "Launch preparation reused a status-only cache entry.");

        var stamp = fixture.Stamp();
        fixture.Write([7, 7, 7, 7]); fixture.Restore(stamp);
        Require(fixture.Stamp() == stamp, "The preserved-metadata fixture is invalid.");
        fixture.DuringHash = () => Throws<IOException>(() => File.WriteAllBytes(fixture.SourcePath, [0]));
        fixture.Registration.PrepareRuntime(beforeObs: true);
        string newName = fixture.Registration.ExpectedRuntimeLibraryName;
        Require(newName == Name(fixture.Bytes) && newName != oldName, "Launch preparation missed content replaced with identical metadata.");
        Require(fixture.HashCalls == 3, "Prepared identity was not cached for subsequent status checks.");
        string runtime = fixture.Registration.RuntimeDirectory;
        Require(File.ReadAllBytes(Path.Combine(runtime, oldName)).SequenceEqual(oldBytes), "An older runtime DLL was changed.");
        Require(File.ReadAllBytes(Path.Combine(runtime, newName)).SequenceEqual(fixture.Bytes), "The copied DLL does not match its content-derived filename.");
        Require(!ProcessMonitor.UsesExpectedViewer([oldName], newName), "A running old DLL was reported as the new build.");
        Require(ProcessMonitor.UsesExpectedViewer([newName], newName), "The new DLL was not recognized.");
        CheckManifest(Path.Combine(runtime, "labplus-layer.json"), $@".\{newName}");
        CheckManifest(Path.Combine(runtime, "capture", "labplus-layer.json"), $@"..\{newName}");
    }

    private static void LockedSourceBlocksPreparation(string directory)
    {
        var fixture = new LibraryFixture(directory);
        string expected = fixture.Registration.ExpectedRuntimeLibraryName;
        using (var writer = File.Open(fixture.SourcePath, FileMode.Open, FileAccess.ReadWrite, FileShare.None))
        {
            Require(fixture.Registration.ExpectedRuntimeLibraryName == expected, "A cache hit tried to open the DLL contents.");
            Require(fixture.HashCalls == 1, "A locked but unchanged source triggered another hash.");
            Throws<IOException>(() => fixture.Registration.PrepareRuntime());
            Require(!Directory.Exists(fixture.Registration.RuntimeDirectory), "Failed verification published runtime state.");
        }
        fixture.Registration.PrepareRuntime();
        Require(fixture.HashCalls == 2, "Preparation did not recover after the source lock was released.");
    }

    private static void DifferentSourcesKeepSeparateIdentities(string directory)
    {
        var first = new LibraryFixture(directory);
        var second = new LibraryFixture(directory);
        second.Write([6, 6, 6, 6]); second.Restore(first.Stamp());
        Require(first.Registration.ExpectedRuntimeLibraryName == Name(first.Bytes), "The first source lost its identity.");
        Require(second.Registration.ExpectedRuntimeLibraryName == Name(second.Bytes), "Different sources shared a cached identity.");
        Require(first.HashCalls == 1 && second.HashCalls == 1, "Source caches were not independent.");
    }

    private static void CheckManifest(string path, string expected)
    {
        using var json = JsonDocument.Parse(File.ReadAllText(path));
        Require(json.RootElement.GetProperty("layer").GetProperty("library_path").GetString() == expected,
            "The layer manifest does not reference the verified runtime DLL.");
    }

    private static string Name(byte[] bytes) => $"labplus-layer-{Convert.ToHexString(SHA256.HashData(bytes))[..16].ToLowerInvariant()}.dll";
    private static void Require(bool value, string message) { if (!value) throw new Exception(message); }
    private static void Throws<T>(Action action) where T : Exception
    {
        try { action(); }
        catch (T) { return; }
        throw new Exception("Expected " + typeof(T).Name);
    }

    private sealed class LibraryFixture
    {
        internal static readonly DateTime Timestamp = new(2020, 1, 1, 0, 0, 0, DateTimeKind.Utc);
        internal string SourcePath { get; }
        internal byte[] Bytes { get; private set; } = [1, 2, 3, 4];
        internal int HashCalls { get; private set; }
        internal bool FailNextHash;
        internal Action? DuringHash;
        internal VulkanLayerRegistration Registration { get; }

        internal LibraryFixture(string directory)
        {
            string root = Path.Combine(directory, "runtime-library", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(root);
            SourcePath = Path.Combine(root, "labplus-layer.dll");
            Write(Bytes); File.SetLastWriteTimeUtc(SourcePath, Timestamp);
            var identity = new RuntimeLibraryIdentity(SourcePath, Hash);
            Registration = new(new SettingsStore(Path.Combine(root, "settings")), identity);
        }

        internal void Write(byte[] bytes) { File.WriteAllBytes(SourcePath, bytes); Bytes = bytes; }
        internal (DateTime Created, DateTime Written) Stamp() => (File.GetCreationTimeUtc(SourcePath), File.GetLastWriteTimeUtc(SourcePath));
        internal void Restore((DateTime Created, DateTime Written) stamp)
        {
            File.SetCreationTimeUtc(SourcePath, stamp.Created);
            File.SetLastWriteTimeUtc(SourcePath, stamp.Written);
        }
        private byte[] Hash(Stream source)
        {
            HashCalls++;
            Require(source is FileStream, "DLL hashing must stream from the file.");
            if (FailNextHash) { FailNextHash = false; throw new IOException("Synthetic hash read failure."); }
            DuringHash?.Invoke();
            return SHA256.HashData(source);
        }
    }
}
