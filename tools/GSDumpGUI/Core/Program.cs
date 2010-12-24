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
using System.Drawing;

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
        static private GSDXWrapper wrap;

        static private TreeNode CurrentNode;

        [STAThread]
        static void Main(String[] args)
        {
            if (args.Length == 4)
            {
                // do this first, else racy mess ;)
                wrap = new GSDXWrapper();

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
                        IntPtr pt = Process.GetCurrentProcess().MainWindowHandle;
                        if (ChangeIcon)
                        {
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

                wrap.Load(DLLPath);
                Directory.SetCurrentDirectory(Path.GetDirectoryName(AppDomain.CurrentDomain.BaseDirectory + "GSDumpGSDXConfigs\\" + Path.GetFileName(DLLPath) + "\\"));
                if (Operation == "GSReplay")
                {
                    dump = GSDump.LoadDump(DumpPath);

                    if (Client != null)
                    {
                        SendStatistics();
                        SendDumpSize();
                    }

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
            RefreshList(false);
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
                case MessageType.GetDebugMode:
                    frmMain.Invoke(new Action<object>(delegate(object e)
                    {
                        frmMain.chkDebugMode.Checked = (Boolean)Mess.Parameters[0];

                        frmMain.lblGif.Enabled = frmMain.chkDebugMode.Checked;
                        frmMain.btnRunToSelection.Enabled = frmMain.chkDebugMode.Checked;
                        frmMain.treTreeView.Enabled = frmMain.chkDebugMode.Checked;
                        frmMain.btnStep.Enabled = frmMain.chkDebugMode.Checked;
                        frmMain.cmdGoToStart.Enabled = frmMain.chkDebugMode.Checked;
                        frmMain.cmdGoToNextVSync.Enabled = frmMain.chkDebugMode.Checked;
                        frmMain.treeGifPacketContent.Enabled = frmMain.chkDebugMode.Checked;
                        if (frmMain.chkDebugMode.Checked == false)
                            frmMain.treTreeView.Nodes.Clear();

                    }), new object[] { null });
                    break;
                case MessageType.DebugState:
                    frmMain.Invoke(new Action<object>(delegate(object e)
                    {
                        frmMain.treTreeView.Nodes.Clear();
                        List<TreeNode> parents = new List<TreeNode>();
                        List<TreeNode> nodes = new List<TreeNode>();
                        foreach (var itm in Mess.Parameters)
                        {
                            String[] parts = itm.ToString().Split(new char[] { '|' });
                            switch (parts[1])
                            {
                                case "Transfer":
                                    TreeNode tn2 = new TreeNode();
                                    tn2.Name = parts[0];
                                    tn2.Text = parts[0] + " - " + parts[1] + " - " + parts[2] + " - " + parts[3] + " byte";
                                    nodes.Add(tn2);
                                    break;
                                case "ReadFIFO2":
                                    TreeNode tn3 = new TreeNode();
                                    tn3.Name = parts[0];
                                    tn3.Text = parts[0] + " - " + parts[1] + " - " + parts[2] + " byte";
                                    nodes.Add(tn3);
                                    break;
                                case "VSync":
                                    TreeNode tn = new TreeNode();
                                    tn.Name = parts[0];
                                    tn.Text = parts[0] + " - " + parts[1] + " - " + parts[2] + " byte";
                                    tn.Nodes.AddRange(nodes.ToArray());
                                    parents.Add(tn);

                                    nodes.Clear();
                                    break;
                                case "Registers":
                                    TreeNode tn4 = new TreeNode();
                                    tn4.Name = parts[0];
                                    tn4.Text = parts[0] + " - " + parts[1] + " - " + parts[2] + " byte";
                                    nodes.Add(tn4);
                                    break;
                            }
                        }
                        frmMain.treTreeView.Nodes.AddRange(parents.ToArray());
                    }), new object[] { null });                    
                    break;
                case MessageType.Step:
                case MessageType.RunToCursor:
                    frmMain.Invoke(new Action<object>(delegate(object e)
                    {
                        int idtoselect = (int)Mess.Parameters[0];
                        TreeNode[] noes = frmMain.treTreeView.Nodes.Find(idtoselect.ToString(), true);
                        if (noes.Length > 0)
                        {
                            if (CurrentNode != null)
                                CurrentNode.BackColor = Color.White;
                            noes[0].BackColor = Color.LightBlue;
                            CurrentNode = noes[0];
                            frmMain.treTreeView.SelectedNode = noes[0];
                        }
                    }), new object[] { null });
                    break;
                case MessageType.PacketInfo:
                    frmMain.Invoke(new Action<object>(delegate(object e)
                    {
                        if (Mess.Parameters[0].ToString() != "No Data Available")
                        {
                            string[] vals = Mess.Parameters[0].ToString().Split('|');
                            frmMain.txtGifPacketSize.Text = vals[0] + " bytes";

                            frmMain.treeGifPacketContent.Nodes.Clear();

                            frmMain.treeGifPacketContent.Nodes.Add(vals[1]);

                            if (vals.Length > 2)
                            {
                                frmMain.treeGifPacketContent.Nodes[0].Nodes.Add(vals[2]);
                                frmMain.treeGifPacketContent.Nodes[0].Nodes.Add(vals[3]);
                                frmMain.treeGifPacketContent.Nodes[0].Nodes.Add(vals[4]);
                                frmMain.treeGifPacketContent.Nodes[0].Nodes.Add(vals[5]);

                                TreeNode nodePrim = new TreeNode("Prim");
                                string[] prim = vals[6].Split('~');
                                for (int j = 1; j < prim.Length; j++)
                                {
                                    nodePrim.Nodes.Add(prim[j]);
                                }
                                frmMain.treeGifPacketContent.Nodes[0].Nodes.Add(nodePrim);

                                frmMain.treeGifPacketContent.Nodes[0].Nodes.Add(vals[7]);

                                TreeNode nodeReg = new TreeNode("reg");
                                string[] reg = vals[8].Split(new char[]{'~'}, StringSplitOptions.RemoveEmptyEntries);
                                for (int j = 1; j < reg.Length; j++)
                                {
                                    nodeReg.Nodes.Add(reg[j]);
                                }
                                frmMain.treeGifPacketContent.Nodes[0].Nodes.Add(nodeReg);
                            }
                            frmMain.treeGifPacketContent.Nodes[0].ExpandAll();
                        }
                        else
                        {
                            frmMain.treeGifPacketContent.Nodes.Clear();

                            frmMain.treeGifPacketContent.Nodes.Add(Mess.Parameters[0].ToString());
                        }
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
                    SendDumpSize();
                    break;
                case MessageType.Statistics:
                    SendStatistics();
                    break;
                case MessageType.SetDebugMode:
                    wrap.DebugMode = (Boolean)Mess.Parameters[0];

                    msg = new TCPMessage();
                    msg.MessageType = MessageType.GetDebugMode;
                    msg.Parameters.Add(wrap.DebugMode);
                    Client.Send(msg);

                    if (wrap.DebugMode)
                    {
                        msg = new TCPMessage();
                        msg.MessageType = MessageType.DebugState;
                        msg.Parameters.AddRange(wrap.GetGifPackets(dump));
                        Client.Send(msg);

                        msg = new TCPMessage();
                        msg.MessageType = MessageType.Step;
                        msg.Parameters.Add(dump.Data.FindIndex(a => a == wrap.CurrentGIFPacket));
                        Client.Send(msg);
                    }
                    break;
                case MessageType.GetDebugMode:
                    msg = new TCPMessage();
                    msg.MessageType = MessageType.GetDebugMode;
                    msg.Parameters.Add(wrap.DebugMode);
                    Client.Send(msg);

                    if (wrap.DebugMode)
                    {
                        msg = new TCPMessage();
                        msg.MessageType = MessageType.DebugState;
                        msg.Parameters.AddRange(wrap.GetGifPackets(dump));
                        Client.Send(msg);

                        msg = new TCPMessage();
                        msg.MessageType = MessageType.Step;
                        msg.Parameters.Add(dump.Data.FindIndex(a => a == wrap.CurrentGIFPacket));
                        Client.Send(msg);
                    }
                    break;

                case MessageType.PacketInfo:
                    int id = (int)Mess.Parameters[0];
                    msg = new TCPMessage();
                    msg.MessageType = MessageType.PacketInfo;
                    msg.Parameters.Add(wrap.GetGifPacketInfo(dump, id));
                    Client.Send(msg);
                    break;
                case MessageType.Step:
                case MessageType.RunToCursor:
                case MessageType.RunToNextVSync:
                    wrap.ExternalEvent.WaitOne();
                    wrap.ExternalEvent.Reset();
                    wrap.QueueMessage.Enqueue(Mess);
                    wrap.ThereIsWork = true;
                    break;
                default:
                    break;
            }
        }

        private static void SendDumpSize()
        {
            TCPMessage msg;
            msg = new TCPMessage();
            msg.MessageType = MessageType.SizeDump;
            if (dump != null)
                msg.Parameters.Add(dump.Size);
            else
                msg.Parameters.Add(0);
            Client.Send(msg);
        }

        private static void SendStatistics()
        {
            TCPMessage msg;
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
        }

        static void Server_OnClientAfterConnect(TCPLibrary.Core.Server server, TCPLibrary.Core.ClientS sender)
        {
            Clients.Add((TCPLibrary.MessageBased.Core.BaseMessageClientS)sender);
            RefreshList(true);
        }

        private static void RefreshList(bool SelectLast)
        {
            frmMain.Invoke(new Action<object>( delegate(object e)
            {
                frmMain.lstProcesses.Items.Clear(); 
                
                foreach (var itm in Clients)
                {
                    frmMain.lstProcesses.Items.Add(itm.IPAddress);
                }
                if (SelectLast)
                    frmMain.lstProcesses.SelectedIndex = frmMain.lstProcesses.Items.Count - 1;
                if (frmMain.lstProcesses.SelectedIndex == -1)
                {
                    frmMain.chkDebugMode.Checked = false;
                    frmMain.lblGif.Enabled = frmMain.chkDebugMode.Checked;
                    frmMain.btnRunToSelection.Enabled = frmMain.chkDebugMode.Checked;
                    frmMain.treTreeView.Enabled = frmMain.chkDebugMode.Checked;
                    frmMain.btnStep.Enabled = frmMain.chkDebugMode.Checked;
                    frmMain.cmdGoToStart.Enabled = frmMain.chkDebugMode.Checked;
                    frmMain.cmdGoToNextVSync.Enabled = frmMain.chkDebugMode.Checked;
                    frmMain.treTreeView.Nodes.Clear();
                    frmMain.treeGifPacketContent.Nodes.Clear();
                }
            }), new object[] { null});
        }
    }
}
