using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;

namespace GSDumpGUI
{
    public delegate void GSgifTransfer(IntPtr data, int size);
    public delegate void GSgifTransfer2(IntPtr data, int size);
    public delegate void GSgifTransfer3(IntPtr data, int size);
    public delegate void GSVSync(byte field);
    public delegate void GSreadFIFO2(IntPtr data, int size);
    public delegate void GSsetGameCRC(int crc, int options);
    public delegate void GSfreeze(int mode, IntPtr data);
    public delegate void GSopen(IntPtr hwnd, String Title, int renderer);
    public delegate void GSclose();
    public delegate void GSshutdown();
    public delegate void GSConfigure();
    public delegate void GSsetBaseMem(IntPtr data);
    public delegate IntPtr PSEgetLibName();
    public delegate void GSinit();

    public class GSDXWrapper
    {
        private GSConfigure gsConfigure;
        private PSEgetLibName PsegetLibName;
        private GSgifTransfer GSgifTransfer;
        private GSgifTransfer2 GSgifTransfer2;
        private GSgifTransfer3 GSgifTransfer3;
        private GSVSync GSVSync;
        private GSreadFIFO2 GSreadFIFO2;
        private GSsetGameCRC GSsetGameCRC;
        private GSfreeze GSfreeze;
        private GSopen GSopen;
        private GSclose GSclose;
        private GSshutdown GSshutdown;
        private GSsetBaseMem GSsetBaseMem;
        private GSinit GSinit;

        private Boolean Loaded;

        private String DLL;
        private IntPtr DLLAddr;

        private Boolean Running;

