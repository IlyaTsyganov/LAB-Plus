using LabPlus;
using System.Reflection;

internal static class WorkspaceUiChecks
{
    private static T Field<T>(LabPlusForm form,string name)=>(T)form.GetType().GetField(name,BindingFlags.Instance|BindingFlags.NonPublic)!.GetValue(form)!;
    internal static void Run(LabPlusForm form,string output)
    {
        if(typeof(System.Windows.Forms.Control).IsAssignableFrom(form.GetType()))throw new Exception("Screen state still derives from a native control.");
        var native=typeof(MainForm).Assembly.GetTypes().Where(t=>typeof(System.Windows.Forms.Control).IsAssignableFrom(t)).ToArray();
        if(native.Length!=1||native[0]!=typeof(WorkspaceHost))throw new Exception("A legacy native widget remains in the application.");
        Field<UiToggle>(form,"_viewerEnabled").Checked=true;
        var outlines=Field<UiChoice>(form,"_outlines");var filled=Field<UiChoice>(form,"_filled");
        outlines.Checked=true;if(filled.Checked)throw new Exception("Render choices are not exclusive.");
        var input=Field<UiNumber>(form,"_distanceInput");var range=Field<UiRange>(form,"_distance");input.Value=145;
        if(range.Value!=145)throw new Exception("Precise distance did not update range state.");
        range.Value=90;if(input.Value!=90)throw new Exception("Range did not update precise distance.");
        var settings=new LabPlus.Configuration.SettingsStore().Load();
        if(!settings.CollisionViewerEnabled||settings.ViewDistanceMeters!=90||settings.Style!=LabPlus.Configuration.RenderStyle.Outlines)throw new Exception("Settings state did not persist.");
        Console.WriteLine("PASS: no native widget classes; headless main settings, exclusive render choices and distance persistence.");
    }
}
