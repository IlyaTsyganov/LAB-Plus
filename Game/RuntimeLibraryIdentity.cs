using System.Security.Cryptography;

namespace LabPlus.Game;

// Status polling checks file metadata. Launch preparation always refreshes the
// content hash, including replacements that preserve the metadata below.
internal sealed class RuntimeLibraryIdentity
{
    private readonly object _gate = new();
    private readonly Func<Stream, byte[]> _hash;
    private CachedIdentity? _cached;

    internal string SourcePath { get; }

    internal RuntimeLibraryIdentity(string sourcePath, Func<Stream, byte[]>? hash = null)
    {
        SourcePath = Path.GetFullPath(sourcePath);
        _hash = hash ?? SHA256.HashData;
    }

    internal string GetName(bool forceRefresh = false)
    {
        lock (_gate)
        {
            try
            {
                var stamp = ReadStamp();
                if (!forceRefresh && _cached is { } cached && cached.Stamp == stamp) return cached.Name;

                using var source = File.OpenRead(SourcePath);
                // Opening the read-only handle prevents writes/replacements.
                // Read metadata again in case the file changed before opening.
                stamp = ReadStamp();
                byte[] hash = _hash(source);
                string name = $"labplus-layer-{Convert.ToHexString(hash)[..16].ToLowerInvariant()}.dll";
                _cached = new(stamp, name);
                return name;
            }
            catch
            {
                // Do not reuse an old identity after a failed lookup or hash.
                _cached = null;
                throw;
            }
        }
    }

    private FileStamp ReadStamp()
    {
        var file = new FileInfo(SourcePath);
        file.Refresh();
        return new(file.Length, file.LastWriteTimeUtc, file.CreationTimeUtc);
    }

    private readonly record struct FileStamp(long Length, DateTime LastWriteUtc, DateTime CreatedUtc);
    private sealed record CachedIdentity(FileStamp Stamp, string Name);
}