        static public Boolean IsValidGSDX(String DLL)
        {
            NativeMethods.SetErrorMode(0x8007);
            Boolean Ris = true;

            Directory.SetCurrentDirectory(Path.GetDirectoryName(DLL));
            IntPtr hmod = NativeMethods.LoadLibrary(DLL);
            if (hmod.ToInt64() > 0)
            {
                IntPtr funcaddrLibName = NativeMethods.GetProcAddress(hmod, "PS2EgetLibName");
                IntPtr funcaddrConfig = NativeMethods.GetProcAddress(hmod, "GSconfigure");

                IntPtr funcaddrGIF = NativeMethods.GetProcAddress(hmod, "GSgifTransfer");
                IntPtr funcaddrVSync = NativeMethods.GetProcAddress(hmod, "GSvsync");
                IntPtr funcaddrSetBaseMem = NativeMethods.GetProcAddress(hmod, "GSsetBaseMem");
                IntPtr funcaddrOpen = NativeMethods.GetProcAddress(hmod, "GSopen");
                IntPtr funcaddrSetCRC = NativeMethods.GetProcAddress(hmod, "GSsetGameCRC");
                IntPtr funcaddrClose = NativeMethods.GetProcAddress(hmod, "GSclose");
                IntPtr funcaddrShutdown = NativeMethods.GetProcAddress(hmod, "GSshutdown");
                IntPtr funcaddrFreeze = NativeMethods.GetProcAddress(hmod, "GSfreeze");
                IntPtr funcaddrGSreadFIFO2 = NativeMethods.GetProcAddress(hmod, "GSreadFIFO2");
                IntPtr funcaddrinit = NativeMethods.GetProcAddress(hmod, "GSinit");

                NativeMethods.FreeLibrary(hmod);
                if (!((funcaddrConfig.ToInt64() > 0) && (funcaddrLibName.ToInt64() > 0) && (funcaddrGIF.ToInt64() > 0)))
                {
                    Int32 id = NativeMethods.GetLastError();
                    System.IO.File.AppendAllText(AppDomain.CurrentDomain.BaseDirectory + "log.txt", DLL + " failed to load. Error " + id + Environment.NewLine);
                    Ris = false;
                }
            }
            else
            {
                Int32 id = NativeMethods.GetLastError();
                System.IO.File.AppendAllText(AppDomain.CurrentDomain.BaseDirectory + "log.txt", DLL + " failed to load. Error " + id + Environment.NewLine);
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
            Directory.SetCurrentDirectory(Path.GetDirectoryName(DLL));
            IntPtr hmod = NativeMethods.LoadLibrary(DLL);
            if (hmod.ToInt64() > 0)
            {
                IntPtr funcaddrLibName = NativeMethods.GetProcAddress(hmod, "PS2EgetLibName");
                IntPtr funcaddrConfig = NativeMethods.GetProcAddress(hmod, "GSconfigure");

                IntPtr funcaddrGIF = NativeMethods.GetProcAddress(hmod, "GSgifTransfer");
                IntPtr funcaddrGIF2 = NativeMethods.GetProcAddress(hmod, "GSgifTransfer2");
                IntPtr funcaddrGIF3 = NativeMethods.GetProcAddress(hmod, "GSgifTransfer3");
                IntPtr funcaddrVSync = NativeMethods.GetProcAddress(hmod, "GSvsync");
                IntPtr funcaddrSetBaseMem = NativeMethods.GetProcAddress(hmod, "GSsetBaseMem");
                IntPtr funcaddrOpen = NativeMethods.GetProcAddress(hmod, "GSopen");
                IntPtr funcaddrSetCRC = NativeMethods.GetProcAddress(hmod, "GSsetGameCRC");
                IntPtr funcaddrClose = NativeMethods.GetProcAddress(hmod, "GSclose");
                IntPtr funcaddrShutdown = NativeMethods.GetProcAddress(hmod, "GSshutdown");
                IntPtr funcaddrFreeze = NativeMethods.GetProcAddress(hmod, "GSfreeze");
                IntPtr funcaddrGSreadFIFO2 = NativeMethods.GetProcAddress(hmod, "GSreadFIFO2");
                IntPtr funcaddrinit = NativeMethods.GetProcAddress(hmod, "GSinit");

                gsConfigure = (GSConfigure)Marshal.GetDelegateForFunctionPointer(funcaddrConfig, typeof(GSConfigure));
                PsegetLibName = (PSEgetLibName)Marshal.GetDelegateForFunctionPointer(funcaddrLibName, typeof(PSEgetLibName));

                this.GSgifTransfer = (GSgifTransfer)Marshal.GetDelegateForFunctionPointer(funcaddrGIF, typeof(GSgifTransfer));
                this.GSgifTransfer2 = (GSgifTransfer2)Marshal.GetDelegateForFunctionPointer(funcaddrGIF2, typeof(GSgifTransfer2));
                this.GSgifTransfer3 = (GSgifTransfer3)Marshal.GetDelegateForFunctionPointer(funcaddrGIF3, typeof(GSgifTransfer3));
                this.GSVSync = (GSVSync)Marshal.GetDelegateForFunctionPointer(funcaddrVSync, typeof(GSVSync));
                this.GSsetBaseMem = (GSsetBaseMem)Marshal.GetDelegateForFunctionPointer(funcaddrSetBaseMem, typeof(GSsetBaseMem));
                this.GSopen = (GSopen)Marshal.GetDelegateForFunctionPointer(funcaddrOpen, typeof(GSopen));
                this.GSsetGameCRC = (GSsetGameCRC)Marshal.GetDelegateForFunctionPointer(funcaddrSetCRC, typeof(GSsetGameCRC));
                this.GSclose = (GSclose)Marshal.GetDelegateForFunctionPointer(funcaddrClose, typeof(GSclose));
                this.GSshutdown = (GSshutdown)Marshal.GetDelegateForFunctionPointer(funcaddrShutdown, typeof(GSshutdown));
                this.GSfreeze = (GSfreeze)Marshal.GetDelegateForFunctionPointer(funcaddrFreeze, typeof(GSfreeze));
                this.GSreadFIFO2 = (GSreadFIFO2)Marshal.GetDelegateForFunctionPointer(funcaddrGSreadFIFO2, typeof(GSreadFIFO2));
                this.GSinit = (GSinit)Marshal.GetDelegateForFunctionPointer(funcaddrinit, typeof(GSinit));

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

        public String PSEGetLibName()
        {
            if (!Loaded)
                throw new Exception("GSDX is not loaded");
            return Marshal.PtrToStringAnsi(PsegetLibName.Invoke());
        }

        public unsafe void Run(GSDump dump, int rendererOverride)
        {
            Running = true;
            GSinit();
            fixed (byte* pointer = dump.Registers)
            {
                GSsetBaseMem(new IntPtr(pointer));
                Int32 HWND = 0;
                GSopen(new IntPtr(&HWND), "", rendererOverride);
                GSsetGameCRC(dump.CRC, 0);
                fixed (byte* freeze = dump.StateData)
                {
                    GSfreeze(0, new IntPtr(freeze));
                    GSVSync(1);

                    while (Running)
                    {
                        if (!NativeMethods.IsWindowVisible(new IntPtr(HWND)))
                        {
                            Running = false;
                            break;
                        }
                        foreach (var itm in dump.Data)
                        {
                            Step(itm, pointer);
                        }
                    }

                    GSclose();
                    GSshutdown();
                }
            }
        }

        private unsafe void Step(GSData itm, byte* registers)
        {
            /*"C:\Users\Alessio\Desktop\Plugins\Dll\gsdx-sse4-r3878.dll" "C:\Users\Alessio\Desktop\Plugins\Dumps\gsdx_20101219182059.gs" "GSReplay" 0*/
            switch (itm.id)
            {
                case GSType.Transfer:
                    switch (itm.data[0])
                    {
                        case 0:
                            fixed (byte* gifdata = itm.data)
                            {
                                GSgifTransfer(new IntPtr(gifdata + 1), (itm.data.Length - 1) / 16);
                            }
                            break;
                        case 1:
                            fixed (byte* gifdata = itm.data)
                            {
                                GSgifTransfer2(new IntPtr(gifdata + 1), (itm.data.Length - 1) /16);
                            }
                            break;
                        case 2:
                            fixed (byte* gifdata = itm.data)
                            {
                                GSgifTransfer3(new IntPtr(gifdata + 1), (itm.data.Length - 1) /16);
                            }
                            break;
                        case 3:
                            fixed (byte* gifdata = itm.data)
                            {
                                GSgifTransfer(new IntPtr(gifdata + 1), (itm.data.Length - 1) /16);
                            }
                            break;
                    }
                    break;
                case GSType.VSync:
                    GSVSync(itm.data[0]);
                    break;
                case GSType.ReadFIFO2:
                    fixed (byte* FIFO = itm.data)
                    {
                        byte[] arrnew = new byte[*((int*)FIFO)];
                        fixed (byte* arrn = arrnew)
                        {
                            GSreadFIFO2(new IntPtr(arrn), *((int*)FIFO));
                        }
                    }
                    break;
                case GSType.Registers:
                    Marshal.Copy(itm.data, 0, new IntPtr(registers), 8192);
                    break;
                default:
                    break;
            }            
        }

        public void Stop()
        {
            Running = false;
        }
    }
}
