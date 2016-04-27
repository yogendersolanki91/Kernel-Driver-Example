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
using System.Windows.Forms;
using BerkeleyDb;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;

namespace overview
{
  public partial class MainFrm: Form
  {
    public MainFrm() {
      InitializeComponent();
      homeDir = Path.GetDirectoryName(Application.ExecutablePath);
      homeDir = Path.Combine(homeDir, "home");
    }

    UTF8Encoding utf8 = new UTF8Encoding();

    int AppCompare(Db db, ref DbEntry appData, ref DbEntry dbData) {
      string appStr = utf8.GetString(appData.Buffer, 0, appData.Size);
      string dbStr = utf8.GetString(dbData.Buffer, 0, dbData.Size);
      return string.Compare(appStr, dbStr);
    }

    string homeDir;
    const string myDb = "MyFile.bdb";

    // for use with Addrecord and GetNextRecord
    MemoryStream keyStream = new MemoryStream();
    MemoryStream dataStream = new MemoryStream();
    BinaryFormatter formatter = new BinaryFormatter(); 

    private void AddRecord(DbBTree btree, Txn txn, Customer value, TextWriter writer) {
      // use standard .NET serialization, with the binary formatter
      keyStream.Position = 0;
      formatter.Serialize(keyStream, value.Name);
      DbEntry key = DbEntry.InOut(keyStream.GetBuffer(), 0, (int)keyStream.Position);
      dataStream.Position = 0;
      formatter.Serialize(dataStream, value);
      DbEntry data = DbEntry.InOut(dataStream.GetBuffer(), 0, (int)dataStream.Position);
      // calling PutNew means we don't want to overwrite an existing record
      WriteStatus status = btree.PutNew(txn, ref key, ref data);
      // if we tried to insert a duplicate, let's report it
      if (status == WriteStatus.KeyExist)
        writer.WriteLine("Duplicate record: " + value.Name);
    }

    private bool GetNextRecord(DbBTreeCursor cursor, ref Customer cust) {
      ReadStatus status;
      // If we need to call SetLength further down, and that causes its value to
      // increase, then it will clear all buffer content between the old and the
      // new length, overwriting some of our data. So we set it to the maximum here.
      keyStream.SetLength(keyStream.Capacity);
      dataStream.SetLength(dataStream.Capacity);
      // we base our DbEntry instances on the serialization stream buffers
      DbEntry key = DbEntry.Out(keyStream.GetBuffer());
      DbEntry data = DbEntry.Out(dataStream.GetBuffer());
      do {
        status = cursor.Get(ref key, ref data, DbFileCursor.GetMode.Next, DbFileCursor.ReadFlags.None);
        switch (status) {
          case ReadStatus.NotFound: return false;
          case ReadStatus.KeyEmpty: continue;  // skip deleted records
          case ReadStatus.BufferSmall:
            if (key.Buffer.Length < key.Size) {
              keyStream.SetLength(key.Size);
              key = DbEntry.Out(keyStream.GetBuffer());
            }
            if (data.Buffer.Length < data.Size) {
              dataStream.SetLength(data.Size);
              data = DbEntry.Out(dataStream.GetBuffer());
            }
            continue;
          case ReadStatus.Success:
            dataStream.Position = 0;
            dataStream.SetLength(data.Size);
            cust = (Customer)formatter.Deserialize(dataStream);
            return true;
          default:
            return false;
        }
      } while (true);
    }

    private void ClearDisplay() {
      dataGridView.DataSource = null;
      errBox.Text = "";
      msgBox.Text = "";
    }

