using LabPlus;
using LabPlus.Updates;
using System.Text.Json;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        string output = Path.GetFullPath(args.Length > 0 ? args[0] : Path.Combine(AppContext.BaseDirectory, "preview-output")); Directory.CreateDirectory(output);
        Environment.SetEnvironmentVariable("LABPLUS_PREVIEW", "1");
        Environment.SetEnvironmentVariable("LABPLUS_DATA_DIRECTORY", Path.Combine(output, "isolated-data"));
        Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
        Application.SetUnhandledExceptionMode(UnhandledExceptionMode.ThrowException);
        LabPlusTheme.Initialize(); WorkspaceWindows.Initialize();
        Exception? failure = null;
        WorkspaceWindows.StartupFailed = error => failure = error;
        using var context = new ApplicationContext();
        int exit = 1;
        try
        {
            WorkspaceWindows.Dispatch(() =>
            {
            try
            {
            foreach (var (label, expected) in new[] { ("No", DialogResult.No), ("Don't ask again for this version", DialogResult.Ignore), ("Yes", DialogResult.Yes) })
            {
                using var prompt = StartupUpdates.Prompt("v1.1");
                prompt.Shown += async (_, _) =>
                {
                    try
                    {
                        await prompt.Workspace!.Ready;
                        var browser = prompt.Workspace.Browser;
                        string data = await browser.ExecuteScriptAsync("JSON.stringify({version:document.querySelector('#app-version').textContent,font:getComputedStyle(document.querySelector('#app-version')).fontFamily,size:getComputedStyle(document.querySelector('#app-version')).fontSize,weight:getComputedStyle(document.querySelector('#app-version')).fontWeight,buttons:[...document.querySelectorAll('#control-root button')].map(b=>b.textContent.trim()),text:document.body.innerText,overflow:document.documentElement.scrollWidth>innerWidth})");
                        using var json = JsonDocument.Parse(JsonSerializer.Deserialize<string>(data)!);
                        var state = json.RootElement;
                        if (state.GetProperty("version").GetString() != AppVersion.Display || !state.GetProperty("font").GetString()!.Contains("LP UI")
                            || state.GetProperty("weight").GetString() != "400" || state.GetProperty("size").GetString() != "12px"
                            || state.GetProperty("buttons").GetArrayLength() != 3 || state.GetProperty("overflow").GetBoolean() || prompt.ClientSize != new Size(600, 300)
                            || !state.GetProperty("text").GetString()!.Contains("A new version of LAB Plus is available: v1.1.")) throw new Exception("Update prompt text, font or layout is incorrect: " + data);
                        if (expected == DialogResult.No)
                        {
                            await using var image = File.Create(Path.Combine(output, "update-prompt.png"));
                            await browser.CoreWebView2.CapturePreviewAsync(Microsoft.Web.WebView2.Core.CoreWebView2CapturePreviewImageFormat.Png, image);
                        }
                        await browser.ExecuteScriptAsync("[...document.querySelectorAll('#control-root button')].find(b=>b.textContent.trim()===" + JsonSerializer.Serialize(label) + ").click()");
                    }
                    catch (Exception error) { failure = error; prompt.Close(); }
                };
                var result = prompt.ShowDialog();
                if (failure is not null) throw failure;
                if (result != expected) throw new Exception("Wrong result for " + label + ": " + result);
            }
            Console.WriteLine("PASS: startup prompt rendered before any main window; all three English choices dispatch correctly; small regular Eternal UI font and version verified.");
            InstallLifecycle(output);
            exit = 0;
            }
            catch (Exception error) { Console.Error.WriteLine(error); File.WriteAllText(Path.Combine(output, "error.txt"), error.ToString()); }
            finally { context.ExitThread(); }
            });
            Application.Run(context);
            return exit;
        }
        catch (Exception error) { Console.Error.WriteLine(error); return 1; }
        finally { File.WriteAllText(Path.Combine(output, "result.txt"), exit == 0 ? "PASS" : "FAIL"); WorkspaceWindows.Shutdown(); }
    }

    private static void InstallLifecycle(string output)
    {
        string fixture = Path.Combine(output, "lifecycle-" + Guid.NewGuid().ToString("N"));
        string target = Path.Combine(fixture, "installation"), payload = Path.Combine(fixture, "release");
        var files = Directory.GetFiles(AppContext.BaseDirectory, "*", SearchOption.AllDirectories)
            .Select(file => (File: file, Relative: Path.GetRelativePath(AppContext.BaseDirectory, file)))
            .Where(file => UpdatePackage.IsRuntimeFile(file.Relative)).ToArray();
        foreach (var file in files)
            foreach (string directory in new[] { target, payload })
            {
                string destination = UpdatePackage.SafePath(directory, file.Relative);
                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                File.Copy(file.File, destination);
            }
        File.WriteAllText(Path.Combine(target, "Assets", "update-proof.txt"), "old");
        File.WriteAllText(Path.Combine(payload, "Assets", "update-proof.txt"), "new");
        File.WriteAllText(Path.Combine(target, "settings.json"), "user settings");
        Directory.CreateDirectory(Path.Combine(target, "logs"));
        File.WriteAllText(Path.Combine(target, "logs", "history.log"), "user history");
        string zip = Path.Combine(fixture, "release.zip");
        System.IO.Compression.ZipFile.CreateFromDirectory(payload, zip);
        byte[] bytes = File.ReadAllBytes(zip);
        using var client = new System.Net.Http.HttpClient(new PackageHandler(bytes));
        AppVersion.TryParse(AppVersion.Number, out var version);
        var release = new ReleaseUpdate(AppVersion.Display, version, new Uri("https://github.com/IlyaTsyganov/LAB-Plus/releases/download/test/LABPlus.zip"), bytes.Length,
            "sha256:" + Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(bytes)));
        string job = UpdateInstaller.PrepareAsync(client, release, target, default).GetAwaiter().GetResult();
        File.WriteAllText(Path.Combine(output, "integration.json"), JsonSerializer.Serialize(new { target, job }));
        UpdateInstaller.StartHelper(job);
        // The helper waits for this preview process to exit before installing.
        // The restarted app is a preview with isolated preferences and no game hooks.
    }

    private sealed class PackageHandler(byte[] package) : System.Net.Http.HttpMessageHandler
    {
        protected override Task<System.Net.Http.HttpResponseMessage> SendAsync(System.Net.Http.HttpRequestMessage request, CancellationToken token)
            => Task.FromResult(new System.Net.Http.HttpResponseMessage(System.Net.HttpStatusCode.OK) { Content = new System.Net.Http.ByteArrayContent(package) });
    }
}
