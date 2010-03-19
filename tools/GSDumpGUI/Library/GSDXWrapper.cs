using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace GSDumpGUI
{
    public class GSDXWrapper
    {
        private delegate void GSReplay(IntPtr HWND, IntPtr HInstance, String File, Boolean Show);
        private delegate void GSConfigure();
        private delegate IntPtr PSEgetLibName();

        private GSReplay gsReplay;
        private GSConfigure gsConfigure;
        private PSEgetLibName PsegetLibName;

        private Boolean Loaded;

        private String DLL;
        private IntPtr DLLAddr;

        static public Boolean IsValidGSDX(String DLL)
        {
            NativeMethods.SetErrorMode(0x8007);
            Boolean Ris = true;
            IntPtr hmod = NativeMethods.LoadLibrary(DLL);
            if (hmod.ToInt64() > 0)
            {
                IntPtr funcaddrReplay = NativeMethods.GetProcAddress(hmod, "GSReplay");
                IntPtr funcaddrLibName = NativeMethods.GetProcAddress(hmod, "PS2EgetLibName");
                IntPtr funcaddrConfig = NativeMethods.GetProcAddress(hmod, "GSconfigure");

                NativeMethods.FreeLibrary(hmod);
                if (!((funcaddrConfig.ToInt64() > 0) && (funcaddrLibName.ToInt64() > 0) && (funcaddrReplay.ToInt64() > 0)))
                {
                    Int32 id = NativeMethods.GetLastError();
                    System.IO.File.AppendAllText(AppDomain.CurrentDomain.BaseDirectory + "test.txt", DLL + " failed to load. Error " + id);
                    Ris = false;
                }
            }
            else
            {
                Int32 id = NativeMethods.GetLastError();
                System.IO.File.AppendAllText(AppDomain.CurrentDomain.BaseDirectory + "test.txt", DLL + " failed to load. Error " + id + Environment.NewLine);
                Ris = false;
            }
            NativeMethods.SetErrorMode(0x0000);
            return Ris;
        }

        public void Load(String DLL)
        {
            this.DLL = DLL;
            NativeMethods.SetErrorMode(0x8007);
            if (!IsValidGSDX(DLL))
                throw new Exception("Invalid GSDX DLL");

            if (Loaded)
                Unload();

            Loaded = true;
            IntPtr hmod = NativeMethods.LoadLibrary(DLL);
            if (hmod.ToInt64() > 0)
            {
                IntPtr funcaddrReplay = NativeMethods.GetProcAddress(hmod, "GSReplay");
                IntPtr funcaddrLibName = NativeMethods.GetProcAddress(hmod, "PS2EgetLibName");
                IntPtr funcaddrConfig = NativeMethods.GetProcAddress(hmod, "GSconfigure");

                gsReplay = (GSReplay)Marshal.GetDelegateForFunctionPointer(funcaddrReplay, typeof(GSReplay));
                gsConfigure = (GSConfigure)Marshal.GetDelegateForFunctionPointer(funcaddrConfig, typeof(GSConfigure));
                PsegetLibName = (PSEgetLibName)Marshal.GetDelegateForFunctionPointer(funcaddrLibName, typeof(PSEgetLibName));
                DLLAddr = hmod;
            }
            NativeMethods.SetErrorMode(0x0000);
        }

        public void Unload()
        {
            NativeMethods.FreeLibrary(DLLAddr);
            Loaded = false;
        }

        public void GSConfig()
        {
            if (!Loaded)
                throw new Exception("GSDX is not loaded");
            gsConfigure.Invoke();
        }

        public void GSReplayDump(String DumpFilename)
        {
            if (!Loaded)
                throw new Exception("GSDX is not loaded");
            gsReplay.Invoke(new IntPtr(0), new IntPtr(0), DumpFilename, false);
        }

        public String PSEGetLibName()
        {
            if (!Loaded)
                throw new Exception("GSDX is not loaded");
            return Marshal.PtrToStringAnsi(PsegetLibName.Invoke());
        }
    }
}