    private void loadBtn_Click(object sender, EventArgs e) {
      ClearDisplay();

      MemoryStream errStream = new MemoryStream();
      MemoryStream msgStream = new MemoryStream();
      TextWriter errWriter = new StreamWriter(errStream);
      TextWriter msgWriter = new StreamWriter(msgStream);

      Db db = null;
      Txn txn = null;
      Env env = null;
      try {
        env = new Env(EnvCreateFlags.None);
        // configure for error and message reporting
        env.ErrorStream = errStream;
        env.ErrorPrefix = Path.GetFileName(Application.ExecutablePath);
        env.MessageStream = msgStream;

        // initialize environment for locking, logging, memory pool and transactions
        Env.OpenFlags envFlags =
          Env.OpenFlags.Create |
          Env.OpenFlags.InitLock |
          Env.OpenFlags.InitLog |
          Env.OpenFlags.InitMPool |
          Env.OpenFlags.InitTxn |
          Env.OpenFlags.Recover;
        env.Open(homeDir, envFlags, 0);

        // create, configure and open database under a transaction
        txn = env.TxnBegin(null, Txn.BeginFlags.None);
        db = env.CreateDatabase(DbCreateFlags.None);
        // set the BTree comparison function
        db.BTreeCompare = AppCompare;
        // error and message reporting already configured on environment
        // db.ErrorStream = errStream;
        // db.ErrorPrefix = Path.GetFileName(Application.ExecutablePath);
        // db.MessageStream = msgStream;
        DbBTree btree = (DbBTree)db.Open(
          txn, myDb, null, DbType.BTree, Db.OpenFlags.Create, 0);
        txn.Commit(Txn.CommitMode.None);

        // create a sequence named '###sequence1' under a transaction
        txn = env.TxnBegin(null, Txn.BeginFlags.None);
        Sequence seq = btree.CreateSequence();
        byte[] seqName = utf8.GetBytes("###sequence1");
        DbEntry seqNameEntry = DbEntry.InOut(seqName);
        seq.Open(txn, ref seqNameEntry, Sequence.OpenFlags.Create);
        seq.Close();
        txn.Commit(Txn.CommitMode.None);

        // open sequence again, retrieve values and then remove the sequence
        seq = btree.CreateSequence();
        txn = env.TxnBegin(null, Txn.BeginFlags.None);
        // seq.CacheSize = 1000;  // cannot use transactions when CacheSize > 0
        seq.Open(txn, ref seqNameEntry, Sequence.OpenFlags.None);
        Int64 seqVal;
        for (int indx = 0; indx < 500; indx++)
          seqVal = seq.Get(txn, 1, Sequence.ReadFlags.None);
        seq.Remove(txn, Sequence.RemoveFlags.None);
        txn.Commit(Txn.CommitMode.None);

        // add a few records under a transaction
        Customer cust;
        txn = env.TxnBegin(null, Txn.BeginFlags.None);

        cust = new Customer("John Doe", "122 Yonge Street", "Toronto", "ON", "M5R 5T9", DateTime.Parse("Dec 22, 1965"));
        AddRecord(btree, txn, cust, errWriter);

        cust = new Customer("Jane Doby", "23 Bloor Street", "Oshawa", "ON", "L1H 2K9", DateTime.Parse("Jun 1, 1962"));
        AddRecord(btree, txn, cust, errWriter);
        
        cust = new Customer("Rusty Nail", "77 Bond Street", "Markham", "ON", "L3T 7Y8", DateTime.Parse("Sep 9, 1915"));
        AddRecord(btree, txn, cust, errWriter);
        
        cust = new Customer("Rosy Cheek", "11 Adelaide Street", "Whitby", "ON", "K3P 4H4", DateTime.Parse("Jan 2, 1980"));
        AddRecord(btree, txn, cust, errWriter);

        // this last one is a duplicate of the first record
        cust = new Customer("John Doe", "1459 King Street", "Toronto", "ON", "N8N 2L0", DateTime.Parse("Apr 14, 1973"));
        AddRecord(btree, txn, cust, errWriter);

        txn.Commit(Txn.CommitMode.None);

        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine("K E Y   S T A T I S T I C S");
        msgWriter.WriteLine("==================================================");

        // get a key range, under a transaction
        txn = env.TxnBegin(null, Txn.BeginFlags.None);
        keyStream.Position = 0;
        formatter.Serialize(keyStream, "John Doe");
        DbEntry key = DbEntry.InOut(keyStream.GetBuffer(), 0, (int)keyStream.Position);
        DbBTree.KeyRange keyRange = btree.GetKeyRange(txn, ref key);
        txn.Commit(Txn.CommitMode.None);
        msgWriter.WriteLine();
        string msg = "KeyRange for 'John Doe': Less = {0}, Equal = {1}, Greater = {2}";
        msgWriter.WriteLine(string.Format(msg, keyRange.Less, keyRange.Equal, keyRange.Greater)); 
        msgWriter.WriteLine();

        // retrieve some database statistics
        TxnStats stats = env.GetTxnStats(StatFlags.None);
        DbBTree.Stats btstats = btree.GetStats(null, DbFile.StatFlags.None);
        CacheFileStats[] cfStats = env.GetCacheFileStats(StatFlags.None);
        // we can also have them written to the message stream
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine("T R A N S A C T I O N   S T A T I S T I C S");
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine();
        msgWriter.Flush();
        env.PrintTxnStats(StatPrintFlags.None);
        msgWriter.WriteLine();
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine("L O G   S T A T I S T I C S");
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine();
        msgWriter.Flush();
        env.PrintLogStats(StatPrintFlags.All);
        msgWriter.WriteLine();
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine("L O C K   S T A T I S T I C S");
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine();
        msgWriter.Flush();
        env.PrintLogStats(StatPrintFlags.All);
        msgWriter.WriteLine();
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine("C A C H E   S T A T I S T I C S");
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine();
        msgWriter.Flush();
        env.PrintCacheStats(CacheStatPrintFlags.All);
        msgWriter.WriteLine();
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine("B T R E E   S T A T I S T I C S");
        msgWriter.WriteLine("==================================================");
        msgWriter.WriteLine();
        msgWriter.Flush();
        btree.PrintStats(DbFile.StatPrintFlags.All);
      }
      catch (BdbException) {
        if (txn != null && !txn.IsDisposed)
          txn.Abort();
        // errors are reported through error stream already
        return;
      }
      catch (Exception ex) {
        if (txn != null && !txn.IsDisposed)
          txn.Abort();
        errWriter.WriteLine(ex.ToString());
        return;
      }
      finally {
        if (db != null)
          db.Close();
        if (env != null)
          env.Close();
        msgWriter.Flush();
        string msgText = utf8.GetString(msgStream.GetBuffer());
        if (msgText != "") {
          msgBox.Text = msgText;
        }
        msgWriter.Close();
        errWriter.Flush();
        string errText = utf8.GetString(errStream.GetBuffer());
        if (errText != "") {
          errBox.Text = errText;
          tabControl.SelectTab("errorPage");
        }
        errWriter.Close();
      }
    }

