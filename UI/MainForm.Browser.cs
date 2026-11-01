using System.Text.Json;

namespace LabPlus;

internal sealed partial class MainForm
{
    internal object WorkspaceSnapshot(ScreenStateAdapter projection) => new
    {
        enabled = _viewerEnabled.Checked, world = _worldCollision.Checked, triggers = _triggers.Checked, walls = _invisibleWalls.Checked,
        inspector = _inspector.Checked, disabled = _disabledVolumes.Checked, history = _volumeHistory.Checked,
        style = _outlines.Checked ? "Outlines" : "Filled", distance = (int)_distanceInput.Value,
        gamePath = _settings.GameExecutablePath, gameVersion = _gameVersion.Text.Replace("Version: ", "", StringComparison.Ordinal),
        gameStatus = _gameStatus.Text, viewerStatus = _viewerStatus.Text, desruStatus = _desruStatus.Text,
        canLaunch = _launchButton.Enabled, preview = _preview,
        appearances = new { world = _settings.WorldAppearance, triggers = _settings.TriggerAppearance, walls = _settings.WallAppearance, inspector = _settings.InspectorAppearance },
        lab = _hudStore.Snapshot(), labFeatures = _hudMonitor.Features, labStatus = _hudMonitor.Status,
        saves = _practiceSaves is null ? null : projection.Snapshot(_practiceSaves.Controls.Cast<UiNode>())
    };

    internal void DispatchWorkspace(JsonElement message)
    {
        switch (message.GetProperty("kind").GetString())
        {
            case "window-size":
                SetWorkspaceSize(message.GetProperty("value").GetString());
                break;
            case "navigate":
                string page = message.GetProperty("page").GetString() ?? "Viewer";
                if (page is "Viewer" or "Research" or "Saves" or "LAB") ShowPage(page);
                break;
            case "lab-setting":
                if (!_hudStore.Apply(message.GetProperty("name").GetString() ?? "", message.GetProperty("value"))) throw new ArgumentException("Invalid LAB setting.");
                Hud.Runtime.GameLauncher.WriteLightingControl(_hudStore, _hudMonitor.CanApplyFeatures);
                break;
            case "appearance":
                ApplyAppearance(message);
                break;
            case "setting":
                string name = message.GetProperty("name").GetString() ?? "";
                if (name != "enabled" && !_viewerEnabled.Checked) return;
                if (name == "distance") _distanceInput.Value = Math.Clamp(message.GetProperty("value").GetInt32(), 20, 200);
                else if (name == "style") { if (message.GetProperty("value").GetString() == "Outlines") _outlines.Checked = true; else _filled.Checked = true; }
                else
                {
                    UiToggle? control = name switch { "enabled" => _viewerEnabled, "world" => _worldCollision, "triggers" => _triggers, "walls" => _invisibleWalls, "inspector" => _inspector, "disabled" => _disabledVolumes, "history" => _volumeHistory, _ => null };
                    if (control is not null) control.Checked = message.GetProperty("value").GetBoolean();
                }
                break;
            case "action":
                switch (message.GetProperty("name").GetString())
                {
                    case "launch": if (_launchButton.Enabled) _launchButton.PerformClick(); break;
                    case "choose": BrowseForGame(); break;
                    case "types": _triggerTypes.PerformClick(); break;
                    case "folder": OpenDataDirectory(); break;
                    case "help": ShowHelp(); break;
                    case "desru": OpenLink("https://github.com/bowsr/DESRU/releases/latest"); break;
                    case "wiki": OpenLink("https://github.com/IlyaTsyganov/LAB-Plus/wiki"); break;
                    case "community": OpenLink("https://discord.gg/dtDa9VZ"); break;
                }
                break;
            case "tool":
                string? tool = message.GetProperty("name").GetString();
                if (tool is "Movement lab…" or "Research timeline & path…" or "Entity links…" or "Level state: before / after…" or "Active encounters…" or "Invisible numbers…" or "Research points…") OpenResearchTool(tool);
                break;
        }
    }
}
