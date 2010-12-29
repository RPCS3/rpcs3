using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;
using TCPLibrary.MessageBased.Core;
using System.Threading;

namespace GSDumpGUI
{
    public delegate void GSgifTransfer(IntPtr data, int size);
    public delegate void GSgifTransfer1(IntPtr data, int size);
    public delegate void GSgifTransfer2(IntPtr data, int size);
    public delegate void GSgifTransfer3(IntPtr data, int size);
    public delegate void GSVSync(byte field);
    public delegate void GSreset();
    public delegate void GSreadFIFO2(IntPtr data, int size);
    public delegate void GSsetGameCRC(int crc, int options);
    public delegate int GSfreeze(int mode, IntPtr data);
    public delegate void GSopen(IntPtr hwnd, String Title, int renderer);
    public delegate void GSclose();
    public delegate void GSshutdown();
    public delegate void GSConfigure();
    public delegate void GSsetBaseMem(IntPtr data);
    public delegate IntPtr PSEgetLibName();
    public delegate void GSinit();

    public class GSDXWrapper
    {
        static public bool DumpTooOld = false;

        private GSConfigure gsConfigure;
        private PSEgetLibName PsegetLibName;
        private GSgifTransfer GSgifTransfer;
        private GSgifTransfer1 GSgifTransfer1;
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
        private GSreset GSreset;

        private Boolean Loaded;

        private String DLL;
        private IntPtr DLLAddr;

        private Boolean Running;

        public Queue<TCPMessage> QueueMessage;
        public Boolean DebugMode;
        public GSData CurrentGIFPacket;
        public bool ThereIsWork;
        public AutoResetEvent ExternalEvent;
        public int RunTo;

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
                IntPtr funcaddrGIF1 = NativeMethods.GetProcAddress(hmod, "GSgifTransfer1");
                IntPtr funcaddrGIF2 = NativeMethods.GetProcAddress(hmod, "GSgifTransfer2");
                IntPtr funcaddrGIF3 = NativeMethods.GetProcAddress(hmod, "GSgifTransfer3");
                IntPtr funcaddrVSync = NativeMethods.GetProcAddress(hmod, "GSvsync");
                IntPtr funcaddrSetBaseMem = NativeMethods.GetProcAddress(hmod, "GSsetBaseMem");
                IntPtr funcaddrGSReset = NativeMethods.GetProcAddress(hmod, "GSreset");
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
                this.GSgifTransfer1 = (GSgifTransfer1)Marshal.GetDelegateForFunctionPointer(funcaddrGIF1, typeof(GSgifTransfer1));
                this.GSgifTransfer2 = (GSgifTransfer2)Marshal.GetDelegateForFunctionPointer(funcaddrGIF2, typeof(GSgifTransfer2));
                this.GSgifTransfer3 = (GSgifTransfer3)Marshal.GetDelegateForFunctionPointer(funcaddrGIF3, typeof(GSgifTransfer3));
                this.GSVSync = (GSVSync)Marshal.GetDelegateForFunctionPointer(funcaddrVSync, typeof(GSVSync));
                this.GSsetBaseMem = (GSsetBaseMem)Marshal.GetDelegateForFunctionPointer(funcaddrSetBaseMem, typeof(GSsetBaseMem));
                this.GSopen = (GSopen)Marshal.GetDelegateForFunctionPointer(funcaddrOpen, typeof(GSopen));
                this.GSsetGameCRC = (GSsetGameCRC)Marshal.GetDelegateForFunctionPointer(funcaddrSetCRC, typeof(GSsetGameCRC));
                this.GSclose = (GSclose)Marshal.GetDelegateForFunctionPointer(funcaddrClose, typeof(GSclose));
                this.GSshutdown = (GSshutdown)Marshal.GetDelegateForFunctionPointer(funcaddrShutdown, typeof(GSshutdown));
                this.GSfreeze = (GSfreeze)Marshal.GetDelegateForFunctionPointer(funcaddrFreeze, typeof(GSfreeze));
                this.GSreset = (GSreset)Marshal.GetDelegateForFunctionPointer(funcaddrGSReset, typeof(GSreset));
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
            QueueMessage = new Queue<TCPMessage>();
            Running = true;
            ExternalEvent = new AutoResetEvent(true);

