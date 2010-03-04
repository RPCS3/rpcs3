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

        private Int32 _selected;
        public Int32 SelectedRad
        {
            get { return _selected; }
            set
            {
                if (value > 4)
                    value = 0;
                _selected = value;
                switch (_selected)
                {
                    case 0:
                        rdaNone.Checked = true;
                        break;
                    case 1:
                        rdaDX9HW.Checked = true;
                        break;
                    case 2:
                        rdaDX10HW.Checked = true;
                        break;
                    case 3:
                        rdaDX9SW.Checked = true;
                        break;
                    case 4:
                        rdaDX10SW.Checked = true;
                        break;
                }
            }
        }

        private Bitmap NoImage;

        public GSDumpGUI()
        {
            InitializeComponent();
            Processes = new List<Process>();

            NoImage = new Bitmap(320, 240, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
            Graphics g = Graphics.FromImage(NoImage);
            g.FillRectangle(new SolidBrush(Color.Black), new Rectangle(0, 0, 320, 240));
            g.DrawString("No Image", new Font(FontFamily.GenericSansSerif, 48, FontStyle.Regular), new SolidBrush(Color.White), new PointF(0, 70));
            g.Dispose();
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

                GSDXWrapper wrap = new GSDXWrapper();
                foreach (var itm in File)
                {
                    if (GSDXWrapper.IsValidGSDX(itm))
                    {
                        wrap.Load(itm);
                        lstGSDX.Items.Add(Path.GetFileName(itm) + " | " + wrap.PSEGetLibName());
                        wrap.Unload();
                    }
                }

                String[] Dumps = Directory.GetFiles(txtDumpsDirectory.Text, "*.gs", SearchOption.TopDirectoryOnly);

                foreach (var itm in Dumps)
                {
                    BinaryReader br = new BinaryReader(System.IO.File.Open(itm, FileMode.Open));
                    Int32 CRC = br.ReadInt32();
                    br.Close();
                    lstDumps.Items.Add(Path.GetFileName(itm) +  " | CRC : " + CRC.ToString("X"));
                }
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
            txtLog.Text = "";
            String GSDXName = lstGSDX.SelectedItem.ToString().Split(new char[] { '|' })[0];

            CreateDirs(GSDXName);

            // Set the Arguments to pass to the child
            String DLLPath = Properties.Settings.Default.GSDXDir + "\\" + GSDXName;
            String DumpPath = "";
            String SelectedRenderer = "";
            switch (SelectedRad)
            {
                case 0:
                    SelectedRenderer = "-1";
                    break;
                case 1:
                    SelectedRenderer = "0";
                    break;
                case 2:
                    SelectedRenderer = "3";
                    break;
                case 3:
                    SelectedRenderer = "1";
                    break;
                case 4:
                    SelectedRenderer = "4";
                    break;
            }
            if (lstDumps.SelectedItem != null)
                DumpPath = Properties.Settings.Default.DumpDir + "\\" + 
                           lstDumps.SelectedItem.ToString().Split(new char[] { '|' })[0];

            // Start the child and link the events.
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.UseShellExecute = false;
            psi.RedirectStandardOutput = true;
            psi.RedirectStandardError = false;
            psi.CreateNoWindow = true;
            psi.FileName = AppDomain.CurrentDomain.BaseDirectory + "GsDumpGUI.exe";
            psi.Arguments = "\"" + DLLPath + "\"" + " \"" + DumpPath + "\"" + " \"" + Function + "\"" + " " + SelectedRenderer;
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
            txtLog.Invoke(new Action<object>(delegate(object o) 
                { 
                    txtLog.Text += e.Data + Environment.NewLine; 
                    txtLog.SelectionStart = txtLog.Text.Length - 1;
                    txtLog.ScrollToCaret();
                }), new object[] { null });
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
                String DumpFileName = lstDumps.SelectedItem.ToString().Split(new char[] { '|' })[0];
                String Filename = Path.GetDirectoryName(Properties.Settings.Default.DumpDir + "\\") + 
                                  "\\" + Path.GetFileNameWithoutExtension(DumpFileName) + ".bmp";
                if (File.Exists(Filename))
                {
                    pctBox.Image = Image.FromFile(Filename);
                    pctBox.Cursor = Cursors.Hand;
                }
                else
                {
                    pctBox.Image = NoImage;
                    pctBox.Cursor = Cursors.Default;
                }
            }
        }

        private void pctBox_Click(object sender, EventArgs e)
        {
            if (pctBox.Cursor == Cursors.Hand)
            {
                String DumpFileName = lstDumps.SelectedItem.ToString().Split(new char[] { '|' })[0];
                String Filename = Path.GetDirectoryName(Properties.Settings.Default.DumpDir + "\\") +
                                  "\\" + Path.GetFileNameWithoutExtension(DumpFileName) + ".bmp";
                Process.Start(Filename);
            }
        }

        private void GSDumpGUI_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return)
                cmdStart_Click(sender, e);

            if (e.KeyCode == Keys.F1)
                cmdConfigGSDX_Click(sender, e);

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

            if ((e.KeyCode == Keys.F2))
                SelectedRad++;
        }

        private void rda_CheckedChanged(object sender, EventArgs e)
        {
            RadioButton itm = ((RadioButton)(sender));
            if (itm.Checked == true)
                SelectedRad = Convert.ToInt32(itm.Tag);
        }
    }
}
