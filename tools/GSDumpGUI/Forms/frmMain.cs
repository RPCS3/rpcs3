using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;
using System.Security;

namespace GSDumpGUI
{
    public partial class GSDumpGUI : Form
    {
        public List<Process> Processes;

        public GSDumpGUI()
        {
            InitializeComponent();
            Processes = new List<Process>();
        }

        public void ReloadGSDXs()
        {
            txtGSDXDirectory.Text = Properties.Settings.Default.GSDXDir;
            txtDumpsDirectory.Text = Properties.Settings.Default.DumpDir;

            lstGSDX.Items.Clear();
            lstDumps.Items.Clear();

            if (Directory.Exists(txtGSDXDirectory.Text))
            {
                String[] File = Directory.GetFiles(txtGSDXDirectory.Text, "*.dll", SearchOption.TopDirectoryOnly);

                NativeMethods.SetErrorMode(0x8007);
                foreach (var itm in File)
                {
                    IntPtr hmod = NativeMethods.LoadLibrary(itm);
                    if (hmod.ToInt64() > 0)
                    {
                        IntPtr funcaddr = NativeMethods.GetProcAddress(hmod, "GSReplay");
                        if (funcaddr.ToInt64() > 0)
                        {
                            IntPtr funcaddrN = NativeMethods.GetProcAddress(hmod, "PSEgetLibName");
                            if (funcaddrN.ToInt64() > 0)
                            {
                                GSDXImport.PSEgetLibName ps = (GSDXImport.PSEgetLibName)Marshal.GetDelegateForFunctionPointer(funcaddrN, typeof(GSDXImport.PSEgetLibName));
                                lstGSDX.Items.Add(Path.GetFileName(itm) + " | " + Marshal.PtrToStringAnsi(ps.Invoke()));
                            }
                        }
                        else
                        {
                            Int32 id = NativeMethods.GetLastError();
                            System.IO.File.AppendAllText(AppDomain.CurrentDomain.BaseDirectory + "test.txt", itm + " failed to load. Error " + id + Environment.NewLine);
                        }
                        NativeMethods.FreeLibrary(hmod);
                    }
                    else
                    {
                        Int32 id = NativeMethods.GetLastError();
                        System.IO.File.AppendAllText(AppDomain.CurrentDomain.BaseDirectory + "test.txt", itm + " failed to load. Error " + id);
                    }
                }
                NativeMethods.SetErrorMode(0x0000);

                String[] Dumps = Directory.GetFiles(txtDumpsDirectory.Text, "*.gs", SearchOption.TopDirectoryOnly);

                foreach (var itm in Dumps)
                    lstDumps.Items.Add(Path.GetFileName(itm));
            }
        }

        private void GSDumpGUI_Load(object sender, EventArgs e)
        {
            ReloadGSDXs();
            lstDumps.Focus();
            if (lstDumps.Items.Count > 0)
                lstDumps.SelectedIndex = 0;
        }

        private void cmdBrowseGSDX_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog fbd = new FolderBrowserDialog();
            fbd.Description = "Select the GSDX DLL Directory";
            fbd.SelectedPath = AppDomain.CurrentDomain.BaseDirectory;
            if (fbd.ShowDialog() == DialogResult.OK)
                txtGSDXDirectory.Text = fbd.SelectedPath;
        }

