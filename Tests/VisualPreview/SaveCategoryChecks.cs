
using System.Text.Json;
using LabPlus;
using LabPlus.Game;

internal static class SaveCategoryChecks
{
    internal static async Task Run(LabPlusForm main, PracticeSaveVault vault, string data)
    {
        const string category = "Moved route";
        var before = new PracticeSaveLibrary(data).Categorized(vault.List());
        async Task OpenCategoryPicker()
        {
            const string isOpen = "document.querySelector('[data-presentation=saves-category-menu]')?.innerText.trim().startsWith('Categories') === true";
            if (!JsonSerializer.Deserialize<bool>(await Script(main, isOpen)))
                await Click(main, "Categories");
        }
        async Task ClickCategoryTrash()
        {
            await OpenCategoryPicker();
            await Check(main, """
                (() => {
                  const button = document.querySelector('button[aria-label="Delete category Moved route"]');
                  const picker = document.querySelector('[data-presentation=saves-folder-row] button');
                  const row = button?.closest('[data-presentation=saves-category-row]');
                  const option = row?.querySelector('[data-presentation=saves-category-active]');
                  return !!button?.querySelector('.lr-trash-icon') && !!picker?.querySelector('.lr-folder-icon')
                    && !!option && button.getBoundingClientRect().left >= option.getBoundingClientRect().right
                    && button.getBoundingClientRect().width === picker.getBoundingClientRect().width
                    && !document.querySelector('button[aria-label="Delete category All checkpoints"]');
                })()
                """, "Category trash is missing, misaligned, or offered for the aggregate view.");
            await Script(main, """document.querySelector('button[aria-label="Delete category Moved route"]').click()""");
        }

        LabPlusForm? Dialog() => Application.OpenForms.OfType<WorkspaceHost>().Select(h => h.Screen).FirstOrDefault(s => s is WorkspaceMessageForm);
        await ClickCategoryTrash();
        await Wait(() => Dialog()?.Workspace?.Ready.IsCompletedSuccessfully == true);
        await Click(Dialog()!, "Cancel");
        await Wait(() => Dialog() is null);
        if (new PracticeSaveLibrary(data).Category != category)
            throw new Exception("Cancel removed the selected category.");

        await ClickCategoryTrash();
        await Wait(() => Dialog()?.Workspace?.Ready.IsCompletedSuccessfully == true);
        await Click(Dialog()!, "Delete");
        await Wait(() => Dialog() is null && new PracticeSaveLibrary(data).Category == PracticeSaveLibrary.All);
        var library = new PracticeSaveLibrary(data);
        var after = library.Categorized(vault.List());
        if (after.Count != before.Count || library.Categories(after).Contains(category)
            || before.Any(s => !after.Any(a => a.Id == s.Id && a.Category ==
                (s.Category == category ? PracticeSaveLibrary.Uncategorized : s.Category))))
            throw new Exception("Deleting a category did not preserve all checkpoints or changed an unrelated category.");

        await OpenCategoryPicker();
        await Check(main, """
            document.querySelector('[data-presentation=saves-category-menu]').innerText.includes('Uncategorized')
              && !document.querySelector('button[aria-label="Delete category Uncategorized"]')
            """, "The fallback category is missing or deletable.");
        await Click(main, "Categories");
        Console.WriteLine("PASS: category trash layout and built-in protections, cancel, confirmed deletion, and checkpoint preservation.");
    }

    private static Task<string> Script(LabPlusForm form, string code) => form.Workspace!.Browser.ExecuteScriptAsync(code);

    private static async Task Click(LabPlusForm form, string label)
    {
        await Script(form, $"[...document.querySelectorAll('button')].find(button=>button.textContent.trim()==={JsonSerializer.Serialize(label)}).click()");
        if (!form.IsDisposed) { form.Workspace!.Publish(true); await Task.Delay(180); }
    }

    private static async Task Check(LabPlusForm form, string code, string message)
    {
        if (!JsonSerializer.Deserialize<bool>(await Script(form, code))) throw new Exception(message);
    }

    private static async Task Wait(Func<bool> condition)
    {
        for (int attempt = 0; attempt < 100; ++attempt)
        {
            if (condition()) return;
            await Task.Delay(50);
        }
        throw new Exception("The category deletion dialog did not finish.");
    }
}
