using System;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace ITKNOW {
    public sealed class SplashSurface : FrameworkElement {
        public double Seconds { get; set; }
        static readonly Typeface Mono = new Typeface("Consolas");
        static readonly Typeface Display = new Typeface(new FontFamily("Segoe UI"), FontStyles.Normal, FontWeights.Bold, FontStretches.Normal);
        static SolidColorBrush Ink(string hex, double opacity = 1) {
            var b = new SolidColorBrush((Color)ColorConverter.ConvertFromString(hex)); b.Opacity = Math.Max(0, Math.Min(1, opacity)); return b;
        }
        static void Line(DrawingContext d, Point a, Point b, string color, double opacity = 1, double width = 1) {
            d.DrawLine(new Pen(Ink(color, opacity), width), a, b);
        }
        static void Text(DrawingContext d, string value, double x, double y, double size, string color, bool center = false, bool bold = false, double opacity = 1) {
            var text = new FormattedText(value, CultureInfo.InvariantCulture, FlowDirection.LeftToRight, bold ? Display : Mono, size, Ink(color, opacity), 1.0);
            d.DrawText(text, new Point(center ? x - text.Width / 2 : x, y));
        }
        static Point Polar(double r, double a) { return new Point(Math.Cos(a)*r, Math.Sin(a)*r); }
        static void Arc(DrawingContext d, double r, double start, double length, string color, double opacity, double width = 1) {
            var g = new StreamGeometry();
            using(var c = g.Open()) { c.BeginFigure(Polar(r,start),false,false); c.ArcTo(Polar(r,start+length),new Size(r,r),0,length>Math.PI,SweepDirection.Clockwise,true,false); }
            d.DrawGeometry(null,new Pen(Ink(color,opacity),width),g);
        }
        protected override void OnRender(DrawingContext d) {
            base.OnRender(d);
            double scale = Math.Min(ActualWidth/1000, ActualHeight/640), t = Seconds;
            if(scale<=0)return;
            double width=ActualWidth/scale,height=ActualHeight/scale,cx=width/2,cy=height*.45;
            d.PushTransform(new ScaleTransform(scale,scale));
            d.DrawRectangle(Ink("#030A12"),null,new Rect(0,0,width,height));
            var glow = new RadialGradientBrush();
            glow.GradientStops.Add(new GradientStop(Color.FromArgb(95,16,79,99),0));
            glow.GradientStops.Add(new GradientStop(Color.FromArgb(0,3,10,18),1));
            d.DrawEllipse(glow,null,new Point(cx,cy),440,340);
            var random = new Random(1807);
            for(int i=0;i<150;i++) { double x=random.NextDouble()*width, y=random.NextDouble()*height, a=random.NextDouble(); d.DrawEllipse(Ink("#A1D4EC",.1+a*.45),null,new Point(x,y),.4+a*.4,.4+a*.4); }
            for(int x=20;x<width;x+=60) Line(d,new Point(x,0),new Point(x,height),"#72C0DE",.035);
            for(int y=20;y<height;y+=60) Line(d,new Point(0,y),new Point(width,y),"#72C0DE",.035);
            d.DrawRectangle(null,new Pen(Ink("#9CDDEB",.12),1),new Rect(24,24,width-48,height-48));
            Line(d,new Point(24,48),new Point(24,24),"#A1EBF4",.7); Line(d,new Point(24,24),new Point(48,24),"#A1EBF4",.7);
            Line(d,new Point(width-48,height-24),new Point(width-24,height-24),"#A1EBF4",.7); Line(d,new Point(width-24,height-24),new Point(width-24,height-48),"#A1EBF4",.7);
            Text(d,"CODEX  /  ITKNOW",48,44,12,"#C8F4FC");
            d.DrawEllipse(Ink("#FFAF78"),null,new Point(width-246,54),2.3,2.3);
            Text(d,"INTELLIGENCE INTERFACE",width-231,47,9,"#799FAB");
            d.PushTransform(new TranslateTransform(cx,cy));
            d.PushOpacity(Math.Min(1,t/.8));
            d.DrawEllipse(null,new Pen(Ink("#80DBEF",.14),1),new Point(0,0),224,224);
            for(int i=0;i<120;i++) { double a=i*Math.PI/60; Line(d,Polar(211,a),Polar(i%5==0?218:214,a),"#82E8FA",i%5==0?.5:.2); }
            for(int j=0;j<3;j++) { double a=t*(j%2==0?.18:-.24)+j*2.1; Arc(d,197-j*13,a,1.5,"#70EAF7",j==0?.7:.3,j==0?1.5:1); Arc(d,197-j*13,a+2.2,.7,"#70EAF7",.18); }
            Arc(d,224,-t*.23,.22,"#FFAE78",1,2);
            d.DrawEllipse(Ink("#FFBE8E"),null,Polar(224,-t*.23),2.5,2.5);
            double fade=t<3.4?1:Math.Max(.14,1-(t-3.4)*.65);
            var nodes = new Point[60]; var depth = new double[60];
            for(int i=0;i<60;i++) { double y=1-2*(i+.5)/60, r=Math.Sqrt(1-y*y), a=i*2.39996+t*.35; nodes[i]=new Point(Math.Cos(a)*r*150,y*150); depth[i]=Math.Sin(a)*r; }
            for(int i=0;i<60;i++) {
                for(int j=i+1;j<60;j++) { double dx=nodes[i].X-nodes[j].X,dy=nodes[i].Y-nodes[j].Y,dz=(depth[i]-depth[j])*150; if(Math.Sqrt(dx*dx+dy*dy+dz*dz)<66) Line(d,nodes[i],nodes[j],"#78EAF9",(.12+(depth[i]+1)*.07)*fade); }
                double r=1+(depth[i]+1)*.55; d.DrawEllipse(Ink("#A2F6FF",(.25+(depth[i]+1)*.3)*fade),null,nodes[i],r,r);
            }
            if(t<4.3) {
                double pulse=(t%1.5)/1.5; d.DrawEllipse(null,new Pen(Ink("#91F1FD",(1-pulse)*.4),1),new Point(0,0),35+pulse*160,35+pulse*160);
                Text(d,"</>",0,-24,32,"#D0FAFF",true,false,Math.Min(1,Math.Max(0,4.3-t)));
            }
            if(t>3.7&&t<5.2) { double f=(t-3.7)/1.5; d.DrawEllipse(null,new Pen(Ink("#BFFAFF",(1-f)*.6),1.5),new Point(0,0),80+f*270,80+f*270); }
            d.Pop();d.Pop();
            double reveal = Math.Min(1,Math.Max(0,(t-3.9)/1.2));
            Text(d,"THE NEXT THOUGHT STARTS HERE",cx,cy-59,10,"#72E9F2",true,false,reveal);
            Text(d,"ITKNOW",cx,cy-46,98,"#D6FAFF",true,true,reveal);
            Text(d,"与你一起，让想法成为现实",cx,cy+75,13,"#A8CDD9",true,false,reveal);
            Text(d,"NEURAL ENGINE",48,cy-28,9,"#5A8494");
            Text(d,t<3?"SYNCING":"ONLINE",48,cy-10,11,"#AFD7E3");
            Text(d,"CORE FREQUENCY",48,cy+34,9,"#5A8494");
            Text(d,(Math.Min(1,t/7.7)*4.8).ToString("0.00",CultureInfo.InvariantCulture)+" THz",48,cy+52,11,"#AFD7E3");
            if(t>3.5&&t<4.4) { double y=(t-3.5)/.9*height; Line(d,new Point(25,y),new Point(width-25,y),"#9EF6FF",.45); }
            double progress = Math.Min(1,t/7.7);
            string phase=t<2?"INITIALIZING CORE":t<3.9?"MAPPING NEURAL LINKS":t<7?"ASSEMBLING IDENTITY":"ALL SYSTEMS ONLINE";
            Text(d,phase,cx-170,height-134,9,"#88B4C3");
            Text(d,((int)(progress*100)).ToString("000")+"%",cx+136,height-136,11,"#B9F6FF");
            d.DrawRectangle(Ink("#183541"),null,new Rect(cx-170,height-112,340,2));
            d.DrawRectangle(Ink("#77EAF2"),null,new Rect(cx-170,height-112,340*progress,2));
            Text(d,t<7?"AWAKENING CREATIVE INTELLIGENCE":"READY TO CREATE",cx,height-96,9,"#608B9B",true);
            Text(d,"CODEX OS / ITKNOW EDITION",48,height-58,9,"#6C909F");
            Text(d,"ESC  跳过动画",width-163,height-60,10,"#7BA9B9");
            d.Pop();
        }
    }
    public static class WindowBounds {
        [StructLayout(LayoutKind.Sequential)] public struct NativeRect {
            public int Left,Top,Right,Bottom;
            public int Width { get { return Right-Left; } }
            public int Height { get { return Bottom-Top; } }
            public override string ToString() { return Left+","+Top+","+Width+","+Height; }
        }
        [DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(IntPtr hwnd,int attribute,out NativeRect rect,int size);
        [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr hwnd,out NativeRect rect);
        [DllImport("user32.dll")] static extern bool IsWindow(IntPtr hwnd);
        [DllImport("user32.dll")] static extern bool IsIconic(IntPtr hwnd);
        [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr hwnd,IntPtr after,int x,int y,int width,int height,uint flags);
        public static bool TryGet(IntPtr hwnd,out NativeRect rect) {
            rect=new NativeRect();if(hwnd==IntPtr.Zero||!IsWindow(hwnd)||IsIconic(hwnd))return false;
            if(DwmGetWindowAttribute(hwnd,9,out rect,Marshal.SizeOf(typeof(NativeRect)))!=0&&!GetWindowRect(hwnd,out rect))return false;
            return rect.Width>0&&rect.Height>0;
        }
        public static void Match(IntPtr overlay,NativeRect target) {
            // Extended frame bounds are physical pixels; a per-monitor-aware process uses the same units.
            SetWindowPos(overlay,IntPtr.Zero,target.Left,target.Top,target.Width,target.Height,0x14);
        }
    }
    public sealed class SplashWindow : Window {
        readonly SplashSurface surface = new SplashSurface();
        readonly Stopwatch watch = new Stopwatch();
        readonly DispatcherTimer timer = new DispatcherTimer();
        readonly IntPtr target;
        IntPtr nativeHandle;
        string lastBounds;
        int tick;
        public string BoundsReport {
            get { WindowBounds.NativeRect original,overlay;return WindowBounds.TryGet(target,out original)&&WindowBounds.TryGet(nativeHandle,out overlay)?"target="+original+" overlay="+overlay:"bounds-unavailable"; }
        }
        void FollowTarget() {
            if(target==IntPtr.Zero||nativeHandle==IntPtr.Zero)return;
            WindowBounds.NativeRect bounds;
            if(!WindowBounds.TryGet(target,out bounds)) { if(IsLoaded)Close();return; }
            string key=bounds.ToString();if(key==lastBounds)return;
            WindowBounds.Match(nativeHandle,bounds);lastBounds=key;
        }
        public SplashWindow() : this(IntPtr.Zero) { }
        public SplashWindow(IntPtr targetWindow) {
            target=targetWindow;
            Title="ITKNOW · Codex 启动"; WindowStyle=WindowStyle.None; ResizeMode=ResizeMode.NoResize;
            Width=Math.Min(1000,SystemParameters.WorkArea.Width*.9); Height=Width*.64;
            if(Height>SystemParameters.WorkArea.Height*.9) { Height=SystemParameters.WorkArea.Height*.9; Width=Height/.64; }
            WindowStartupLocation=target==IntPtr.Zero?WindowStartupLocation.CenterScreen:WindowStartupLocation.Manual; Background=Brushes.Black; Topmost=true; ShowInTaskbar=false; Content=surface;
            SourceInitialized+=(s,e)=>{nativeHandle=new WindowInteropHelper(this).Handle;FollowTarget();};
            KeyDown+=(s,e)=>{if(e.Key==Key.Escape) Close();};
            MouseLeftButtonDown+=(s,e)=>{if(target==IntPtr.Zero)DragMove();};
            timer.Interval=TimeSpan.FromMilliseconds(16);
            timer.Tick+=(s,e)=>{if(++tick%6==0)FollowTarget();surface.Seconds=watch.Elapsed.TotalSeconds;surface.InvalidateVisual();if(surface.Seconds>=8.2)Close();};
            Loaded+=(s,e)=>{lastBounds=null;FollowTarget();watch.Start();timer.Start();Activate();Focus();};
            Closed+=(s,e)=>{timer.Stop();watch.Stop();};
        }
    }
}
