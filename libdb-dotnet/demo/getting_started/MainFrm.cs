/*
 * This software is licensed according to the "Modified BSD License",
 * where the following substitutions are made in the license template:
 * <OWNER> = Karl Waclawek
 * <ORGANIZATION> = Karl Waclawek
 * <YEAR> = 2005, 2006
 * It can be obtained from http://opensource.org/licenses/bsd-license.html.
 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Windows.Forms;
using BerkeleyDb;
using Kds.Serialization.Buffer;

namespace getting_started
{
  public partial class MainFrm: Form
  {
    public MainFrm() {
      InitializeComponent();
    }

    private FileStream errStream;
    private byte[] keyBuffer = new byte[1024];
    private byte[] dataBuffer = new byte[4096];

    private void InitVendorFromRegex(Vendor vendor, GroupCollection groups) {
      vendor.Name = groups[1].Value;
      vendor.Street = groups[2].Value;
      vendor.City = groups[3].Value;
      vendor.State = groups[4].Value;
      vendor.ZipCode = groups[5].Value;
      vendor.PhoneNumber = groups[6].Value;
      SalesRep rep = new SalesRep();
      rep.Name = groups[7].Value;
      rep.PhoneNumber = groups[8].Value;
      vendor.SalesRep = rep;
    }

    private void InitStockItemFromRegex(StockItem item, GroupCollection groups) {
      item.Name = groups[1].Value;
      item.Sku = groups[2].Value;
      item.Price = float.Parse(groups[3].Value);
      item.Quantity = int.Parse(groups[4].Value);
      item.Category = groups[5].Value;
      item.Vendor = groups[6].Value;
    }

    // load databases from files vendors.txt and inventory.txt
    private void LoadBtn_Click(object sender, EventArgs e) {
      if (errStream != null)
        errStream.Close();
      Databases dbs = Databases.Instance;
      string dbPath = Path.Combine(Application.StartupPath, "Db");
      string appName = Path.GetFileName(Application.ExecutablePath);
      errStream = File.Open(Path.Combine(dbPath, "errors.txt"), FileMode.OpenOrCreate, FileAccess.Write);
      dbs.Open(dbPath, appName, errStream);

      // load vendors
      StreamReader sr = new StreamReader(Path.Combine(dbPath, "vendors.txt"));
      Regex rgx = new Regex("(.*)#(.*)#(.*)#(.*)#(.*)#(.*)#(.*)#(.*)", RegexOptions.Compiled);
      Vendor vendor = new Vendor();
      string line;
      DbEntry keyEntry;
      DbEntry dataEntry;

      while ((line = sr.ReadLine()) != null && line.Length > 0) {
        Match match = rgx.Match(line);
        GroupCollection groups = match.Groups;
        if (groups.Count < 9)
          throw new ApplicationException("Input error on vendors.txt");
        InitVendorFromRegex(vendor, groups);
        keyEntry = dbs.Fmt.ToDbEntry<string>(vendor.Name);
        dataEntry = dbs.Fmt.ToDbEntry<Vendor>(vendor);
        WriteStatus status = dbs.VendorDb.Put(null, ref keyEntry, ref dataEntry);
        if (status != WriteStatus.Success)
          throw new ApplicationException("Put failed");
      }

      // load inventory
      sr = new StreamReader(Path.Combine(dbPath, "inventory.txt"));
      rgx = new Regex("(.*)#(.*)#(.*)#(.*)#(.*)#(.*)", RegexOptions.Compiled);
      StockItem item = new StockItem();

      int count = 0;
      while ((line = sr.ReadLine()) != null && line.Length > 0) {
        Match match = rgx.Match(line);
        GroupCollection groups = match.Groups;
        if (groups.Count < 7)
          throw new ApplicationException("Input error on inventory.txt");
        InitStockItemFromRegex(item, groups);
        keyEntry = dbs.Fmt.ToDbEntry<string>(item.Sku);
        dataEntry = dbs.Fmt.ToDbEntry<StockItem>(item);
        WriteStatus status = dbs.InventoryDb.Put(null, ref keyEntry, ref dataEntry);
        if (status != WriteStatus.Success)
          throw new Exception("Put failed");
        count++;
      }
    }

    private List<Vendor> ReadVendors() {
      Databases dbs = Databases.Instance;
      List<Vendor> vendors = new List<Vendor>();
      // using a foreach loop (which closes the cursor automatically)
      foreach (KeyDataPair entry in dbs.VendorDb.OpenCursor(null, DbFileCursor.CreateFlags.None)) {
        Vendor vendor = null;
        dbs.Fmt.FromDbEntry<Vendor>(ref vendor, ref entry.Data);
        vendors.Add(vendor);
      }

      vendorView.DataSource = null;
      DataGridViewColumn col = new DataGridViewTextBoxColumn();
      col.Name = "NameCol";
      col.HeaderText = "Name";
      col.DataPropertyName = "Name";
      vendorView.Columns.Add(col);
      col = new DataGridViewTextBoxColumn();
      col.Name = "SalesRepCol";
      col.HeaderText = "Sales Rep";
      col.DataPropertyName = "SalesRep";
      vendorView.Columns.Add(col);
      vendorView.DataSource = vendors;
      return vendors;
    }

    private List<StockItem> ReadStockItems(Vendor vendor) {
      Databases dbs = Databases.Instance;
      List<StockItem> items = new List<StockItem>();
      DbBTreeCursor cursor = dbs.VendorNameIdx.OpenCursor(null, DbFileCursor.CreateFlags.None);
      try {
        DbEntry secKeyEntry = dbs.Fmt.ToDbEntry<string>(vendor.Name);

#if false
        // same as below, but using standard cursor methods
        DbEntry keyEntry = DbEntry.Out(keyBuffer);
        DbEntry dataEntry = DbEntry.Out(dataBuffer);
        ReadStatus status;
        status = cursor.PGetAt(ref secKeyEntry, ref keyEntry, ref dataEntry,
          DbcFile.GetAtMode.Set, DbcFile.ReadFlags.None);
        // we are not checking for ReadStatus.BufferSmall, but maybe we should
        while (status == ReadStatus.Success) {
          StockItem item = null;
          index = dataEntry.Start;
          dbs.Formatter.Deserialize<StockItem>(ref item, dataEntry.Buffer, ref index);
          items.Add(item);
          status = cursor.PGet(ref secKeyEntry, ref keyEntry, ref dataEntry,
             DbcFile.GetMode.NextDup, DbcFile.ReadFlags.None);
        }
#endif
#if true
        // same as above, but using a foreach loop (which closes the cursor automatically)
        foreach (KeyDataPair entry in cursor.ItemsAt(ref secKeyEntry, DbFileCursor.ReadFlags.None)) {
          StockItem item = null;
          dbs.Fmt.FromDbEntry<StockItem>(ref item, ref entry.Data);
          items.Add(item);
        }
#endif

      }
      finally {
        cursor.Close();
      }

      inventoryView.DataSource = null;
      DataGridViewColumn col = new DataGridViewTextBoxColumn();
      col.Name = "NameCol";
      col.HeaderText = "Name";
      col.DataPropertyName = "Name";
      inventoryView.Columns.Add(col);
      inventoryView.DataSource = items;
      return items;
    }

    private void readBtn_Click(object sender, EventArgs e) {
      if (Databases.Instance.VendorDb == null)
        throw new ApplicationException("Databases closed.");
      List<Vendor> vendors = ReadVendors();
    }

    private void MainFrm_FormClosed(object sender, FormClosedEventArgs e) {
      Databases.Instance.Close();
    }

    private void vendorView_RowEnter(object sender, DataGridViewCellEventArgs e) {
      if (Databases.Instance.VendorDb == null)
        return;
      Vendor vendor = ((List<Vendor>)vendorView.DataSource)[e.RowIndex];
      ReadStockItems(vendor);
    }

    private void vendorView_CellFormatting(object sender, DataGridViewCellFormattingEventArgs e) {
      if (vendorView.Rows.Count <= e.RowIndex)
        return;
      Vendor vendor = (Vendor)vendorView.Rows[e.RowIndex].DataBoundItem;
      if (vendor == null)
        return;
      if (vendorView.Columns[e.ColumnIndex].Name == "SalesRepCol")
        e.Value = vendor.SalesRep.Name + " (" + vendor.SalesRep.PhoneNumber + ")";
      e.FormattingApplied = true;
    }
  }
}