    private void verifyBtn_Click(object sender, EventArgs e) {
      ClearDisplay();

      MemoryStream errStream = new MemoryStream();
      FileStream dumpStream = new FileStream("MyFile.vrf", FileMode.Create, FileAccess.Write);
      UTF8Encoding utf8 = new UTF8Encoding();

      Db db = new Db(DbCreateFlags.None);
      try {
        string errText = "";
        try {
          db.BTreeCompare = AppCompare;
          db.ErrorStream = errStream;
          db.Verify(Path.Combine(homeDir, myDb), null, dumpStream, Db.VerifyFlags.DbSalvage);
        }
        catch (BdbException ex) {
          errText = ex.Message;
        }
        if (errText != "")
          errText += Environment.NewLine;
        errText += utf8.GetString(errStream.GetBuffer());
        if (errText != "") {
          errBox.Text = errText;
          tabControl.SelectTab("errorPage");
        }
      }
      finally {
        db.Close();
        dumpStream.Close();
        errStream.Close();
      }
    }

    private void removeBtn_Click(object sender, EventArgs e) {
      ClearDisplay();

      Env env = new Env(EnvCreateFlags.None);
      try {
        Env.OpenFlags envFlags =
          Env.OpenFlags.Create |
          Env.OpenFlags.InitLock |
          Env.OpenFlags.InitLog |
          Env.OpenFlags.InitMPool |
          Env.OpenFlags.InitTxn |
          Env.OpenFlags.Recover;
        env.Open(homeDir, envFlags, 0);
        // remove existing database file
        Txn txn = env.TxnBegin(null, Txn.BeginFlags.None);
        env.DbRemove(txn, myDb, null, Env.WriteFlags.None);
        txn.Commit(Txn.CommitMode.None);
        env.Close();
        // remove existing environment
        env = new Env(EnvCreateFlags.None);
        env.Remove(homeDir, Env.RemoveFlags.None);
      }
      catch (Exception ex) {
        errBox.Text = ex.ToString();
        tabControl.SelectTab("errorPage");
      }
      finally {
        env.Close();
      }
    }

