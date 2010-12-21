using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Specialized = System.Collections.Specialized;
using Reflection = System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;
using GSDumpGUI.Properties;
using System.IO;
using TCPLibrary.MessageBased.Core;

namespace GSDumpGUI
{
    static class Program
    {
        static public GSDumpGUI frmMain;
        static public TCPLibrary.MessageBased.Core.BaseMessageServer Server;
        static public List<TCPLibrary.MessageBased.Core.BaseMessageClientS> Clients;

        static public TCPLibrary.MessageBased.Core.BaseMessageClient Client;
        static private Boolean ChangeIcon;
        static private GSDump dump;

        [STAThread]
        static void Main(String[] args)
        {
            if (args.Length == 4)
            {
                try
                {
                    Client = new TCPLibrary.MessageBased.Core.BaseMessageClient();
                    Client.OnMessageReceived += new TCPLibrary.MessageBased.Core.BaseMessageClient.MessageReceivedHandler(Client_OnMessageReceived);
                    Client.Connect("localhost", 9999);
                }
                catch (Exception)
                {
                    Client = null;
                }

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
                Directory.SetCurrentDirectory(Path.GetDirectoryName(AppDomain.CurrentDomain.BaseDirectory + "GSDumpGSDXConfigs\\" + Path.GetFileName(DLLPath) + "\\"));
                if (Operation == "GSReplay")
                {
                    dump = GSDump.LoadDump(DumpPath);
                    wrap.Run(dump, Renderer);
                    ChangeIcon = true;
                }
                else
                    wrap.GSConfig();
                wrap.Unload();

                if (GSDXWrapper.DumpTooOld)
                {
                    if (Client != null)
                    {
                        TCPMessage msg = new TCPMessage();
                        msg.MessageType = MessageType.StateOld;
                        Client.Send(msg);
                    }
                }

                if (Client != null)
                    Client.Disconnect();
            }
            else
            {
                Clients = new List<TCPLibrary.MessageBased.Core.BaseMessageClientS>();

                Server = new TCPLibrary.MessageBased.Core.BaseMessageServer();
                Server.OnClientMessageReceived += new BaseMessageServer.MessageReceivedHandler(Server_OnClientMessageReceived);
                Server.OnClientAfterConnect += new TCPLibrary.Core.Server.ConnectedHandler(Server_OnClientAfterConnect);
                Server.OnClientAfterDisconnected += new TCPLibrary.Core.Server.DisconnectedHandler(Server_OnClientAfterDisconnected);
                Server.Port = 9999;
                Server.Enabled = true;

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                frmMain = new GSDumpGUI();
                Application.Run(frmMain);

                Server.Enabled = false;
            }
        }

        static void Server_OnClientAfterDisconnected(TCPLibrary.Core.Server server, TCPLibrary.Core.ClientS sender)
        {
            Clients.Remove((TCPLibrary.MessageBased.Core.BaseMessageClientS)sender);
            RefreshList();
        }

        static void Server_OnClientMessageReceived(BaseMessageServer server, BaseMessageClientS sender, TCPMessage Mess)
        {
            switch (Mess.MessageType)
            {
                case MessageType.Connect:
                    break;
                case MessageType.MaxUsers:
                    break;
                case MessageType.SizeDump:
                    frmMain.Invoke(new Action<object>(delegate(object e)
                       {
                           frmMain.txtDumpSize.Text = (((int)Mess.Parameters[0]) / 1024f / 1024f).ToString("F2") + " MB";
                       }), new object[] { null });
                    break;
                case MessageType.Statistics:
                    frmMain.Invoke(new Action<object>(delegate(object e)
                       {
                           frmMain.txtGIFPackets.Text = ((int)Mess.Parameters[0]).ToString();
                           frmMain.txtPath1.Text = ((int)Mess.Parameters[1]).ToString();
                           frmMain.txtPath2.Text = ((int)Mess.Parameters[2]).ToString();
                           frmMain.txtPath3.Text = ((int)Mess.Parameters[3]).ToString();
                           frmMain.txtReadFifo.Text = ((int)Mess.Parameters[5]).ToString();
                           frmMain.txtVSync.Text = ((int)Mess.Parameters[4]).ToString();
                           frmMain.txtRegisters.Text = ((int)Mess.Parameters[6]).ToString();
                       }), new object[] { null });
                    break;
                case MessageType.StateOld:
                    frmMain.Invoke(new Action<object>(delegate(object e)
                    {
                        MessageBox.Show("Savestate too old to be read. :(", "Warning");
                        frmMain.Focus();
                    }), new object[] { null });                    
                    break;
                default:
                    break;
            }
        }

        static void Client_OnMessageReceived(TCPLibrary.Core.Client sender, TCPLibrary.MessageBased.Core.TCPMessage Mess)
        {
            TCPMessage msg;
            switch (Mess.MessageType)
            {
                case TCPLibrary.MessageBased.Core.MessageType.Connect:
                    break;
                case TCPLibrary.MessageBased.Core.MessageType.MaxUsers:
                    break;
                case TCPLibrary.MessageBased.Core.MessageType.SizeDump:
                    msg = new TCPMessage();
                    msg.MessageType = MessageType.SizeDump;
                    if (dump != null)
                        msg.Parameters.Add(dump.Size);
                    else
                        msg.Parameters.Add(0);
                    Client.Send(msg);
                    break;
                case MessageType.Statistics:
                    msg = new TCPMessage();
                    msg.MessageType = MessageType.Statistics;
                    if (dump != null)
                    {
                        msg.Parameters.Add(dump.Data.Count);
                        msg.Parameters.Add(dump.Data.FindAll(a => (int)a.id == 0 && (a.data[0] == 3 || a.data[0] == 0)).Count);
                        msg.Parameters.Add(dump.Data.FindAll(a => (int)a.id == 0 && a.data[0] == 1).Count);
                        msg.Parameters.Add(dump.Data.FindAll(a => (int)a.id == 0 && a.data[0] == 2).Count);
                        msg.Parameters.Add(dump.Data.FindAll(a => (int)a.id == 1).Count);
                        msg.Parameters.Add(dump.Data.FindAll(a => (int)a.id == 2).Count);
                        msg.Parameters.Add(dump.Data.FindAll(a => (int)a.id == 3).Count);
                    }
                    else
                    {
                        msg.Parameters.Add(0);
                        msg.Parameters.Add(0);
                        msg.Parameters.Add(0);
                        msg.Parameters.Add(0);
                        msg.Parameters.Add(0);
                        msg.Parameters.Add(0);
                        msg.Parameters.Add(0);
                    }
                    Client.Send(msg);
                    break;
                default:
                    break;
            }
        }

        static void Server_OnClientAfterConnect(TCPLibrary.Core.Server server, TCPLibrary.Core.ClientS sender)
        {
            Clients.Add((TCPLibrary.MessageBased.Core.BaseMessageClientS)sender);
            RefreshList();
        }

        private static void RefreshList()
        {
            frmMain.Invoke(new Action<object>( delegate(object e)
            {
                frmMain.lstProcesses.Items.Clear(); 
                
                foreach (var itm in Clients)
                {
                    frmMain.lstProcesses.Items.Add(itm.IPAddress);
                }
            }), new object[] { null});
        }
    }
}
