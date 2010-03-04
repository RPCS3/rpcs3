namespace GSDumpGUI
{
    partial class GSDumpGUI
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(GSDumpGUI));
            this.txtGSDXDirectory = new System.Windows.Forms.TextBox();
            this.lblDirectory = new System.Windows.Forms.Label();
            this.cmdBrowseGSDX = new System.Windows.Forms.Button();
            this.cmdBrowseDumps = new System.Windows.Forms.Button();
            this.lblDumpDirectory = new System.Windows.Forms.Label();
            this.txtDumpsDirectory = new System.Windows.Forms.TextBox();
            this.cmdSaveAndReload = new System.Windows.Forms.Button();
            this.lstGSDX = new System.Windows.Forms.ListBox();
            this.lstDumps = new System.Windows.Forms.ListBox();
            this.lblDumps = new System.Windows.Forms.Label();
            this.GsdxList = new System.Windows.Forms.Label();
            this.cmdStart = new System.Windows.Forms.Button();
            this.cmdConfigGSDX = new System.Windows.Forms.Button();
            this.txtLog = new System.Windows.Forms.TextBox();
            this.lblLog = new System.Windows.Forms.Label();
            this.cmdOpenIni = new System.Windows.Forms.Button();
            this.pctBox = new System.Windows.Forms.PictureBox();
            this.rdaDX9HW = new System.Windows.Forms.RadioButton();
            this.rdaDX10HW = new System.Windows.Forms.RadioButton();
            this.rdaDX9SW = new System.Windows.Forms.RadioButton();
            this.rdaDX10SW = new System.Windows.Forms.RadioButton();
            this.lblOverride = new System.Windows.Forms.Label();
            this.rdaNone = new System.Windows.Forms.RadioButton();
            ((System.ComponentModel.ISupportInitialize)(this.pctBox)).BeginInit();
            this.SuspendLayout();
            // 
            // txtGSDXDirectory
            // 
            this.txtGSDXDirectory.Location = new System.Drawing.Point(624, 26);
            this.txtGSDXDirectory.Name = "txtGSDXDirectory";
            this.txtGSDXDirectory.Size = new System.Drawing.Size(243, 20);
            this.txtGSDXDirectory.TabIndex = 0;
            this.txtGSDXDirectory.TabStop = false;
            // 
            // lblDirectory
            // 
            this.lblDirectory.AutoSize = true;
            this.lblDirectory.Location = new System.Drawing.Point(627, 10);
            this.lblDirectory.Name = "lblDirectory";
            this.lblDirectory.Size = new System.Drawing.Size(82, 13);
            this.lblDirectory.TabIndex = 1;
            this.lblDirectory.Text = "GSDX Directory";
            // 
            // cmdBrowseGSDX
            // 
            this.cmdBrowseGSDX.Location = new System.Drawing.Point(873, 24);
            this.cmdBrowseGSDX.Name = "cmdBrowseGSDX";
            this.cmdBrowseGSDX.Size = new System.Drawing.Size(29, 23);
            this.cmdBrowseGSDX.TabIndex = 2;
            this.cmdBrowseGSDX.TabStop = false;
            this.cmdBrowseGSDX.Text = "...";
            this.cmdBrowseGSDX.UseVisualStyleBackColor = true;
            this.cmdBrowseGSDX.Click += new System.EventHandler(this.cmdBrowseGSDX_Click);
            // 
            // cmdBrowseDumps
            // 
            this.cmdBrowseDumps.Location = new System.Drawing.Point(873, 67);
            this.cmdBrowseDumps.Name = "cmdBrowseDumps";
            this.cmdBrowseDumps.Size = new System.Drawing.Size(29, 23);
            this.cmdBrowseDumps.TabIndex = 5;
            this.cmdBrowseDumps.TabStop = false;
            this.cmdBrowseDumps.Text = "...";
            this.cmdBrowseDumps.UseVisualStyleBackColor = true;
            this.cmdBrowseDumps.Click += new System.EventHandler(this.cmdBrowseDumps_Click);
            // 
            // lblDumpDirectory
            // 
            this.lblDumpDirectory.AutoSize = true;
            this.lblDumpDirectory.Location = new System.Drawing.Point(624, 54);
            this.lblDumpDirectory.Name = "lblDumpDirectory";
            this.lblDumpDirectory.Size = new System.Drawing.Size(85, 13);
            this.lblDumpDirectory.TabIndex = 4;
            this.lblDumpDirectory.Text = "Dumps Directory";
            // 
            // txtDumpsDirectory
            // 
            this.txtDumpsDirectory.Location = new System.Drawing.Point(624, 70);
            this.txtDumpsDirectory.Name = "txtDumpsDirectory";
            this.txtDumpsDirectory.Size = new System.Drawing.Size(243, 20);
            this.txtDumpsDirectory.TabIndex = 3;
            this.txtDumpsDirectory.TabStop = false;
            // 
            // cmdSaveAndReload
            // 
            this.cmdSaveAndReload.Location = new System.Drawing.Point(626, 97);
            this.cmdSaveAndReload.Name = "cmdSaveAndReload";
            this.cmdSaveAndReload.Size = new System.Drawing.Size(276, 23);
            this.cmdSaveAndReload.TabIndex = 6;
            this.cmdSaveAndReload.TabStop = false;
            this.cmdSaveAndReload.Text = "Save And Reload";
            this.cmdSaveAndReload.UseVisualStyleBackColor = true;
            this.cmdSaveAndReload.Click += new System.EventHandler(this.cmdSave_Click);
            // 
            // lstGSDX
            // 
            this.lstGSDX.FormattingEnabled = true;
            this.lstGSDX.Location = new System.Drawing.Point(319, 24);
            this.lstGSDX.Name = "lstGSDX";
            this.lstGSDX.Size = new System.Drawing.Size(301, 225);
            this.lstGSDX.TabIndex = 1;
            // 
            // lstDumps
            // 
            this.lstDumps.FormattingEnabled = true;
            this.lstDumps.Location = new System.Drawing.Point(12, 24);
            this.lstDumps.Name = "lstDumps";
            this.lstDumps.Size = new System.Drawing.Size(301, 225);
            this.lstDumps.TabIndex = 0;
            this.lstDumps.SelectedIndexChanged += new System.EventHandler(this.lstDumps_SelectedIndexChanged);
            // 
            // lblDumps
            // 
            this.lblDumps.AutoSize = true;
            this.lblDumps.Location = new System.Drawing.Point(9, 8);
            this.lblDumps.Name = "lblDumps";
            this.lblDumps.Size = new System.Drawing.Size(59, 13);
            this.lblDumps.TabIndex = 9;
            this.lblDumps.Text = "Dumps List";
            // 
            // GsdxList
            // 
            this.GsdxList.AutoSize = true;
            this.GsdxList.Location = new System.Drawing.Point(316, 9);
            this.GsdxList.Name = "GsdxList";
            this.GsdxList.Size = new System.Drawing.Size(56, 13);
            this.GsdxList.TabIndex = 10;
            this.GsdxList.Text = "GSDX List";
            // 
            // cmdStart
            // 
            this.cmdStart.Location = new System.Drawing.Point(720, 192);
            this.cmdStart.Name = "cmdStart";
            this.cmdStart.Size = new System.Drawing.Size(182, 58);
            this.cmdStart.TabIndex = 11;
            this.cmdStart.TabStop = false;
            this.cmdStart.Text = "Start";
            this.cmdStart.UseVisualStyleBackColor = true;
            this.cmdStart.Click += new System.EventHandler(this.cmdStart_Click);
            // 
            // cmdConfigGSDX
            // 
            this.cmdConfigGSDX.Location = new System.Drawing.Point(720, 130);
            this.cmdConfigGSDX.Name = "cmdConfigGSDX";
            this.cmdConfigGSDX.Size = new System.Drawing.Size(88, 56);
            this.cmdConfigGSDX.TabIndex = 12;
            this.cmdConfigGSDX.TabStop = false;
            this.cmdConfigGSDX.Text = "Configure Current GSDX";
            this.cmdConfigGSDX.UseVisualStyleBackColor = true;
            this.cmdConfigGSDX.Click += new System.EventHandler(this.cmdConfigGSDX_Click);
            // 
            // txtLog
            // 
            this.txtLog.Location = new System.Drawing.Point(269, 275);
            this.txtLog.Multiline = true;
            this.txtLog.Name = "txtLog";
            this.txtLog.ReadOnly = true;
            this.txtLog.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtLog.Size = new System.Drawing.Size(633, 208);
            this.txtLog.TabIndex = 13;
            this.txtLog.TabStop = false;
            // 
            // lblLog
            // 
            this.lblLog.AutoSize = true;
            this.lblLog.Location = new System.Drawing.Point(266, 259);
            this.lblLog.Name = "lblLog";
            this.lblLog.Size = new System.Drawing.Size(25, 13);
            this.lblLog.TabIndex = 14;
            this.lblLog.Text = "Log";
            // 
            // cmdOpenIni
            // 
            this.cmdOpenIni.Location = new System.Drawing.Point(814, 130);
            this.cmdOpenIni.Name = "cmdOpenIni";
            this.cmdOpenIni.Size = new System.Drawing.Size(88, 56);
            this.cmdOpenIni.TabIndex = 15;
            this.cmdOpenIni.TabStop = false;
            this.cmdOpenIni.Text = "Open GSDX.ini";
            this.cmdOpenIni.UseVisualStyleBackColor = true;
            this.cmdOpenIni.Click += new System.EventHandler(this.cmdOpenIni_Click);
            // 
            // pctBox
            // 
            this.pctBox.Location = new System.Drawing.Point(12, 262);
            this.pctBox.Name = "pctBox";
            this.pctBox.Size = new System.Drawing.Size(248, 221);
            this.pctBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.pctBox.TabIndex = 17;
            this.pctBox.TabStop = false;
            this.pctBox.Click += new System.EventHandler(this.pctBox_Click);
            // 
            // rdaDX9HW
            // 
            this.rdaDX9HW.AutoSize = true;
            this.rdaDX9HW.Location = new System.Drawing.Point(626, 165);
            this.rdaDX9HW.Name = "rdaDX9HW";
            this.rdaDX9HW.Size = new System.Drawing.Size(68, 17);
            this.rdaDX9HW.TabIndex = 18;
            this.rdaDX9HW.Tag = "1";
            this.rdaDX9HW.Text = "DX9 HW";
            this.rdaDX9HW.UseVisualStyleBackColor = true;
            this.rdaDX9HW.CheckedChanged += new System.EventHandler(this.rda_CheckedChanged);
            // 
            // rdaDX10HW
            // 
            this.rdaDX10HW.AutoSize = true;
            this.rdaDX10HW.Location = new System.Drawing.Point(626, 188);
            this.rdaDX10HW.Name = "rdaDX10HW";
            this.rdaDX10HW.Size = new System.Drawing.Size(74, 17);
            this.rdaDX10HW.TabIndex = 19;
            this.rdaDX10HW.Tag = "2";
            this.rdaDX10HW.Text = "DX10 HW";
            this.rdaDX10HW.UseVisualStyleBackColor = true;
            this.rdaDX10HW.CheckedChanged += new System.EventHandler(this.rda_CheckedChanged);
            // 
            // rdaDX9SW
            // 
            this.rdaDX9SW.AutoSize = true;
            this.rdaDX9SW.Location = new System.Drawing.Point(626, 211);
            this.rdaDX9SW.Name = "rdaDX9SW";
            this.rdaDX9SW.Size = new System.Drawing.Size(67, 17);
            this.rdaDX9SW.TabIndex = 20;
            this.rdaDX9SW.Tag = "3";
            this.rdaDX9SW.Text = "DX9 SW";
            this.rdaDX9SW.UseVisualStyleBackColor = true;
            this.rdaDX9SW.CheckedChanged += new System.EventHandler(this.rda_CheckedChanged);
            // 
            // rdaDX10SW
            // 
            this.rdaDX10SW.AutoSize = true;
            this.rdaDX10SW.Location = new System.Drawing.Point(626, 234);
            this.rdaDX10SW.Name = "rdaDX10SW";
            this.rdaDX10SW.Size = new System.Drawing.Size(73, 17);
            this.rdaDX10SW.TabIndex = 21;
            this.rdaDX10SW.Tag = "4";
            this.rdaDX10SW.Text = "DX10 SW";
            this.rdaDX10SW.UseVisualStyleBackColor = true;
            this.rdaDX10SW.CheckedChanged += new System.EventHandler(this.rda_CheckedChanged);
            // 
            // lblOverride
            // 
            this.lblOverride.AutoSize = true;
            this.lblOverride.Location = new System.Drawing.Point(621, 130);
            this.lblOverride.Name = "lblOverride";
            this.lblOverride.Size = new System.Drawing.Size(94, 13);
            this.lblOverride.TabIndex = 22;
            this.lblOverride.Text = "Renderer Override";
            // 
            // rdaNone
            // 
            this.rdaNone.AutoSize = true;
            this.rdaNone.Checked = true;
            this.rdaNone.Location = new System.Drawing.Point(625, 146);
            this.rdaNone.Name = "rdaNone";
            this.rdaNone.Size = new System.Drawing.Size(51, 17);
            this.rdaNone.TabIndex = 23;
            this.rdaNone.TabStop = true;
            this.rdaNone.Tag = "0";
            this.rdaNone.Text = "None";
            this.rdaNone.UseVisualStyleBackColor = true;
            this.rdaNone.CheckedChanged += new System.EventHandler(this.rda_CheckedChanged);
            // 
            // GSDumpGUI
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(906, 495);
            this.Controls.Add(this.rdaNone);
            this.Controls.Add(this.lblOverride);
            this.Controls.Add(this.rdaDX10SW);
            this.Controls.Add(this.rdaDX9SW);
            this.Controls.Add(this.rdaDX10HW);
            this.Controls.Add(this.rdaDX9HW);
            this.Controls.Add(this.lstGSDX);
            this.Controls.Add(this.pctBox);
            this.Controls.Add(this.cmdOpenIni);
            this.Controls.Add(this.lblLog);
            this.Controls.Add(this.txtLog);
            this.Controls.Add(this.cmdConfigGSDX);
            this.Controls.Add(this.cmdStart);
            this.Controls.Add(this.GsdxList);
            this.Controls.Add(this.lblDumps);
            this.Controls.Add(this.lstDumps);
            this.Controls.Add(this.cmdSaveAndReload);
            this.Controls.Add(this.cmdBrowseDumps);
            this.Controls.Add(this.lblDumpDirectory);
            this.Controls.Add(this.txtDumpsDirectory);
            this.Controls.Add(this.cmdBrowseGSDX);
            this.Controls.Add(this.lblDirectory);
            this.Controls.Add(this.txtGSDXDirectory);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.KeyPreview = true;
            this.MaximizeBox = false;
            this.Name = "GSDumpGUI";
            this.Text = "GSDumpGUI";
            this.Load += new System.EventHandler(this.GSDumpGUI_Load);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.GSDumpGUI_KeyDown);
            ((System.ComponentModel.ISupportInitialize)(this.pctBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox txtGSDXDirectory;
        private System.Windows.Forms.Label lblDirectory;
        private System.Windows.Forms.Button cmdBrowseGSDX;
        private System.Windows.Forms.Button cmdBrowseDumps;
        private System.Windows.Forms.Label lblDumpDirectory;
        private System.Windows.Forms.TextBox txtDumpsDirectory;
        private System.Windows.Forms.Button cmdSaveAndReload;
        private System.Windows.Forms.ListBox lstGSDX;
        private System.Windows.Forms.ListBox lstDumps;
        private System.Windows.Forms.Label lblDumps;
        private System.Windows.Forms.Label GsdxList;
        private System.Windows.Forms.Button cmdStart;
        private System.Windows.Forms.Button cmdConfigGSDX;
        private System.Windows.Forms.TextBox txtLog;
        private System.Windows.Forms.Label lblLog;
        private System.Windows.Forms.Button cmdOpenIni;
        private System.Windows.Forms.PictureBox pctBox;
        private System.Windows.Forms.RadioButton rdaDX9HW;
        private System.Windows.Forms.RadioButton rdaDX10HW;
        private System.Windows.Forms.RadioButton rdaDX9SW;
        private System.Windows.Forms.RadioButton rdaDX10SW;
        private System.Windows.Forms.Label lblOverride;
        private System.Windows.Forms.RadioButton rdaNone;
    }
}