    private void viewBtn_Click(object sender, EventArgs e) {
      ClearDisplay();

      // open database and read records back - the database is transactional
      Env env = new Env(EnvCreateFlags.None);
      try {
        DbBTree btree;
        Env.OpenFlags envFlags =
          Env.OpenFlags.Create |
          Env.OpenFlags.InitLock |
          Env.OpenFlags.InitLog |
          Env.OpenFlags.InitMPool |
          Env.OpenFlags.InitTxn |
          Env.OpenFlags.Recover;
        env.Open(homeDir, envFlags, 0);
        Db db = env.CreateDatabase(DbCreateFlags.None);
        Txn txn = env.TxnBegin(null, Txn.BeginFlags.None);
        DbFile tmpDb =
          db.Open(txn, Path.Combine(homeDir, myDb), null, DbType.Unknown, Db.OpenFlags.None, 0);
        txn.Commit(Txn.CommitMode.None);
        if (tmpDb.DbType == DbType.BTree)
          btree = (DbBTree)tmpDb;
        else
          throw new ApplicationException("Unexpected database type.");

        List<Customer> custList = new List<Customer>();
        // DbBTreeCursor implements IDisposable - will be closed through "using"
        using (DbBTreeCursor cursor = btree.OpenCursor(null, DbFileCursor.CreateFlags.None)) {
          Customer cust = null;
          while (GetNextRecord(cursor, ref cust))
            custList.Add(cust);
        }
        db.Close();

        DataGridViewColumn nameCol = new DataGridViewTextBoxColumn();
        nameCol.Name = "NameCol";
        nameCol.HeaderText = "Name";
        nameCol.DataPropertyName = "Name";
        dataGridView.Columns.Add(nameCol);
        dataGridView.DataSource = custList;
        tabControl.SelectTab("dataPage");
      }
      catch (Exception ex) {
        errBox.Text = ex.ToString();
        tabControl.SelectTab("errorPage");
      }
      finally {
        env.Close();
      }
    }
  }

  // the class whose instances we want to store in the database
  [Serializable]
  public class Customer
  {
    string name;

    public string Name {
      get { return name; }
    }

    string street;

    public string Street {
      get { return street; }
    }

    string city;

    public string City {
      get { return city; }
    }

    string province;

    public string Province {
      get { return province; }
    }

    string postalCode;

    public string PostalCode {
      get { return postalCode; }
    }

    DateTime birthDate;

    public DateTime BirthDate {
      get { return birthDate; }
    }

    public Customer(
      string name,
      string street,
      string city,
      string province,
      string postalCode,
      DateTime birthDate)
    {
      this.name = name;
      this.street = street;
      this.city = city;
      this.province = province;
      this.postalCode = postalCode;
      this.birthDate = birthDate;
    }
  }
}