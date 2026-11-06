namespace LabPlus;

internal sealed partial class MainForm
{
    private readonly bool _preview=Environment.GetEnvironmentVariable("LABPLUS_PREVIEW")=="1";
    private string _page="Viewer";
    private void BuildMainPage()
    {
        _distance.Minimum=20;_distance.Maximum=200;
        _distanceInput.Minimum=20;_distanceInput.Maximum=200;
        var choices=new UiStack();choices.Controls.AddRange([_outlines,_filled]);Controls.Add(choices);
        Controls.AddRange([_viewerEnabled,_shotCollision,_trimShotCollision,_worldCollision,_triggers,_invisibleWalls,_inspector,_disabledVolumes,_volumeHistory,_distance,_distanceInput,_triggerTypes,_launchButton,_gameStatus,_desruStatus,_gameVersion,_viewerStatus,_pathValue]);
        _triggerTypes.Click+=(_,_)=>{using var tool=new TriggerTypesForm(_settings,_settingsStore.TriggerClassesPath,SaveSettings);tool.ShowDialog(this);};
        _launchButton.Click+=(_,_)=>{if(!_preview)LaunchGame();};
    }
    private void ShowPage(string page)
    {
        _page=page;
        if(page=="Saves") EnsurePracticeSaves();
        _practiceSaves?.SetRuntimePolling(page == "Saves");
    }
    private PracticeSaveForm EnsurePracticeSaves()
    {
        if(_practiceSaves is null)
        { _practiceSaves=new PracticeSaveForm(_settingsStore.SettingsDirectory,gameProcess:SelectedGameProcess){TopLevel=false};Controls.Add(_practiceSaves);_practiceSaves.Show();_practiceSaves.SetRuntimePolling(_page=="Saves"); }
        return _practiceSaves;
    }
    private void OpenResearchTool(string name)
    {
        LabPlusForm screen=name switch
        {
            "Movement lab…"=>_movementLab is {IsDisposed:false}?_movementLab:_movementLab=new MovementLabForm(_settings,_settingsStore.SettingsDirectory,SaveSettings,SelectedGameProcess),
            "Entity links…"=>_entityGraph is {IsDisposed:false}?_entityGraph:_entityGraph=new EntityGraphForm(_settingsStore.SettingsDirectory,SelectedGameProcess),
            "Level state: before / after…"=>_levelStates is {IsDisposed:false}?_levelStates:_levelStates=new LevelStateForm(_settingsStore.SettingsDirectory,SelectedGameProcess),
            "Active encounters…"=>_activeEncounters is {IsDisposed:false}?_activeEncounters:_activeEncounters=new ActiveEncountersForm(_settings,_settingsStore.SettingsDirectory,SaveSettings,SelectedGameProcess),
            "Invisible numbers…"=>_invisibleNumbers is {IsDisposed:false}?_invisibleNumbers:_invisibleNumbers=new InvisibleNumbersForm(_settings,_settingsStore.SettingsDirectory,SaveSettings,SelectedGameProcess),
            "Research points…"=>_researchPoints is {IsDisposed:false}?_researchPoints:_researchPoints=new ResearchPointsForm(_settingsStore.SettingsDirectory,SelectedGameProcess),
            _=>new ResearchForm(_settings,_settingsStore.SettingsDirectory,SaveSettings,SelectedGameProcess)
        };
        if(screen.Visible)screen.Activate();else screen.Show(this);
    }
    private static Font UiFont(float size)=>LabPlusTheme.Ui(size);
    private static void SetStatus(UiText node,string text,bool active,bool warning=false)
    {node.Text=text;node.ForeColor=warning?LabPlusTheme.Gold:active?LabPlusTheme.Text:LabPlusTheme.Muted;}
}
