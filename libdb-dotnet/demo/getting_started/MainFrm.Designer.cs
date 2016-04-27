namespace getting_started
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
      this.vendorView = new System.Windows.Forms.DataGridView();
      this.viewBtn = new System.Windows.Forms.Button();
      this.inventoryView = new System.Windows.Forms.DataGridView();
      this.label1 = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      ((System.ComponentModel.ISupportInitialize)(this.vendorView)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.inventoryView)).BeginInit();
      this.SuspendLayout();
      // 
      // loadBtn
      // 
      this.loadBtn.Location = new System.Drawing.Point(72, 12);
      this.loadBtn.Name = "loadBtn";
      this.loadBtn.Size = new System.Drawing.Size(112, 23);
      this.loadBtn.TabIndex = 0;
      this.loadBtn.Text = "Load Databases";
      this.loadBtn.Click += new System.EventHandler(this.LoadBtn_Click);
      // 
      // vendorView
      // 
      this.vendorView.AllowUserToOrderColumns = true;
      this.vendorView.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.vendorView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.AllCells;
      this.vendorView.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
      this.vendorView.Location = new System.Drawing.Point(12, 50);
      this.vendorView.Name = "vendorView";
      this.vendorView.ReadOnly = true;
      this.vendorView.Size = new System.Drawing.Size(733, 157);
      this.vendorView.TabIndex = 1;
      this.vendorView.Text = "dataGridView1";
      this.vendorView.RowEnter += new System.Windows.Forms.DataGridViewCellEventHandler(this.vendorView_RowEnter);
      this.vendorView.CellFormatting += new System.Windows.Forms.DataGridViewCellFormattingEventHandler(this.vendorView_CellFormatting);
      // 
      // viewBtn
      // 
      this.viewBtn.Location = new System.Drawing.Point(207, 12);
      this.viewBtn.Name = "viewBtn";
      this.viewBtn.Size = new System.Drawing.Size(112, 23);
      this.viewBtn.TabIndex = 2;
      this.viewBtn.Text = "View Databases";
      this.viewBtn.Click += new System.EventHandler(this.readBtn_Click);
      // 
      // inventoryView
      // 
      this.inventoryView.AllowUserToOrderColumns = true;
      this.inventoryView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                  | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.inventoryView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.AllCells;
      this.inventoryView.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
      this.inventoryView.Location = new System.Drawing.Point(12, 232);
      this.inventoryView.Name = "inventoryView";
      this.inventoryView.ReadOnly = true;
      this.inventoryView.Size = new System.Drawing.Size(733, 225);
      this.inventoryView.TabIndex = 3;
      this.inventoryView.Text = "dataGridView1";
      // 
      // label1
      // 
      this.label1.Location = new System.Drawing.Point(12, 35);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(50, 12);
      this.label1.TabIndex = 4;
      this.label1.Text = "Vendors";
      // 
      // label2
      // 
      this.label2.Location = new System.Drawing.Point(12, 217);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(94, 12);
      this.label2.TabIndex = 5;
      this.label2.Text = "Items for Vendor";
      // 
      // MainFrm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(757, 469);
      this.Controls.Add(this.label2);
      this.Controls.Add(this.label1);
      this.Controls.Add(this.inventoryView);
      this.Controls.Add(this.viewBtn);
      this.Controls.Add(this.vendorView);
      this.Controls.Add(this.loadBtn);
      this.Name = "MainFrm";
      this.Text = "Getting Started Demo";
      this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainFrm_FormClosed);
      ((System.ComponentModel.ISupportInitialize)(this.vendorView)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.inventoryView)).EndInit();
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Button loadBtn;
    private System.Windows.Forms.DataGridView vendorView;
    private System.Windows.Forms.Button viewBtn;
    private System.Windows.Forms.DataGridView inventoryView;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label2;
  }
}

