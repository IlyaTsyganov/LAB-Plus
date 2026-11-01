using System.Reflection;
using System.Runtime.InteropServices;
using System.Drawing;

internal static class Program
{
    [DllImport("shell32.dll", CharSet=CharSet.Unicode)] private static extern uint ExtractIconEx(string file,int index,IntPtr[] large,IntPtr[] small,uint count);
    [DllImport("user32.dll")] private static extern bool DestroyIcon(IntPtr icon);
    private static int Main(string[] args)
    {
        try { return Run(args); }
        catch(Exception error) { Console.Error.WriteLine(error.Message);return 1; }
    }
    private static int Run(string[] args)
    {
        if(args.Length==2 && args[0]=="--verify-exe")
        {
            var large=new IntPtr[1];var small=new IntPtr[1];
            uint count=ExtractIconEx(Path.GetFullPath(args[1]),0,large,small,1);
            if(count==0 || large[0]==IntPtr.Zero)throw new Exception($"The executable icon could not be extracted (count={count}).");
            try
            {
                using var icon=Icon.FromHandle(large[0]);using var pixels=icon.ToBitmap();int red=0,gold=0,warmRed=0;
                for(int y=0;y<pixels.Height;y++)for(int x=0;x<pixels.Width;x++)
                {
                    var c=pixels.GetPixel(x,y);if(c.A<200)continue;
                    if(c.R-c.G>64){if(c.G<3&&c.B<7)red++;else if(c.R>190&&c.G>20)warmRed++;}
                    else if(c.R>190&&c.G>140&&c.B>60&&c.B<160&&c.G>c.B+30)gold++;
                }
                if(red==0||gold==0||warmRed>0)throw new Exception($"Wrong embedded palette: darkRed={red} gold={gold} warmRed={warmRed}.");
                Console.WriteLine($"PASS: the actual EXE icon contains LAB red and original gold; darkRed={red}, gold={gold}.");return 0;
            }
            finally{if(large[0]!=IntPtr.Zero)DestroyIcon(large[0]);if(small[0]!=IntPtr.Zero)DestroyIcon(small[0]);}
        }
        if(args.Length!=2)throw new ArgumentException("IconResource <compiled-labplus.dll> <output.ico> or --verify-exe <exe>");
        var assembly=Assembly.LoadFrom(Path.GetFullPath(args[0]));
        var method=assembly.GetType("LabPlus.LabPlusTheme",true)!.GetMethod("Emblem",BindingFlags.Static|BindingFlags.NonPublic)!;
        using var emblem=(Image)method.Invoke(null,null)!;
        var frames=new List<(int Size,byte[] Png)>();
        foreach(int size in new[]{16,24,32,48,64,128,256})
        {
            using var bitmap=new Bitmap(size,size,System.Drawing.Imaging.PixelFormat.Format32bppArgb);
            using(var graphics=Graphics.FromImage(bitmap))
            {
                graphics.CompositingMode=System.Drawing.Drawing2D.CompositingMode.SourceCopy;
                graphics.InterpolationMode=System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                graphics.PixelOffsetMode=System.Drawing.Drawing2D.PixelOffsetMode.HighQuality;
                graphics.DrawImage(emblem,new Rectangle(0,0,size,size));
            }
            using var data=new MemoryStream();bitmap.Save(data,System.Drawing.Imaging.ImageFormat.Png);frames.Add((size,data.ToArray()));
        }
        using var output=new MemoryStream();using var writer=new BinaryWriter(output);
        writer.Write((ushort)0);writer.Write((ushort)1);writer.Write((ushort)frames.Count);
        int offset=6+frames.Count*16;
        foreach(var frame in frames)
        {
            writer.Write((byte)(frame.Size==256?0:frame.Size));writer.Write((byte)(frame.Size==256?0:frame.Size));
            writer.Write((byte)0);writer.Write((byte)0);writer.Write((ushort)1);writer.Write((ushort)32);
            writer.Write(frame.Png.Length);writer.Write(offset);offset+=frame.Png.Length;
        }
        foreach(var frame in frames)writer.Write(frame.Png);
        File.WriteAllBytes(Path.GetFullPath(args[1]),output.ToArray());
        Console.WriteLine("Wrote seven ICO frames from the application's existing icon palette.");return 0;
    }
}
