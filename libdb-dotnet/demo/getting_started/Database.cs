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
using System.Text;
using System.IO;
using BerkeleyDb;
using BerkeleyDb.Serialization;
using Kds.Serialization;
using Kds.Serialization.Buffer;

namespace getting_started
{
  class Databases
  {
    public const string vendorDbName = "vendor.db";
    public const string inventoryDbName = "inventory.db";
    public const string vendorIndexName = "vendor_name.idx";

    // will be initialized on first access to static field in a thread-safe way
    private static readonly Databases instance = new Databases();

    private string dbDir;
    private DbBTree vendorDb;
    private DbBTree inventoryDb;
    private DbBTree vendorNameIdx;

    // used for key gnerator call-backs, not thread-safe
    private BdbFormatter fmt;

    private Databases() {
      fmt = new BdbFormatter(3, 2048);
      new VendorField(fmt, true);
      new SalesRepField(fmt, true);
      new StockItemField(fmt, true);
    }

    // singleton
    public static Databases Instance {
      get { return instance; }
    }

    // get vendor name for secondary key
    private DbFile.KeyGenStatus GetVendorName(DbFile secondary, ref DbEntry key, ref DbEntry data, out DbEntry result) {
      string vendorName = "";
      // for skipping members we have to use the explicit code
      int vendorIndex = data.Start;
      fmt.InitDeserialization(data.Buffer, vendorIndex);
      if (!fmt.SkipMembers<StockItem>(2))
        throw new ApplicationException("Skip path invalid.");
      fmt.Deserialize<string>(ref vendorName);
      fmt.FinishDeserialization();  // not interested in end index

      // now we re-serialize the vendor name into the return buffer
      result = fmt.ToDbEntry<string>(vendorName);
      return DbFile.KeyGenStatus.Success;
    }

    // faster way of getting vendor name, uses "unsafe" techniques
    private unsafe DbRetVal GetVendorNameFast(DbFile secondary, ref DBT key, ref DBT data, ref DBT result) {
      // not implemented - would need "UnsafeBufferFormatter"
      return DbRetVal.DONOTINDEX;
    }

    private int VendorNameCompareFcn(Db db, ref DbEntry entry1, ref DbEntry entry2) {
      string vendorName1 = null;
      fmt.FromDbEntry<string>(ref vendorName1, ref entry1);
      string vendorName2 = null;
      fmt.FromDbEntry<string>(ref vendorName2, ref entry2);
      return string.Compare(vendorName1, vendorName2, StringComparison.InvariantCulture);
    }

    public void Open(string dbDir, string appName, Stream errStream) {
      this.dbDir = dbDir;
      Remove();

      // open vendor database
      Db db = new Db(DbCreateFlags.None);
      db.ErrorPrefix = appName;
      db.ErrorStream = errStream;
      vendorDb = (DbBTree)db.Open(null, VendorDbName, null, DbType.BTree, Db.OpenFlags.Create, 0);

      // open inventory database
      db = new Db(DbCreateFlags.None);
      db.ErrorPrefix = appName;
      db.ErrorStream = errStream;
      inventoryDb = (DbBTree)db.Open(null, InventoryDbName, null, DbType.BTree, Db.OpenFlags.Create, 0);

      // open & associate vendor name index
      db = new Db(DbCreateFlags.None);
      db.ErrorPrefix = appName;
      db.ErrorStream = errStream;
      db.BTreeCompare = VendorNameCompareFcn;
      db.SetFlags(DbFlags.Dup | DbFlags.DupSort);
      vendorNameIdx = (DbBTree)db.Open(null, VendorIndexName, null, DbType.BTree, Db.OpenFlags.Create, 0);
      inventoryDb.Associate(vendorNameIdx, GetVendorName, DbFile.AssociateFlags.Create);
      // unsafe {
      //   inventoryDb.Associate(vendorNameIdx, GetVendorNameFast, DbFile.AssociateFlags.Create);
      // }
    }

    public void Remove() {
      try {
        Db db = new Db(DbCreateFlags.None);
        db.Remove(VendorDbName, null);
      }
      catch { }
      try {
        Db db = new Db(DbCreateFlags.None);
        db.Remove(InventoryDbName, null);
      }
      catch { }
      try {
        Db db = new Db(DbCreateFlags.None);
        db.Remove(VendorIndexName, null);
      }
      catch { }
    }

    public void Close() {
      if (vendorDb != null) {
        vendorDb.GetDb().Close();
        vendorDb = null;
      }
      if (inventoryDb != null) {
        inventoryDb.GetDb().Close();
        inventoryDb = null;
      }
      if (vendorNameIdx != null) {
        vendorNameIdx.GetDb().Close();
        vendorNameIdx = null;
      }
    }

    public BdbFormatter Fmt {
      get { return fmt; }
    }

    public string VendorDbName {
      get { return Path.Combine(dbDir, vendorDbName); }
    }

    public DbBTree VendorDb {
      get { return vendorDb; }
    }

    public string InventoryDbName {
      get { return Path.Combine(dbDir, inventoryDbName); }
    }

    public DbBTree InventoryDb {
      get { return inventoryDb; }
    }

    public string VendorIndexName {
      get { return Path.Combine(dbDir, vendorIndexName); }
    }

    public DbBTree VendorNameIdx {
      get { return vendorNameIdx; }
    }
  }
}
