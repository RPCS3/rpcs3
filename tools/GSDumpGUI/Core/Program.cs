using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Specialized = System.Collections.Specialized;
using Reflection = System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;

namespace GSDumpGUI
{
    static class Program
    {
        static public GSDumpGUI frmMain;

        [STAThread]
        static void Main(String[] args)
        {
            if (args.Length == 3)
            {
                Thread thd = new Thread(new ThreadStart(delegate
                {
                    while (true)
                    {
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

                // Try to load the DLL in memory
                IntPtr hmod = NativeMethods.LoadLibrary(DLLPath);
                if (hmod.ToInt64() > 0)
                {
                    // Search if the DLL has the requested operation
                    IntPtr funcaddr = NativeMethods.GetProcAddress(hmod, Operation);
                    if (funcaddr.ToInt64() > 0)
                    {
                        // Execute the appropriate function pointer by casting it to a delegate.
                        if (Operation == "GSReplay")
                        {
                            GSDXImport.GSReplay dg = (GSDXImport.GSReplay)Marshal.GetDelegateForFunctionPointer(funcaddr, typeof(GSDXImport.GSReplay));
                            dg.Invoke(new IntPtr(0), new IntPtr(0), DumpPath, false);
                        }
                        else
                        {
                            GSDXImport.GSConfigure dg = (GSDXImport.GSConfigure)Marshal.GetDelegateForFunctionPointer(funcaddr, typeof(GSDXImport.GSConfigure));
                            dg.Invoke();
                        }
                    }
                    // Unload the library.
                    NativeMethods.FreeLibrary(hmod);
                }
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
