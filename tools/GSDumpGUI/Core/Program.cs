using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Specialized = System.Collections.Specialized;
using Reflection = System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;
using GSDumpGUI.Properties;

namespace GSDumpGUI
{
    static class Program
    {
        static public GSDumpGUI frmMain;

        static private Boolean ChangeIcon;

        [STAThread]
        static void Main(String[] args)
        {
            if (args.Length == 4)
            {
                Thread thd = new Thread(new ThreadStart(delegate
                {
                    while (true)
                    {
                        if (ChangeIcon)
                        {
                            IntPtr pt = Process.GetCurrentProcess().MainWindowHandle;
                            if (pt.ToInt64() != 0)
                            {
                                NativeMethods.SetClassLong(pt, -14, Resources.AppIcon.Handle.ToInt64());
                                ChangeIcon = false;
                            }
                        }
                        Int32 tmp = NativeMethods.GetAsyncKeyState(0x1b) & 0xf;
                        if (tmp != 0)
                            Process.GetCurrentProcess().Kill();
                        Thread.Sleep(16);
                    }
                }));
                thd.IsBackground = true;
                thd.Start();

                // Retrieve parameters
                String DLLPath = args[0];
                String DumpPath = args[1];
                String Operation = args[2];
                Int32 Renderer = Convert.ToInt32(args[3]);

                GSDXWrapper wrap = new GSDXWrapper();
                wrap.Load(DLLPath);
                if (Operation == "GSReplay")
                {
                    ChangeIcon = true;
                    if (Renderer != -1)
                        wrap.GSReplayDump(Renderer + " " + DumpPath);
                    else
                        wrap.GSReplayDump(DumpPath);
                }
                else
                    wrap.GSConfig();
                wrap.Unload();
            }
            else
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                frmMain = new GSDumpGUI();
                Application.Run(frmMain);
            }
        }
    }
}
