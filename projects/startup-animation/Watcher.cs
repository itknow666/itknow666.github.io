using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Threading;

namespace ITKNOW {
    public sealed class WindowTracker {
        private HashSet<long> previous = new HashSet<long>();
        public List<long> AddedWindows { get; private set; }
        public void Forget(long window) { previous.Remove(window); }
        public bool Observe(IEnumerable<long> windows) {
            var next = new HashSet<long>(windows);
            bool added = false;
            AddedWindows=new List<long>();
            foreach (long id in next) if (!previous.Contains(id)) { added = true;AddedWindows.Add(id); }
            previous = next;
            return added;
        }
    }
    public static class Watcher {
        delegate bool EnumCallback(IntPtr hwnd, IntPtr lparam);
        [DllImport("user32.dll")] static extern bool EnumWindows(EnumCallback callback, IntPtr param);
        [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hwnd);
        [DllImport("user32.dll")] static extern bool IsIconic(IntPtr hwnd);
        delegate void WinEventCallback(IntPtr hook,uint kind,IntPtr window,int objectId,int childId,uint thread,uint time);
        [DllImport("user32.dll")] static extern IntPtr SetWinEventHook(uint min,uint max,IntPtr module,WinEventCallback callback,uint process,uint thread,uint flags);
        [DllImport("user32.dll")] static extern bool UnhookWinEvent(IntPtr hook);
        [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint processId);
        [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr hwnd, StringBuilder buffer, int count);
        [DllImport("user32.dll")] static extern IntPtr GetWindow(IntPtr hwnd, uint command);
        [DllImport("user32.dll")] static extern int GetWindowLong(IntPtr hwnd, int index);
        [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
        static IntPtr SelectTarget(IEnumerable<long> windows) {
            IntPtr foreground=GetForegroundWindow(),best=IntPtr.Zero;long area=0;
            foreach(long id in windows) { var hwnd=new IntPtr(id);WindowBounds.NativeRect rect;if(!WindowBounds.TryGet(hwnd,out rect))continue;
                if(hwnd==foreground)return hwnd;long size=(long)rect.Width*rect.Height;if(size>area){area=size;best=hwnd;}
            }
            return best;
        }
        static string Root = AppDomain.CurrentDomain.BaseDirectory;
        static readonly object LogLock = new object();
        static int Session = Process.GetCurrentProcess().SessionId;
        static readonly Dictionary<int,bool> ProcessMatches = new Dictionary<int,bool>();
        static void Log(string message) {
            try { lock(LogLock) { string path=Path.Combine(Root,"watcher.log"); if(File.Exists(path)&&new FileInfo(path).Length>256000) File.WriteAllText(path,""); File.AppendAllText(path,DateTime.Now.ToString("o")+" "+message+Environment.NewLine,Encoding.UTF8); } } catch { }
        }
        static bool IsCodex(Process p) {
            try { string path=p.MainModule.FileName; return p.SessionId==Session && path.IndexOf(@"\WindowsApps\OpenAI.Codex_",StringComparison.OrdinalIgnoreCase)>=0 && path.EndsWith(@"\app\ChatGPT.exe",StringComparison.OrdinalIgnoreCase); } catch { return false; }
        }
        static HashSet<long> Windows() {
            var ids=new HashSet<uint>();
            var live=new HashSet<int>();
            foreach(Process p in Process.GetProcessesByName("ChatGPT")) { using(p) {
                int id=p.Id;live.Add(id);bool matches;
                if(!ProcessMatches.TryGetValue(id,out matches)) { matches=IsCodex(p);if(matches)ProcessMatches[id]=true; }
                if(matches)ids.Add((uint)id);
            } }
            var expired=new List<int>();foreach(int id in ProcessMatches.Keys)if(!live.Contains(id))expired.Add(id);
            foreach(int id in expired)ProcessMatches.Remove(id);
            var found=new HashSet<long>();
            if(ids.Count==0) return found;
            EnumWindows(delegate(IntPtr hwnd,IntPtr unused) {
                if(!IsWindowVisible(hwnd)||IsIconic(hwnd)||GetWindow(hwnd,4)!=IntPtr.Zero||(GetWindowLong(hwnd,-20)&0x80)!=0) return true;
                uint pid;GetWindowThreadProcessId(hwnd,out pid);if(!ids.Contains(pid))return true;
                var name=new StringBuilder(256);GetClassName(hwnd,name,name.Capacity);
                if(name.ToString()=="Chrome_WidgetWin_1") found.Add(hwnd.ToInt64());
                return true;
            },IntPtr.Zero);
            return found;
        }
        static void Assert(bool condition,string message) { if(!condition)throw new Exception(message); }
        static void SelfTest() {
            var t=new WindowTracker();
            Assert(!t.Observe(new long[0]),"idle triggered");
            Assert(t.Observe(new long[]{10}),"new window missed");
            Assert(!t.Observe(new long[]{10}),"stable window retriggered");
            Assert(!t.Observe(new long[0]),"closing window triggered");
            Assert(t.Observe(new long[]{10}),"reopened window missed");
            Assert(t.Observe(new long[]{11}),"restarted application missed");
            Assert(!t.Observe(new long[]{11,11}),"duplicate window retriggered");
            t.Forget(11);
            Assert(t.Observe(new long[]{11}),"rapid hide-and-show between scans missed");
            Assert(!t.Observe(new long[0]),"minimizing triggered");
            Assert(t.Observe(new long[]{11}),"restoring minimized window missed");
            t.Forget(999);
            Assert(!t.Observe(new long[]{11}),"unrelated window event triggered");
            File.WriteAllText(Path.Combine(Root,"watcher-tests.txt"),"PASS: idle, first open, stable window, close, reopen, restart, duplicate suppression, rapid hide/show, minimize/restore, unrelated-window isolation.");
        }
        [STAThread] public static int Main(string[] args) {
            bool owns=false;
            try {
                if(Array.IndexOf(args,"--self-test")>=0) { SelfTest();return 0; }
                if(Array.IndexOf(args,"--probe")>=0) { var windows=Windows();File.WriteAllText(Path.Combine(Root,"watcher-probe.txt"),"session="+Session+" visible-codex-windows="+windows.Count+Environment.NewLine+string.Join(",",windows));return 0; }
                if(Array.IndexOf(args,"--preview")>=0) { var previewApp=new Application();var preview=new SplashWindow(SelectTarget(Windows()));preview.ContentRendered+=(s,e)=>Log("preview-rendered "+preview.BoundsReport);preview.Closed+=(s,e)=>Log("preview-closed");previewApp.Run(preview);return 0; }
                using(var mutex=new Mutex(true,@"Local\ITKNOW_Codex_StartupWatcher",out owns)) {
                    if(!owns)return 0;
                    var app=new Application(); app.ShutdownMode=ShutdownMode.OnExplicitShutdown;
                    var tracker=new WindowTracker();
                    if(Array.IndexOf(args,"--baseline")>=0)tracker.Observe(Windows());
                    SplashWindow splash=null;
                    var pending=new HashSet<long>();
                    // Events preserve very short hide/restore transitions that polling alone can miss.
                    WinEventCallback eventCallback=delegate(IntPtr hook,uint kind,IntPtr hwnd,int obj,int child,uint thread,uint time) {
                        if(hwnd==IntPtr.Zero||obj!=0||child!=0)return;
                        if(kind==0x8001||kind==0x8003||kind==0x16)tracker.Forget(hwnd.ToInt64());
                    };
                    IntPtr objectHook=SetWinEventHook(0x8001,0x8003,IntPtr.Zero,eventCallback,0,0,2);
                    IntPtr minimizeHook=SetWinEventHook(0x16,0x17,IntPtr.Zero,eventCallback,0,0,2);
                    if(objectHook==IntPtr.Zero||minimizeHook==IntPtr.Zero)Log("event-hook-unavailable polling-remains-active");
                    var timer=new DispatcherTimer();timer.Interval=TimeSpan.FromMilliseconds(300);
                    timer.Tick+=(s,e)=>{
                        try {
                            if(File.Exists(Path.Combine(Root,"watcher.stop"))) { Log("stopped-by-user");timer.Stop();app.Shutdown();return; }
                            var visible=Windows();tracker.Observe(visible);
                            foreach(long hwnd in tracker.AddedWindows)pending.Add(hwnd);
                            pending.RemoveWhere(hwnd=>!visible.Contains(hwnd));
                            if(pending.Count>0&&splash==null) {
                                IntPtr target=SelectTarget(pending);if(target==IntPtr.Zero)return;
                                pending.Remove(target.ToInt64());Log("codex-window-opened hwnd="+target.ToInt64());splash=new SplashWindow(target);
                                splash.ContentRendered+=(ws,we)=>Log("animation-rendered "+splash.BoundsReport);
                                splash.Closed+=(ws,we)=>{Log("animation-closed");splash=null;};
                                splash.Show();
                            }
                        } catch(Exception ex) { Log("tick-error "+ex.Message); }
                    };
                    app.DispatcherUnhandledException+=(s,e)=>{Log("unhandled "+e.Exception.Message);e.Handled=true;app.Shutdown(1);};
                    timer.Start();Log("watcher-started session="+Session+" baseline="+(Array.IndexOf(args,"--baseline")>=0));
                    app.Run();timer.Stop();if(objectHook!=IntPtr.Zero)UnhookWinEvent(objectHook);if(minimizeHook!=IntPtr.Zero)UnhookWinEvent(minimizeHook);GC.KeepAlive(eventCallback);mutex.ReleaseMutex();
                }
                return 0;
            } catch(Exception ex) { Log("fatal "+ex.ToString());return 1; }
        }
    }
}
