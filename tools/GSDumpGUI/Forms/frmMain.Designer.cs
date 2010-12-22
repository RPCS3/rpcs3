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
            this.lblInternalLog = new System.Windows.Forms.Label();
            this.txtIntLog = new System.Windows.Forms.TextBox();
            this.lblDebugger = new System.Windows.Forms.Label();
            this.lstProcesses = new System.Windows.Forms.ListBox();
            this.lblChild = new System.Windows.Forms.Label();
            this.lblDumpSize = new System.Windows.Forms.Label();
            this.txtDumpSize = new System.Windows.Forms.Label();
            this.txtGIFPackets = new System.Windows.Forms.Label();
            this.lblGIFPackets = new System.Windows.Forms.Label();
            this.txtPath1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.txtPath2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.txtPath3 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.txtVSync = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.txtReadFifo = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.txtRegisters = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.chkDebugMode = new System.Windows.Forms.CheckBox();
            this.lblGif = new System.Windows.Forms.Label();
            this.btnStep = new System.Windows.Forms.Button();
            this.btnRunToSelection = new System.Windows.Forms.Button();
            this.treTreeView = new System.Windows.Forms.TreeView();
            ((System.ComponentModel.ISupportInitialize)(this.pctBox)).BeginInit();
            this.SuspendLayout();
            // 
            // txtGSDXDirectory
            // 
            this.txtGSDXDirectory.Location = new System.Drawing.Point(703, 25);
            this.txtGSDXDirectory.Name = "txtGSDXDirectory";
            this.txtGSDXDirectory.Size = new System.Drawing.Size(243, 20);
            this.txtGSDXDirectory.TabIndex = 0;
            this.txtGSDXDirectory.TabStop = false;
            this.txtGSDXDirectory.Leave += new System.EventHandler(this.txtGSDXDirectory_Leave);
            // 
            // lblDirectory
            // 
            this.lblDirectory.AutoSize = true;
            this.lblDirectory.Location = new System.Drawing.Point(706, 9);
            this.lblDirectory.Name = "lblDirectory";
            this.lblDirectory.Size = new System.Drawing.Size(82, 13);
            this.lblDirectory.TabIndex = 1;
            this.lblDirectory.Text = "GSDX Directory";
            // 
            // cmdBrowseGSDX
            // 
            this.cmdBrowseGSDX.Location = new System.Drawing.Point(952, 23);
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
            this.cmdBrowseDumps.Location = new System.Drawing.Point(952, 66);
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
            this.lblDumpDirectory.Location = new System.Drawing.Point(703, 53);
            this.lblDumpDirectory.Name = "lblDumpDirectory";
            this.lblDumpDirectory.Size = new System.Drawing.Size(85, 13);
            this.lblDumpDirectory.TabIndex = 4;
            this.lblDumpDirectory.Text = "Dumps Directory";
            // 
            // txtDumpsDirectory
            // 
            this.txtDumpsDirectory.Location = new System.Drawing.Point(703, 69);
            this.txtDumpsDirectory.Name = "txtDumpsDirectory";
            this.txtDumpsDirectory.Size = new System.Drawing.Size(243, 20);
            this.txtDumpsDirectory.TabIndex = 3;
            this.txtDumpsDirectory.TabStop = false;
            this.txtDumpsDirectory.Leave += new System.EventHandler(this.txtDumpsDirectory_Leave);
            // 
            // lstGSDX
            // 
            this.lstGSDX.FormattingEnabled = true;
            this.lstGSDX.Location = new System.Drawing.Point(367, 24);
            this.lstGSDX.Name = "lstGSDX";
            this.lstGSDX.Size = new System.Drawing.Size(330, 199);
            this.lstGSDX.TabIndex = 1;
            // 
            // lstDumps
            // 
            this.lstDumps.FormattingEnabled = true;
            this.lstDumps.Location = new System.Drawing.Point(12, 24);
            this.lstDumps.Name = "lstDumps";
            this.lstDumps.Size = new System.Drawing.Size(349, 199);
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
            this.GsdxList.Location = new System.Drawing.Point(364, 9);
            this.GsdxList.Name = "GsdxList";
            this.GsdxList.Size = new System.Drawing.Size(56, 13);
            this.GsdxList.TabIndex = 10;
            this.GsdxList.Text = "GSDX List";
            // 
            // cmdStart
            // 
            this.cmdStart.Location = new System.Drawing.Point(802, 164);
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
            this.cmdConfigGSDX.Location = new System.Drawing.Point(802, 102);
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
            this.txtLog.Location = new System.Drawing.Point(269, 249);
            this.txtLog.Multiline = true;
            this.txtLog.Name = "txtLog";
            this.txtLog.ReadOnly = true;
            this.txtLog.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtLog.Size = new System.Drawing.Size(351, 208);
            this.txtLog.TabIndex = 13;
            this.txtLog.TabStop = false;
            // 
            // lblLog
            // 
            this.lblLog.AutoSize = true;
            this.lblLog.Location = new System.Drawing.Point(266, 233);
            this.lblLog.Name = "lblLog";
            this.lblLog.Size = new System.Drawing.Size(58, 13);
            this.lblLog.TabIndex = 14;
            this.lblLog.Text = "Log GSDX";
            // 
            // cmdOpenIni
            // 
            this.cmdOpenIni.Location = new System.Drawing.Point(896, 102);
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
            this.pctBox.Location = new System.Drawing.Point(12, 236);
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
            this.rdaDX9HW.Location = new System.Drawing.Point(708, 137);
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
            this.rdaDX10HW.Location = new System.Drawing.Point(708, 160);
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
            this.rdaDX9SW.Location = new System.Drawing.Point(708, 183);
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
            this.rdaDX10SW.Location = new System.Drawing.Point(708, 206);
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
            this.lblOverride.Location = new System.Drawing.Point(703, 102);
            this.lblOverride.Name = "lblOverride";
            this.lblOverride.Size = new System.Drawing.Size(94, 13);
            this.lblOverride.TabIndex = 22;
            this.lblOverride.Text = "Renderer Override";
            // 
            // rdaNone
            // 
            this.rdaNone.AutoSize = true;
            this.rdaNone.Checked = true;
            this.rdaNone.Location = new System.Drawing.Point(707, 118);
            this.rdaNone.Name = "rdaNone";
            this.rdaNone.Size = new System.Drawing.Size(51, 17);
            this.rdaNone.TabIndex = 23;
            this.rdaNone.TabStop = true;
            this.rdaNone.Tag = "0";
            this.rdaNone.Text = "None";
            this.rdaNone.UseVisualStyleBackColor = true;
            this.rdaNone.CheckedChanged += new System.EventHandler(this.rda_CheckedChanged);
            // 
            // lblInternalLog
            // 
            this.lblInternalLog.AutoSize = true;
            this.lblInternalLog.Location = new System.Drawing.Point(626, 233);
            this.lblInternalLog.Name = "lblInternalLog";
            this.lblInternalLog.Size = new System.Drawing.Size(63, 13);
            this.lblInternalLog.TabIndex = 25;
            this.lblInternalLog.Text = "Log Internal";
            // 
            // txtIntLog
            // 
            this.txtIntLog.Location = new System.Drawing.Point(629, 249);
            this.txtIntLog.Multiline = true;
            this.txtIntLog.Name = "txtIntLog";
            this.txtIntLog.ReadOnly = true;
            this.txtIntLog.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtIntLog.Size = new System.Drawing.Size(351, 208);
            this.txtIntLog.TabIndex = 24;
            this.txtIntLog.TabStop = false;
            // 
            // lblDebugger
            // 
            this.lblDebugger.AutoSize = true;
            this.lblDebugger.Location = new System.Drawing.Point(417, 470);
            this.lblDebugger.Name = "lblDebugger";
            this.lblDebugger.Size = new System.Drawing.Size(54, 13);
            this.lblDebugger.TabIndex = 26;
            this.lblDebugger.Text = "Debugger";
            // 
            // lstProcesses
            // 
            this.lstProcesses.FormattingEnabled = true;
            this.lstProcesses.Location = new System.Drawing.Point(12, 502);
            this.lstProcesses.Name = "lstProcesses";
            this.lstProcesses.Size = new System.Drawing.Size(248, 277);
            this.lstProcesses.TabIndex = 27;
            this.lstProcesses.SelectedIndexChanged += new System.EventHandler(this.lstProcesses_SelectedIndexChanged);
            // 
            // lblChild
            // 
            this.lblChild.AutoSize = true;
            this.lblChild.Location = new System.Drawing.Point(9, 487);
            this.lblChild.Name = "lblChild";
            this.lblChild.Size = new System.Drawing.Size(82, 13);
            this.lblChild.TabIndex = 28;
            this.lblChild.Text = "Child Processes";
            // 
            // lblDumpSize
            // 
            this.lblDumpSize.AutoSize = true;
            this.lblDumpSize.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblDumpSize.Location = new System.Drawing.Point(279, 487);
            this.lblDumpSize.Name = "lblDumpSize";
            this.lblDumpSize.Size = new System.Drawing.Size(67, 13);
            this.lblDumpSize.TabIndex = 29;
            this.lblDumpSize.Text = "Dump Size";
            // 
            // txtDumpSize
            // 
            this.txtDumpSize.AutoSize = true;
            this.txtDumpSize.Location = new System.Drawing.Point(279, 502);
            this.txtDumpSize.Name = "txtDumpSize";
            this.txtDumpSize.Size = new System.Drawing.Size(0, 13);
            this.txtDumpSize.TabIndex = 30;
            // 
            // txtGIFPackets
            // 
            this.txtGIFPackets.AutoSize = true;
            this.txtGIFPackets.Location = new System.Drawing.Point(279, 539);
            this.txtGIFPackets.Name = "txtGIFPackets";
            this.txtGIFPackets.Size = new System.Drawing.Size(0, 13);
            this.txtGIFPackets.TabIndex = 33;
            // 
            // lblGIFPackets
            // 
            this.lblGIFPackets.AutoSize = true;
            this.lblGIFPackets.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblGIFPackets.Location = new System.Drawing.Point(279, 523);
            this.lblGIFPackets.Name = "lblGIFPackets";
            this.lblGIFPackets.Size = new System.Drawing.Size(110, 13);
            this.lblGIFPackets.TabIndex = 32;
            this.lblGIFPackets.Text = "Total GIF Packets";
            // 
            // txtPath1
            // 
            this.txtPath1.AutoSize = true;
            this.txtPath1.Location = new System.Drawing.Point(279, 578);
            this.txtPath1.Name = "txtPath1";
            this.txtPath1.Size = new System.Drawing.Size(0, 13);
            this.txtPath1.TabIndex = 35;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(279, 560);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(114, 13);
            this.label2.TabIndex = 34;
            this.label2.Text = "Path1 GIF Packets";
            // 
            // txtPath2
            // 
            this.txtPath2.AutoSize = true;
            this.txtPath2.Location = new System.Drawing.Point(279, 618);
            this.txtPath2.Name = "txtPath2";
            this.txtPath2.Size = new System.Drawing.Size(0, 13);
            this.txtPath2.TabIndex = 37;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(279, 601);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(114, 13);
            this.label3.TabIndex = 36;
            this.label3.Text = "Path2 GIF Packets";
            // 
            // txtPath3
            // 
            this.txtPath3.AutoSize = true;
            this.txtPath3.Location = new System.Drawing.Point(279, 653);
            this.txtPath3.Name = "txtPath3";
            this.txtPath3.Size = new System.Drawing.Size(0, 13);
            this.txtPath3.TabIndex = 39;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(279, 635);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(114, 13);
            this.label5.TabIndex = 38;
            this.label5.Text = "Path3 GIF Packets";
            // 
            // txtVSync
            // 
            this.txtVSync.AutoSize = true;
            this.txtVSync.Location = new System.Drawing.Point(279, 690);
            this.txtVSync.Name = "txtVSync";
            this.txtVSync.Size = new System.Drawing.Size(0, 13);
            this.txtVSync.TabIndex = 41;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(279, 671);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(91, 13);
            this.label4.TabIndex = 40;
            this.label4.Text = "Vsync Packets";
            // 
            // txtReadFifo
            // 
            this.txtReadFifo.AutoSize = true;
            this.txtReadFifo.Location = new System.Drawing.Point(279, 724);
            this.txtReadFifo.Name = "txtReadFifo";
            this.txtReadFifo.Size = new System.Drawing.Size(0, 13);
            this.txtReadFifo.TabIndex = 43;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label7.Location = new System.Drawing.Point(279, 709);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(114, 13);
            this.label7.TabIndex = 42;
            this.label7.Text = "ReadFIFO Packets";
            // 
            // txtRegisters
            // 
            this.txtRegisters.AutoSize = true;
            this.txtRegisters.Location = new System.Drawing.Point(279, 764);
            this.txtRegisters.Name = "txtRegisters";
            this.txtRegisters.Size = new System.Drawing.Size(0, 13);
            this.txtRegisters.TabIndex = 45;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(279, 744);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(110, 13);
            this.label6.TabIndex = 44;
            this.label6.Text = "Registers Packets";
            // 
            // chkDebugMode
            // 
            this.chkDebugMode.AutoSize = true;
            this.chkDebugMode.Enabled = false;
            this.chkDebugMode.Location = new System.Drawing.Point(481, 487);
            this.chkDebugMode.Name = "chkDebugMode";
            this.chkDebugMode.Size = new System.Drawing.Size(88, 17);
            this.chkDebugMode.TabIndex = 46;
            this.chkDebugMode.Text = "Debug Mode";
            this.chkDebugMode.UseVisualStyleBackColor = true;
            this.chkDebugMode.CheckedChanged += new System.EventHandler(this.chkDebugMode_CheckedChanged);
            // 
            // lblGif
            // 
            this.lblGif.AutoSize = true;
            this.lblGif.Enabled = false;
            this.lblGif.Location = new System.Drawing.Point(427, 520);
            this.lblGif.Name = "lblGif";
            this.lblGif.Size = new System.Drawing.Size(66, 13);
            this.lblGif.TabIndex = 48;
            this.lblGif.Text = "GIF Packets";
            // 
            // btnStep
            // 
            this.btnStep.Enabled = false;
            this.btnStep.Location = new System.Drawing.Point(629, 533);
            this.btnStep.Name = "btnStep";
            this.btnStep.Size = new System.Drawing.Size(108, 40);
            this.btnStep.TabIndex = 49;
            this.btnStep.TabStop = false;
            this.btnStep.Text = "Step";
            this.btnStep.UseVisualStyleBackColor = true;
            this.btnStep.Click += new System.EventHandler(this.btnStep_Click);
            // 
            // btnRunToSelection
            // 
            this.btnRunToSelection.Enabled = false;
            this.btnRunToSelection.Location = new System.Drawing.Point(629, 579);
            this.btnRunToSelection.Name = "btnRunToSelection";
            this.btnRunToSelection.Size = new System.Drawing.Size(108, 40);
            this.btnRunToSelection.TabIndex = 50;
            this.btnRunToSelection.TabStop = false;
            this.btnRunToSelection.Text = "Run To Selection";
            this.btnRunToSelection.UseVisualStyleBackColor = true;
            this.btnRunToSelection.Click += new System.EventHandler(this.btnRunToSelection_Click);
            // 
            // treTreeView
            // 
            this.treTreeView.Enabled = false;
            this.treTreeView.Location = new System.Drawing.Point(420, 539);
            this.treTreeView.Name = "treTreeView";
            this.treTreeView.Size = new System.Drawing.Size(200, 240);
            this.treTreeView.TabIndex = 51;
            // 
            // GSDumpGUI
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(988, 790);
            this.Controls.Add(this.treTreeView);
            this.Controls.Add(this.btnRunToSelection);
            this.Controls.Add(this.btnStep);
            this.Controls.Add(this.lblGif);
            this.Controls.Add(this.chkDebugMode);
            this.Controls.Add(this.txtRegisters);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.txtReadFifo);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.txtVSync);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.txtPath3);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.txtPath2);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.txtPath1);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.txtGIFPackets);
            this.Controls.Add(this.lblGIFPackets);
            this.Controls.Add(this.txtDumpSize);
            this.Controls.Add(this.lblDumpSize);
            this.Controls.Add(this.lstProcesses);
            this.Controls.Add(this.lblChild);
            this.Controls.Add(this.lblDebugger);
            this.Controls.Add(this.lblInternalLog);
            this.Controls.Add(this.txtIntLog);
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
        private System.Windows.Forms.Label lblInternalLog;
        private System.Windows.Forms.TextBox txtIntLog;
        private System.Windows.Forms.Label lblDebugger;
        private System.Windows.Forms.Label lblChild;
        public System.Windows.Forms.ListBox lstProcesses;
        private System.Windows.Forms.Label lblDumpSize;
        public System.Windows.Forms.Label txtDumpSize;
        public System.Windows.Forms.Label txtGIFPackets;
        private System.Windows.Forms.Label lblGIFPackets;
        public System.Windows.Forms.Label txtPath1;
        private System.Windows.Forms.Label label2;
        public System.Windows.Forms.Label txtPath2;
        private System.Windows.Forms.Label label3;
        public System.Windows.Forms.Label txtPath3;
        private System.Windows.Forms.Label label5;
        public System.Windows.Forms.Label txtVSync;
        private System.Windows.Forms.Label label4;
        public System.Windows.Forms.Label txtReadFifo;
        private System.Windows.Forms.Label label7;
        public System.Windows.Forms.Label txtRegisters;
        private System.Windows.Forms.Label label6;
        public System.Windows.Forms.CheckBox chkDebugMode;
        public System.Windows.Forms.TreeView treTreeView;
        public System.Windows.Forms.Label lblGif;
        public System.Windows.Forms.Button btnStep;
        public System.Windows.Forms.Button btnRunToSelection;
    }
}

