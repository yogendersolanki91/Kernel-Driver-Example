namespace overview
{
  partial class MainFrm
  {
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing) {
      if (disposing && (components != null)) {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent() {
      this.loadBtn = new System.Windows.Forms.Button();
      this.verifyBtn = new System.Windows.Forms.Button();
      this.tabControl = new System.Windows.Forms.TabControl();
      this.dataPage = new System.Windows.Forms.TabPage();
      this.dataGridView = new System.Windows.Forms.DataGridView();
      this.errorPage = new System.Windows.Forms.TabPage();
      this.errBox = new System.Windows.Forms.TextBox();
      this.messagePage = new System.Windows.Forms.TabPage();
      this.msgBox = new System.Windows.Forms.TextBox();
      this.removeBtn = new System.Windows.Forms.Button();
      this.viewBtn = new System.Windows.Forms.Button();
      this.tabControl.SuspendLayout();
      this.dataPage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.dataGridView)).BeginInit();
      this.errorPage.SuspendLayout();
      this.messagePage.SuspendLayout();
      this.SuspendLayout();
      // 
      // loadBtn
      // 
      this.loadBtn.Location = new System.Drawing.Point(12, 12);
      this.loadBtn.Name = "loadBtn";
      this.loadBtn.Size = new System.Drawing.Size(97, 23);
      this.loadBtn.TabIndex = 0;
      this.loadBtn.Text = "Setup && Load";
      this.loadBtn.Click += new System.EventHandler(this.loadBtn_Click);
      // 
      // verifyBtn
      // 
      this.verifyBtn.Location = new System.Drawing.Point(231, 12);
      this.verifyBtn.Name = "verifyBtn";
      this.verifyBtn.Size = new System.Drawing.Size(75, 23);
      this.verifyBtn.TabIndex = 1;
      this.verifyBtn.Text = "Verify";
      this.verifyBtn.Click += new System.EventHandler(this.verifyBtn_Click);
      // 
      // tabControl
      // 
      this.tabControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                  | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.tabControl.Controls.Add(this.dataPage);
      this.tabControl.Controls.Add(this.errorPage);
      this.tabControl.Controls.Add(this.messagePage);
      this.tabControl.Location = new System.Drawing.Point(12, 41);
      this.tabControl.Name = "tabControl";
      this.tabControl.SelectedIndex = 0;
      this.tabControl.Size = new System.Drawing.Size(459, 289);
      this.tabControl.TabIndex = 5;
      // 
      // dataPage
      // 
      this.dataPage.Controls.Add(this.dataGridView);
      this.dataPage.Location = new System.Drawing.Point(4, 22);
      this.dataPage.Name = "dataPage";
      this.dataPage.Padding = new System.Windows.Forms.Padding(3);
      this.dataPage.Size = new System.Drawing.Size(451, 263);
      this.dataPage.TabIndex = 0;
      this.dataPage.Text = "Data";
      this.dataPage.UseVisualStyleBackColor = true;
      // 
      // dataGridView
      // 
      this.dataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
      this.dataGridView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.dataGridView.Location = new System.Drawing.Point(3, 3);
      this.dataGridView.Name = "dataGridView";
      this.dataGridView.ReadOnly = true;
      this.dataGridView.Size = new System.Drawing.Size(445, 257);
      this.dataGridView.TabIndex = 5;
      // 
      // errorPage
      // 
      this.errorPage.Controls.Add(this.errBox);
      this.errorPage.Location = new System.Drawing.Point(4, 22);
      this.errorPage.Name = "errorPage";
      this.errorPage.Padding = new System.Windows.Forms.Padding(3);
      this.errorPage.Size = new System.Drawing.Size(451, 263);
      this.errorPage.TabIndex = 1;
      this.errorPage.Text = "Errors";
      this.errorPage.UseVisualStyleBackColor = true;
      // 
      // errBox
      // 
      this.errBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.errBox.Location = new System.Drawing.Point(3, 3);
      this.errBox.Multiline = true;
      this.errBox.Name = "errBox";
      this.errBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
      this.errBox.Size = new System.Drawing.Size(445, 257);
      this.errBox.TabIndex = 0;
      // 
      // messagePage
      // 
      this.messagePage.Controls.Add(this.msgBox);
      this.messagePage.Location = new System.Drawing.Point(4, 22);
      this.messagePage.Name = "messagePage";
      this.messagePage.Padding = new System.Windows.Forms.Padding(3);
      this.messagePage.Size = new System.Drawing.Size(451, 263);
      this.messagePage.TabIndex = 2;
      this.messagePage.Text = "Messages";
      this.messagePage.UseVisualStyleBackColor = true;
      // 
      // msgBox
      // 
      this.msgBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.msgBox.Location = new System.Drawing.Point(3, 3);
      this.msgBox.Multiline = true;
      this.msgBox.Name = "msgBox";
      this.msgBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
      this.msgBox.Size = new System.Drawing.Size(445, 257);
      this.msgBox.TabIndex = 0;
      // 
      // removeBtn
      // 
      this.removeBtn.Location = new System.Drawing.Point(312, 12);
      this.removeBtn.Name = "removeBtn";
      this.removeBtn.Size = new System.Drawing.Size(75, 23);
      this.removeBtn.TabIndex = 6;
      this.removeBtn.Text = "Remove";
      this.removeBtn.UseVisualStyleBackColor = true;
      this.removeBtn.Click += new System.EventHandler(this.removeBtn_Click);
      // 
      // viewBtn
      // 
      this.viewBtn.Location = new System.Drawing.Point(115, 12);
      this.viewBtn.Name = "viewBtn";
      this.viewBtn.Size = new System.Drawing.Size(75, 23);
      this.viewBtn.TabIndex = 7;
      this.viewBtn.Text = "View";
      this.viewBtn.UseVisualStyleBackColor = true;
      this.viewBtn.Click += new System.EventHandler(this.viewBtn_Click);
      // 
      // MainFrm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(483, 342);
      this.Controls.Add(this.viewBtn);
      this.Controls.Add(this.removeBtn);
      this.Controls.Add(this.tabControl);
      this.Controls.Add(this.verifyBtn);
      this.Controls.Add(this.loadBtn);
      this.Name = "MainFrm";
      this.Text = "Simple Demo";
      this.tabControl.ResumeLayout(false);
      this.dataPage.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.dataGridView)).EndInit();
      this.errorPage.ResumeLayout(false);
      this.errorPage.PerformLayout();
      this.messagePage.ResumeLayout(false);
      this.messagePage.PerformLayout();
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Button loadBtn;
    private System.Windows.Forms.Button verifyBtn;
    private System.Windows.Forms.TabControl tabControl;
    private System.Windows.Forms.TabPage dataPage;
    private System.Windows.Forms.DataGridView dataGridView;
    private System.Windows.Forms.TabPage errorPage;
    private System.Windows.Forms.TabPage messagePage;
    private System.Windows.Forms.Button removeBtn;
    private System.Windows.Forms.TextBox errBox;
    private System.Windows.Forms.TextBox msgBox;
    private System.Windows.Forms.Button viewBtn;
  }
}

