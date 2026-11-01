using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.Runtime.InteropServices;

namespace LabPlus;

internal static class LabPlusTheme
{
    internal static readonly Color Canvas = Color.FromArgb(25, 22, 21);
    internal static readonly Color Panel = Color.FromArgb(36, 30, 28);
    internal static readonly Color Raised = Color.FromArgb(52, 40, 37);
    internal static readonly Color Line = Color.FromArgb(75, 57, 50);
    internal static readonly Color Text = Color.FromArgb(241, 230, 218);
    internal static readonly Color Muted = Color.FromArgb(192, 173, 160);
    internal static readonly Color Accent = Color.FromArgb(217, 120, 98);
    internal static readonly Color Gold = Color.FromArgb(215, 180, 114);
    internal static readonly Color Ink = Color.FromArgb(35, 20, 16);
    private static readonly PrivateFontCollection Fonts = new();
    private static FontFamily? _ui, _battle;
    [DllImport("gdi32.dll", CharSet = CharSet.Unicode)]
    private static extern int AddFontResourceEx(string path, uint flags, IntPtr reserved);

    internal static void Initialize()
    {
        if (_ui is not null) return;
        foreach (string name in new[] { "EternalUi2Regular.ttf", "EternalUi2Bold.ttf", "EternalBattleBold.ttf" })
        {
            string path = Path.Combine(AppContext.BaseDirectory, "Assets", "fonts", name);
            if (!File.Exists(path)) throw new FileNotFoundException("A bundled LAB Plus font is missing. Keep Assets/fonts beside the application.", path);
            Fonts.AddFontFile(path);
            AddFontResourceEx(path, 0x10, IntPtr.Zero); // Process-private GDI registration, also used by native WinForms controls.
        }
        _ui = Fonts.Families.Single(f => f.Name.Equals("Eternal UI 2", StringComparison.OrdinalIgnoreCase));
        _battle = Fonts.Families.Single(f => f.Name.Equals("Eternal Battle", StringComparison.OrdinalIgnoreCase));
    }
    internal static Font Ui(float size = 10f, FontStyle style = FontStyle.Regular)
    { Initialize(); return new Font(_ui!, size, _ui!.IsStyleAvailable(style) ? style : FontStyle.Regular, GraphicsUnit.Point); }
    internal static Font Battle(float size) { Initialize(); return new Font(_battle!, size, FontStyle.Bold, GraphicsUnit.Point); }
    private static readonly Lazy<Bitmap> LogoPixels = new(CreateLogoPixels);
    internal static Image Emblem() => (Bitmap)LogoPixels.Value.Clone();
    private static unsafe Bitmap CreateLogoPixels()
    {
        using var original=Image.FromFile(Path.Combine(AppContext.BaseDirectory,"Assets","labplus.png"));
        var bitmap=new Bitmap(original);
        var data=bitmap.LockBits(new Rectangle(0,0,bitmap.Width,bitmap.Height),System.Drawing.Imaging.ImageLockMode.ReadWrite,System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        try
        {
            for(int y=0;y<bitmap.Height;y++)
                for(int x=0;x<bitmap.Width;x++)
                {
                    byte* pixel=(byte*)data.Scan0+y*data.Stride+x*4;
                    if(pixel[2]-pixel[1]>64){pixel[2]=127;pixel[1]=0;pixel[0]=4;}
                }
        }
        finally{bitmap.UnlockBits(data);}
        return bitmap;
    }
    // The resource contains the same full-color emblem as the WebView header.
    internal static Icon ApplicationIcon(int size = 48) => new(Path.Combine(AppContext.BaseDirectory,"Assets","labplus.ico"),size,size);
    internal static void Apply(UiNode root)
    {
        if (!root.Font.Name.StartsWith("Eternal", StringComparison.Ordinal)) root.Font = Ui(root.Font.Size, root.Font.Style);
        foreach (var child in root.Controls) Apply(child);
    }
}

internal sealed class WorkspaceButton : UiAction;
internal sealed class WorkspaceSwitch : UiToggle;
internal sealed class WorkspaceRadio : UiChoice;
internal sealed class WorkspaceSlider : UiRange;