            GSinit();
            byte[] tempregisters = new byte[8192];
            Array.Copy(dump.Registers, tempregisters, 8192);
            fixed (byte* pointer = tempregisters)
            {
                GSsetBaseMem(new IntPtr(pointer));
                Int32 HWND = 0;
                GSopen(new IntPtr(&HWND), "", rendererOverride);
                GSsetGameCRC(dump.CRC, 0);

                fixed (byte* freeze = dump.StateData)
                {
                    byte[] GSFreez = new byte[8];
                    Array.Copy(BitConverter.GetBytes(dump.StateData.Length), 0, GSFreez, 0, 4);
                    Array.Copy(BitConverter.GetBytes(new IntPtr(freeze).ToInt32()), 0, GSFreez, 4, 4);

                    fixed (byte* fr = GSFreez)
                    {
                        int ris = GSfreeze(0, new IntPtr(fr));
                        if (ris == -1)
                        {
                            DumpTooOld = true;
                            return;
                        }
                        GSVSync(1);

                        while (Running)
                        {
                            if (!NativeMethods.IsWindowVisible(new IntPtr(HWND)))
                            {
                                Running = false;
                                break;
                            }

                            GSreset();
                            Marshal.Copy(dump.Registers, 0, new IntPtr(pointer), 8192);
                            GSsetBaseMem(new IntPtr(pointer));
                            GSfreeze(0, new IntPtr(fr));

                            for (int i = 0; i < dump.Data.Count; i++)
                            {
                                GSData itm = dump.Data[i];
                                CurrentGIFPacket = itm;

                                if (DebugMode)
                                {
                                    if (RunTo != -1)
                                    {
                                        if (i == RunTo)
                                        {
                                            RunTo = -1;
                                            int idxnextReg = dump.Data.FindIndex(i, a => a.id == GSType.Registers);
                                            if (idxnextReg != -1)
                                            {
                                                Step(dump.Data[idxnextReg], pointer);
                                            }

                                            GSData g = new GSData();
                                            g.id = GSType.VSync;
                                            Step(g, pointer);

                                            TCPMessage Msg = new TCPMessage();
                                            Msg.MessageType = MessageType.RunToCursor;
                                            Msg.Parameters.Add(i);
                                            Program.Client.Send(Msg);

                                            ExternalEvent.Set();
                                        }
                                        else
                                        {
                                            Step(itm, pointer);
                                        }
                                    }
                                    else
                                    {
                                        while (!ThereIsWork && Running)
                                        {
                                            NativeMessage message;
                                            while (NativeMethods.PeekMessage(out message, IntPtr.Zero, 0, 0, 1))
                                            {
                                                if (!NativeMethods.IsWindowVisible(new IntPtr(HWND)))
                                                {
                                                    Running = false;
                                                }
                                                NativeMethods.TranslateMessage(ref message);
                                                NativeMethods.DispatchMessage(ref message);
                                            }
                                        }

                                        ThereIsWork = false;
                                        if (QueueMessage.Count > 0)
                                        {
                                            TCPMessage Mess = QueueMessage.Dequeue();
                                            switch (Mess.MessageType)
                                            {
                                                case MessageType.Step:
                                                    RunTo = i;
                                                    break;
                                                case MessageType.RunToCursor:
                                                    RunTo = (int)Mess.Parameters[0];
                                                    break;
                                                case MessageType.RunToNextVSync:
                                                    RunTo = dump.Data.FindIndex(i, a => a.id == GSType.VSync);
                                                    break;
                                                default:
                                                    break;
                                            }
                                            break;
                                        }
                                    }
                                }
                                else
                                {
                                    Step(itm, pointer);
                                }
                            }
                        }

                        GSclose();
                        GSshutdown();
                    }
                }
            }
        }

        private unsafe void Step(GSData itm, byte* registers)
        {
            /*"C:\Users\Alessio\Desktop\Plugins\Dll\GSdx-SSE4.dll" "C:\Users\Alessio\Desktop\Plugins\Dumps\gsdx_20101222215004.gs" "GSReplay" 0*/
            switch (itm.id)
            {
                case GSType.Transfer:
                    switch (((GSTransfer)itm).Path)
                    {
                        case GSTransferPath.Path1Old:
                            byte[] data = new byte[16384];
                            int addr = 16384 - itm.data.Length;
                            Array.Copy(itm.data, 0, data, addr, itm.data.Length);
                            fixed (byte* gifdata = data)
                            {
                                GSgifTransfer1(new IntPtr(gifdata), addr);
                            }
                            break;
                        case GSTransferPath.Path2:
                            fixed (byte* gifdata = itm.data)
                            {
                                GSgifTransfer2(new IntPtr(gifdata), (itm.data.Length) / 16);
                            }
                            break;
                        case GSTransferPath.Path3:
                            fixed (byte* gifdata = itm.data)
                            {
                                GSgifTransfer3(new IntPtr(gifdata), (itm.data.Length) / 16);
                            }
                            break;
                        case GSTransferPath.Path1New:
                            fixed (byte* gifdata = itm.data)
                            {
                                GSgifTransfer(new IntPtr(gifdata), (itm.data.Length) / 16);
                            }
                            break;
                    }
                    break;
                case GSType.VSync:
                    GSVSync((*((int*)(registers + 4096)) & 0x2000) > 0 ? (byte)1 : (byte)0);
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

        internal List<Object> GetGifPackets(GSDump dump)
        {
            List<Object> Data = new List<Object>();
            for (int i = 0; i < dump.Data.Count; i++)
            {
                String act = i.ToString() + "|";
                act += dump.Data[i].id.ToString() + "|";
                if (dump.Data[i].GetType().IsSubclassOf(typeof(GSData)))
                {
                    act += ((GSTransfer)dump.Data[i]).Path.ToString() + "|";
                    act += ((GSTransfer)dump.Data[i]).data.Length;
                }
                else
                {
                    act += ((GSData)dump.Data[i]).data.Length;
                }
                Data.Add(act);
            }
            return Data;
        }

        internal object GetGifPacketInfo(GSDump dump, int i)
        {
            if (dump.Data[i].id == GSType.Transfer)
                return GIFTag.ExtractGifTag(dump.Data[i].data, ((GSTransfer)dump.Data[i]).Path);
            else
            {
                switch (dump.Data[i].id)
                {
                    case GSType.VSync:
                        return dump.Data[i].data.Length + "|Field = " + dump.Data[i].data[0].ToString();
                    case GSType.ReadFIFO2:
                        return dump.Data[i].data.Length + "|ReadFIFO2 : Size = " + BitConverter.ToInt32(dump.Data[i].data, 0).ToString() + " byte";
                    case GSType.Registers:
                        return dump.Data[i].data.Length + "|Registers";
                    default:
                        return "";
                }
            }
        }
    }
}