        private void cmdBrowseDumps_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog fbd = new FolderBrowserDialog();
            fbd.Description = "Select the GSDX Dumps Directory";
            fbd.SelectedPath = AppDomain.CurrentDomain.BaseDirectory;
            if (fbd.ShowDialog() == DialogResult.OK)
                txtDumpsDirectory.Text = fbd.SelectedPath;
        }

        private void cmdSave_Click(object sender, EventArgs e)
        {
            if (System.IO.Directory.Exists(txtDumpsDirectory.Text))
                Properties.Settings.Default.DumpDir = txtDumpsDirectory.Text;
            else
                MessageBox.Show("Select a correct directory for dumps", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);

            if (System.IO.Directory.Exists(txtGSDXDirectory.Text))
                Properties.Settings.Default.GSDXDir = txtGSDXDirectory.Text;
            else
                MessageBox.Show("Select a correct directory for GSDX", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);

            Properties.Settings.Default.Save();
            ReloadGSDXs();
        }

        private void cmdStart_Click(object sender, EventArgs e)
        {
            // Execute the GSReplay function
            if (lstDumps.SelectedIndex != -1)
            {
                if (lstGSDX.SelectedIndex != -1)
                    ExecuteFunction("GSReplay");
                else
                    MessageBox.Show("Select your GSDX first", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
                MessageBox.Show("Select your Dump first", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void ExecuteFunction(String Function)
        {
            String GSDXName = lstGSDX.SelectedItem.ToString().Split(new char[] { '|' })[0];

            CreateDirs(GSDXName);

            // Set the Arguments to pass to the child
            String DLLPath = Properties.Settings.Default.GSDXDir + "\\" + GSDXName;
            String DumpPath = "";
            if (lstDumps.SelectedItem != null)
                DumpPath = Properties.Settings.Default.DumpDir + "\\" + lstDumps.SelectedItem.ToString();

            // Start the child and link the events.
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.UseShellExecute = false;
            psi.RedirectStandardOutput = true;
            psi.RedirectStandardError = false;
            psi.CreateNoWindow = true;
            psi.FileName = AppDomain.CurrentDomain.BaseDirectory + "GsDumpGUI.exe";
            psi.Arguments = "\"" + DLLPath + "\"" + " \"" + DumpPath + "\"" + " \"" + Function + "\"";
            Process p = Process.Start(psi);
            p.OutputDataReceived += new DataReceivedEventHandler(p_OutputDataReceived);
            p.BeginOutputReadLine();
            p.Exited += new EventHandler(p_Exited);
            Processes.Add(p);
        }

        private static void CreateDirs(String GSDXName)
        {
            // Create and set the config directory.
            String Dir = AppDomain.CurrentDomain.BaseDirectory + "GSDumpGSDXConfigs\\";
            if (!Directory.Exists(Dir))
            {
                Directory.CreateDirectory(Dir);
            }
            Dir += GSDXName;
            if (!Directory.Exists(Dir))
            {
                Directory.CreateDirectory(Dir);
            }
            Dir += "\\Inis\\";
            if (!Directory.Exists(Dir))
            {
                Directory.CreateDirectory(Dir);
                File.Create(Dir + "\\gsdx.ini").Close();
            }
            Dir = AppDomain.CurrentDomain.BaseDirectory + "GSDumpGSDXConfigs\\" + GSDXName;
            Directory.SetCurrentDirectory(Dir);
        }

        void p_Exited(object sender, EventArgs e)
        {
            // Remove the child if is closed
            Processes.Remove((Process)sender);
        }

        void p_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            // Write the log.
            txtLog.Invoke(new Action<object>(delegate(object o) { txtLog.Text += e.Data + Environment.NewLine; }), new object[] { null });
        }

        private void cmdConfigGSDX_Click(object sender, EventArgs e)
        {
            // Execute the GSconfigure function
            if (lstGSDX.SelectedIndex != -1)
                ExecuteFunction("GSconfigure");
            else
                MessageBox.Show("Select your GSDX first", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void cmdOpenIni_Click(object sender, EventArgs e)
        {
            // Execute the GSconfigure function
            if (lstGSDX.SelectedIndex != -1)
            {
                String GSDXName = lstGSDX.SelectedItem.ToString().Split(new char[] { '|' })[0];
                CreateDirs(GSDXName);
                Process.Start(AppDomain.CurrentDomain.BaseDirectory + "GSDumpGSDXConfigs\\" + GSDXName + "\\inis\\gsdx.ini");
            }
            else
                MessageBox.Show("Select your GSDX first", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void lstDumps_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lstDumps.SelectedIndex != -1)
            {
                String Filename = Path.GetDirectoryName(Properties.Settings.Default.DumpDir + "\\") + "\\" + Path.GetFileNameWithoutExtension(lstDumps.SelectedItem.ToString()) + ".bmp";
                if (File.Exists(Filename))
                {
                    pctBox.Image = Image.FromFile(Filename);
                    pctBox.Cursor = Cursors.Hand;
                }
                else
                {
                    Bitmap bt = new Bitmap(320, 240, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
                    Graphics g = Graphics.FromImage(bt);
                    g.FillRectangle(new SolidBrush(Color.Black), new Rectangle(0, 0, 320, 240));
                    g.DrawString("No Image", new Font(FontFamily.GenericSansSerif, 48, FontStyle.Regular), new SolidBrush(Color.White), new PointF(0,70));
                    g.Dispose();
                    pctBox.Image = bt;
                    pctBox.Cursor = Cursors.Default;
                }
            }
        }

        private void pctBox_Click(object sender, EventArgs e)
        {
            if (pctBox.Cursor == Cursors.Hand)
            {
                String Filename = Path.GetDirectoryName(Properties.Settings.Default.DumpDir + "\\") + "\\" + Path.GetFileNameWithoutExtension(lstDumps.SelectedItem.ToString()) + ".bmp";
                Process.Start(Filename);
            }
        }

        private void lstDumps_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return)
                cmdStart_Click(sender, e);
            if (e.KeyCode == Keys.F1)
                cmdConfigGSDX_Click(sender, e);
        }

        private void lstGSDX_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return)
                cmdStart_Click(sender, e);
            if (e.KeyCode == Keys.F1)
                cmdConfigGSDX_Click(sender, e);
        }

        private void GSDumpGUI_KeyDown(object sender, KeyEventArgs e)
        {
            if ((e.KeyCode == Keys.Down))
            {
                e.Handled = true;
                if (lstDumps.Items.Count > lstDumps.SelectedIndex + 1)
                    lstDumps.SelectedIndex++;
            }
            if ((e.KeyCode == Keys.Up))
            {
                e.Handled = true;
                if (lstDumps.SelectedIndex > 0)
                    lstDumps.SelectedIndex--;
            }
        }
    }
}
