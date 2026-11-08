using System.Text.Json;
using LabPlus;
using LabPlus.Game;

internal static class SaveTableChecks
{
    private const string Table = "[data-presentation=saves-table]";

    internal static async Task Run(MainForm main, PracticeSaveVault vault, string data)
    {
        var browser = main.Workspace!.Browser;
        async Task<T> Query<T>(string js) => JsonSerializer.Deserialize<T>(await browser.ExecuteScriptAsync(js))!;
        async Task Settle() { main.Workspace!.Publish(true); await Task.Delay(180); }
        async Task<bool> CheckLayout() => await Query<bool>("""
            (()=>{const table=document.querySelector('[data-presentation=saves-table]');
                  const actions=document.querySelector('[data-presentation=saves-actions]');
                  const note=document.querySelector('[data-presentation=saves-limitations]');
                  return actions.getBoundingClientRect().bottom<=table.getBoundingClientRect().top
                    &&note.getBoundingClientRect().top>=table.getBoundingClientRect().bottom
                    &&/ammo, health, and armor.*last checkpoint.*start or reset.*ammo may not restore correctly/i.test(note.textContent);})()
            """);
        if (!await CheckLayout()) throw new Exception("Save actions must be above the list and the English checkpoint note must be below it.");

        if (!await Query<bool>($$"""
            (()=>{const headers=[...document.querySelectorAll('{{Table}} .table-sort')];
            return headers.length===3&&headers.every(h=>getComputedStyle(h,'::before').maskImage.startsWith('url('));})()
            """))
            throw new Exception("All sortable headers must show the funnel icon.");

        if (!await Query<bool>("""
            (()=>{const list=document.querySelector('[data-presentation=saves-table] .control-grid-table');
                  const page=list.closest('.lr-workspace');
                  page.scrollTop=list.scrollTop=0;
                  list.dispatchEvent(new WheelEvent('wheel',{deltaY:90,bubbles:true,cancelable:true}));
                  const pageTop=page.scrollTop, pageScrolled=pageTop>0&&list.scrollTop===0;
                  list.dispatchEvent(new WheelEvent('wheel',{deltaY:40,shiftKey:true,bubbles:true,cancelable:true}));
                  const listScrolled=list.scrollTop>0&&page.scrollTop===pageTop;
                  page.scrollTop=list.scrollTop=0;
                  return pageScrolled&&listScrolled;})()
            """))
            throw new Exception("The wheel over Saves must scroll the page first, with Shift reserved for the list.");

        var grip = await Query<double[]>("""
            (()=>{const g=document.querySelector('.control-table-grip');g.scrollIntoView({block:'center'});
                  const r=g.getBoundingClientRect();return [r.left+r.width/2,r.top+r.height/2,Number(g.getAttribute('aria-valuenow')),innerHeight];})()
            """);
        async Task Mouse(string type, double y, string button, int buttons) =>
            await browser.CoreWebView2.CallDevToolsProtocolMethodAsync("Input.dispatchMouseEvent",
                JsonSerializer.Serialize(new { type, x = grip[0], y, button, buttons, clickCount = type == "mouseMoved" ? 0 : 1 }));
        var point = JsonSerializer.Serialize(grip[..2]);
        if (grip[1] + 72 >= grip[3] || !await Query<bool>($"document.elementFromPoint(...{point})?.classList.contains('control-table-grip')"))
            throw new Exception($"The resize gesture is outside the test viewport. [x, y, height, viewport] = {JsonSerializer.Serialize(grip)}");
        await Mouse("mouseMoved", grip[1], "none", 0);
        await Mouse("mousePressed", grip[1], "left", 1);
        await Mouse("mouseMoved", grip[1] + 72, "none", 1);
        await Mouse("mouseReleased", grip[1] + 72, "left", 0);
        await Settle();
        double height = await Query<double>($"document.querySelector('{Table} .control-grid-table').getBoundingClientRect().height");
        if (Math.Abs(height - grip[2] - 72) > 1)
            throw new Exception($"Dragging the bottom edge did not enlarge the checkpoint list to {grip[2] + 72}px. Actual height: {height}px.");
        await browser.ExecuteScriptAsync("""
            (()=>{const grip=document.querySelector('.control-table-grip');
                  for(let i=0;i<3;i++)grip.dispatchEvent(new KeyboardEvent('keydown',{key:'ArrowUp',bubbles:true}));})()
            """);
        if (await Query<int>("Number(document.querySelector('.control-table-grip').getAttribute('aria-valuenow'))") != grip[2])
            throw new Exception("The resize grip did not support keyboard resizing.");

        async Task<string[]> Names() => await Query<string[]>($"[...document.querySelectorAll('{Table} tbody tr[data-row]')].map(r=>r.cells[1].firstChild.textContent)");
        string[] original = await Names();
        var saves = vault.List().ToDictionary(s => s.Name, StringComparer.Ordinal);
        if (original.Length < 3) throw new Exception("The sort fixture needs at least three archives.");
        await browser.ExecuteScriptAsync($"document.querySelector('{Table} tbody input').click()");
        await Settle();
        foreach (var (column, ascending, descending) in new[]
        {
            (0, original.OrderBy(name => name, StringComparer.CurrentCultureIgnoreCase), original.OrderByDescending(name => name, StringComparer.CurrentCultureIgnoreCase)),
            (1, original.OrderBy(name => PracticeSaveSource.Campaign(saves[name].SourceSlot), StringComparer.CurrentCultureIgnoreCase),
                original.OrderByDescending(name => PracticeSaveSource.Campaign(saves[name].SourceSlot), StringComparer.CurrentCultureIgnoreCase)),
            (2, original.OrderBy(name => saves[name].Captured), original.OrderByDescending(name => saves[name].Captured))
        })
        {
            string heading = $"{Table} [data-sort-column=\"{column}\"]";
            async Task Click() { await browser.ExecuteScriptAsync($"document.querySelector('{heading}').click()"); await Settle(); }
            await Click();
            if (!(await Names()).SequenceEqual(ascending) ||
                await Query<string>($"document.querySelector('{heading}').parentElement.getAttribute('aria-sort')") != "ascending")
                throw new Exception($"Checkpoint column {column} did not sort ascending.");
            await Click();
            if (!(await Names()).SequenceEqual(descending) ||
                await Query<string>($"document.querySelector('{heading}').parentElement.getAttribute('aria-sort')") != "descending")
                throw new Exception($"Checkpoint column {column} did not sort descending.");
            if (await Query<string>($"document.querySelector('{Table} tbody tr.selected').cells[1].firstChild.textContent") != original[0])
                throw new Exception("Sorting changed the selected checkpoint.");
            if (column == 2)
            {
                await browser.ExecuteScriptAsync("document.querySelector('.control-table-grip').dispatchEvent(new KeyboardEvent('keydown',{key:'ArrowDown',bubbles:true}))");
                await Settle();
                using var reopened = new PracticeSaveForm(data, vault: vault);
                try
                {
                    var saved = reopened.Controls[0].Controls.OfType<UiTable>().Single();
                    if (saved.ResizeHeight != grip[2] + 24 || saved.SortColumn != 2 || saved.SortDirection != SortOrder.Descending
                        || !saved.Rows.Select(r => (string?)r.Cells[0].Value).SequenceEqual(descending))
                        throw new Exception("Reopening Saves did not restore its height, sort direction, and row order.");
                }
                finally { reopened.Close(); }
                await browser.ExecuteScriptAsync("document.querySelector('.control-table-grip').dispatchEvent(new KeyboardEvent('keydown',{key:'ArrowUp',bubbles:true}))");
                await Settle();
            }
            await Click();
            if (!(await Names()).SequenceEqual(original) || await Query<bool>($"!!document.querySelector('{Table} th[aria-sort]')"))
                throw new Exception("Clearing the sort did not restore the manual order.");
        }
        await browser.ExecuteScriptAsync($"document.querySelector('{Table} tbody input:checked').click()");
        await Settle();
        Console.WriteLine("PASS: checkpoint list drag and keyboard resize, wheel routing, funnel icons, persisted height and sorting, retained selection and restored manual order.");
    }
}
