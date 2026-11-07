using LabPlus.Configuration;
using LabPlus.Game;
using System.Globalization;

namespace LabPlus;

internal sealed class FinisherVisualizerForm : LabPlusForm
{
    private readonly LabPlusSettings _settings;
    private readonly string _directory;
    private readonly Func<int> _gameProcess;
    private readonly Action _save;
    private readonly UiToggle _syncTarget = new() { Text = "Enable visualizer", AutoSize = true };
    private readonly UiSelect _syncAction = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 150 };
    private readonly UiSelect _syncAnimation = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 275,
        ToolTipText = "Choose Automatic or preview one animation. This does not force the game's choice." };
    private readonly UiText _syncHelp = new() { AutoSize = true };
    private readonly UiText _status = new() { AutoSize = true };
    private readonly UiGroup _bodyLegend = new() { Text = "Glory Kill body colours", Presentation = "finisher-body" };
    private readonly UiText _chainsawNote = new() { Text = FinisherGuide.ChainsawNote, AutoSize = true };
    private readonly System.Windows.Forms.Timer _refresh = new() { Interval = 250 };
    private bool _busy, _syncing;
    private SyncAnimationPreview? _lastSyncPreview, _shownSyncPreview;
    private string _syncCatalogMap = "";
    private int _syncCatalogProcess;
    private sealed record AnimationOption(string Key, string Label) { public override string ToString() => Label; }

    internal FinisherVisualizerForm(LabPlusSettings settings, string directory, Action save, Func<int> gameProcess)
    {
        _settings=settings;_directory=directory;_save=save;_gameProcess=gameProcess;
        Text="LAB Plus — " + FinisherGuide.Title;ClientSize=new(920,800);MinimumSize=new(680,580);ShowInTaskbar=false;
        StartPosition=FormStartPosition.CenterParent;
        var layout=new UiStack { Dock=DockStyle.Fill, Presentation="finisher-layout" };Controls.Add(layout);
        var options=new UiToolbar { WrapContents=true };
        options.Controls.Add(_syncTarget);options.Controls.Add(new UiText { Text="Mode",AutoSize=true });options.Controls.Add(_syncAction);
        options.Controls.Add(new UiText { Text="Animation",AutoSize=true });options.Controls.Add(_syncAnimation);
        layout.Controls.Add(options);layout.Controls.Add(_syncHelp);layout.Controls.Add(_status);
        var reading=new UiGroup { Text="How to read the preview" };reading.Controls.Add(new UiText { Text=FinisherGuide.Summary,AutoSize=true });
        reading.Controls.Add(new UiText { Text="Position zones follow the enemy. Glory Kill body colours follow the animation that wins for each aim area at your current angle, height and distance.",AutoSize=true });
        reading.Controls.Add(new UiText { Text=FinisherGuide.RuneNote,AutoSize=true,Presentation="finisher-rune-note" });
        layout.Controls.Add(reading);
        var legends=new UiGrid { ColumnCount=2,Presentation="finisher-legends" };legends.ColumnStyles.Add(new(SizeType.Percent,50));legends.ColumnStyles.Add(new(SizeType.Percent,50));
        _bodyLegend.Controls.Add(Legend("Blue — torso or a general upper/body animation.","blue"));
        _bodyLegend.Controls.Add(Legend("Orange — a separately selectable head or limb.","orange"));
        _bodyLegend.Controls.Add(Legend("Purple — legs, feet or a lower-area animation.","purple"));
        _bodyLegend.Controls.Add(new UiText { Text="If only upper and lower remain, the body uses blue and purple. A head or arm can change colour when its separate animation stops winning. Unavailable areas stay unpainted.",AutoSize=true });
        var zones=new UiGroup { Text="Position zone colours" };
        zones.Controls.Add(Legend("Grey — reference area; availability is not confirmed.","grey"));
        zones.Controls.Add(Legend("Amber — boundary, competing choice or checks still pending.","amber"));
        zones.Controls.Add(Legend("Red — the selected animation does not fit current conditions.","red"));
        zones.Controls.Add(Legend("Green — the selected choice is currently confirmed and there is room for the player.","green"));
        zones.Controls.Add(new UiText { Text="The target box marks the tracked enemy. Zone colours and body colours have separate meanings. Final game checks still apply, even in green.",AutoSize=true });
        legends.Controls.Add(_bodyLegend,0,0);legends.Controls.Add(zones,1,0);layout.Controls.Add(legends);layout.Controls.Add(_chainsawNote);
        var fullGuide=new UiText { Text=FinisherGuide.Read("en"),AutoSize=true,Visible=false,Presentation="finisher-guide" };
        var guideButton=new UiAction { Text="Read the full guide",AutoSize=true };
        guideButton.Click+=(_,_)=>{fullGuide.Visible=!fullGuide.Visible;guideButton.Text=fullGuide.Visible?"Hide the full guide":"Read the full guide";};
        layout.Controls.Add(guideButton);layout.Controls.Add(fullGuide);
        layout.Controls.Add(new UiText { Text="Available on audited game patches. Debug HUD reports when a patch is unsupported. Special boss finishers may follow different rules.",AutoSize=true });
        _syncTarget.CheckedChanged+=(_,_)=>SaveOptions();_syncAction.Items.AddRange(["Glory Kill","Chainsaw"]);
        _syncAction.SelectedIndexChanged+=(_,_)=>{if(_syncing)return;SaveOptions();UpdateSyncAnimations(null);};
        _syncAnimation.SelectedIndexChanged+=(_,_)=>{
            if(_syncing||_syncAnimation.SelectedItem is not AnimationOption option)return;
            if(_settings.SyncTargetAction==1)_settings.SyncChainsawAnimation=option.Key;else _settings.SyncGloryKillAnimation=option.Key;
            _save();UpdateSyncAnimations(_shownSyncPreview);
        };
        SyncOptions();UpdateSyncAnimations(null);
        _refresh.Tick+=async(_,_)=>await RefreshSnapshot();Shown+=async(_,_)=>{_refresh.Start();await RefreshSnapshot();};
    }
    private static UiText Legend(string text,string colour)=>new() { Text=text,Presentation="finisher-"+colour,AutoSize=true };
    private static bool RandomFront(SyncAnimationPreview? preview,string name)=>preview?.Action==1&&name is "cut_front_01" or "cut_front_02"&&
        preview.Animations.Contains("cut_front_01")&&preview.Animations.Contains("cut_front_02")&&preview.Hints.TryGetValue(name,out var hint)&&hint.Enemy=="Zombie tier 1";
    private void SyncOptions()
    {
        _syncing=true;_syncTarget.Checked=_settings.ShowSyncTarget;_syncAction.SelectedIndex=_settings.SyncTargetAction==1?1:0;_syncAction.Enabled=_settings.ShowSyncTarget;_syncing=false;
        _bodyLegend.Visible=_settings.SyncTargetAction==0;_chainsawNote.Visible=_settings.SyncTargetAction==1;
    }
    private void SaveOptions()
    {
        if(_syncing)return;_settings.ShowSyncTarget=_syncTarget.Checked;_settings.SyncTargetAction=_syncAction.SelectedIndex==1?1:0;
        _syncAction.Enabled=_settings.ShowSyncTarget;_syncAnimation.Enabled=_settings.ShowSyncTarget&&_syncAnimation.Items.Count>1;
        _bodyLegend.Visible=_settings.SyncTargetAction==0;_chainsawNote.Visible=_settings.SyncTargetAction==1;_save();
    }
    private void UpdateSyncAnimations(SyncAnimationPreview? preview)
    {
        _shownSyncPreview = preview?.Action == _settings.SyncTargetAction ? preview : null;
        bool syncing = _syncing; _syncing = true;
        try
        {
            string saved = LabPlusSettings.SyncAnimationKey(_settings.SyncTargetAction == 1 ? _settings.SyncChainsawAnimation : _settings.SyncGloryKillAnimation);
            var available = preview?.Action == _settings.SyncTargetAction ? preview.Animations : Array.Empty<string>();
            string automatic = preview?.Action == _settings.SyncTargetAction && available.Contains(preview.AutomaticAnimation, StringComparer.Ordinal) ? preview.AutomaticAnimation : "";
            var choices = new List<AnimationOption> { new("", automatic.Length > 0 ? "Automatic: " + (RandomFront(preview, automatic) ? "front 01 / 02 (random)" : AnimationLabel(automatic)) : available.Count > 0 ? "Automatic (position and aim)" : "Automatic (aim at an enemy)") };
            choices.AddRange(available.Select(name => new AnimationOption(name, AnimationLabel(name))));
            if (saved.Length > 0 && !available.Contains(saved, StringComparer.Ordinal)) choices.Add(new(saved, AnimationLabel(saved) + " (unavailable on target)"));
            if (!_syncAnimation.Items.Cast<AnimationOption>().SequenceEqual(choices)) { _syncAnimation.Items.Clear(); _syncAnimation.Items.AddRange(choices.Cast<object>().ToArray()); }
            _syncAnimation.SelectedIndex = choices.FindIndex(option => option.Key == saved);
            _syncAnimation.Enabled = _settings.ShowSyncTarget && available.Count > 0;
            _syncHelp.Text = available.Count == 0 ? "Aim at a living enemy to load supported animations."
                : saved.Length > 0 && !available.Contains(saved, StringComparer.Ordinal) ? "This enemy has no " + AnimationLabel(saved) + ". Choose Automatic or another animation."
                : _settings.SyncTargetAction == 1 ? "Position zones only. Body-part colours are disabled for Chainsaw."
                : "Glory Kill colours show the winning aim areas at your current position.";
            string active = saved.Length > 0 ? saved : automatic;
            if (_settings.SyncTargetAction == 0 && preview?.Action == 0 && available.Contains(active, StringComparer.Ordinal) && preview.Hints.TryGetValue(active, out var hint))
                _syncHelp.Text = (hint.Enemy.Length > 0 ? hint.Enemy + ": " : "") + hint.Aim + ". " + _syncHelp.Text;
        }
        finally { _syncing = syncing; }
    }
    private static string AnimationLabel(string name) => name.Replace("kill_", "").Replace("cut_", "").Replace('_', ' ');
    private void UpdateSyncCatalog(InvisibleNumbersSnapshot? snapshot, bool fresh, int process)
    {
        var preview = fresh && snapshot!.Ready ? snapshot.SyncPreview : null;
        bool paused = fresh && snapshot!.Phase == "paused";
        if (preview is not null) { _lastSyncPreview = preview; _syncCatalogMap = snapshot!.Map; _syncCatalogProcess = process; }
        else if (paused && _syncCatalogProcess == process && _syncCatalogMap == snapshot!.Map) preview = _lastSyncPreview;
        else { _lastSyncPreview = null; _syncCatalogMap = ""; _syncCatalogProcess = 0; }
        UpdateSyncAnimations(preview);
        if (paused && preview?.Action == _settings.SyncTargetAction)
            _syncHelp.Text = "Paused: choices from the last aimed enemy. Return to the game to preview.";
    }
    private async Task RefreshSnapshot()
    {
        if(_busy||IsDisposed)return;_busy=true;
        try{
            SyncOptions();int process=_gameProcess();
            var snapshot=await Task.Run(()=>{
                if(process<=0)return null;Directory.CreateDirectory(_directory);
                File.WriteAllText(Path.Combine(_directory,$"invisible-numbers-{process}.request"),DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(CultureInfo.InvariantCulture));
                return InvisibleNumbersSnapshot.Read(_directory,process);
            });
            if(IsDisposed)return;bool fresh=snapshot is not null&&snapshot.Process==_gameProcess()&&snapshot.IsFresh(DateTimeOffset.UtcNow);
            UpdateSyncCatalog(snapshot,fresh,process);
            _status.Text=process<=0?"Launch a supported DOOM Eternal patch through LAB Plus to preview finishers.":!fresh?"Waiting for game data. Aim at a living enemy.":"Aim at an enemy to track it. Looking away keeps a reference until you leave its action range.";
        }catch(Exception ex) when(ex is IOException or UnauthorizedAccessException){if(!IsDisposed){UpdateSyncCatalog(null,false,0);_status.Text="Waiting for game data.";}}
        finally{_busy=false;}
    }
    protected override void Dispose(bool disposing){if(disposing)_refresh.Dispose();base.Dispose(disposing);}
}
