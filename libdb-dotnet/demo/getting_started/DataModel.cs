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
using System.Collections.ObjectModel;
using BerkeleyDb;
using Kds.Serialization;
using Kds.Serialization.Buffer;

namespace getting_started
{
  public class Vendor
  {
    internal string name;
    internal string street;
    internal string city;
    internal string state;
    internal string zipCode;
    internal string phoneNumber;
    internal SalesRep salesRep;

    public string Name {
      get { return name; }
      set { name = value; }
    }

    public string Street {
      get { return street; }
      set { street = value; }
    }

    public string City {
      get { return city; }
      set { city = value; }
    }

    public string State {
      get { return state; }
      set { state = value; }
    }

    public string ZipCode {
      get { return zipCode; }
      set { zipCode = value; }
    }

    public string PhoneNumber {
      get { return phoneNumber; }
      set { phoneNumber = value; }
    }

    public SalesRep SalesRep {
      get { return salesRep; }
      set { salesRep = value; }
    }
  }

  public class VendorField: ReferenceField<Vendor>
  {
    public VendorField(Formatter fmt, bool isDefault) : base(fmt, isDefault) { }

    protected override void SerializeValue(Vendor value) {
      Fmt.Serialize<string>(value.Name);
      Fmt.Serialize<string>(value.Street);
      Fmt.Serialize<string>(value.City);
      Fmt.Serialize<string>(value.State);
      Fmt.Serialize<string>(value.ZipCode);
      Fmt.Serialize<string>(value.PhoneNumber);
      Fmt.Serialize<SalesRep>(value.SalesRep);
    }

    protected override void DeserializeInstance(ref Vendor instance) {
      if (instance == null)
        instance = new Vendor();
    }

    protected override void DeserializeMembers(Vendor instance) {
      Fmt.Deserialize<string>(ref instance.name);
      Fmt.Deserialize<string>(ref instance.street);
      Fmt.Deserialize<string>(ref instance.city);
      Fmt.Deserialize<string>(ref instance.state);
      Fmt.Deserialize<string>(ref instance.zipCode);
      Fmt.Deserialize<string>(ref instance.phoneNumber);
      Fmt.Deserialize<SalesRep>(ref instance.salesRep);
    }

    protected override void SkipValue() {
      if (Fmt.Skip<string>())
      if (Fmt.Skip<string>())
      if (Fmt.Skip<string>())
      if (Fmt.Skip<string>())
      if (Fmt.Skip<string>())
      if (Fmt.Skip<string>())
      Fmt.Skip<SalesRep>();
    }
  }

  public class SalesRep
  {
    internal string name;
    internal string phoneNumber;

    public string Name {
      get { return name; }
      set { name = value; }
    }

    public string PhoneNumber {
      get { return phoneNumber; }
      set { phoneNumber = value; }
    }
  }

  public class SalesRepField: ReferenceField<SalesRep>
  {
    public SalesRepField(Formatter fmt, bool isDefault) : base(fmt, isDefault) { }

    protected override void SerializeValue(SalesRep value) {
      Fmt.Serialize<string>(value.Name);
      Fmt.Serialize<string>(value.PhoneNumber);
    }

    protected override void DeserializeInstance(ref SalesRep instance) {
      if (instance == null)
        instance = new SalesRep();
    }

    protected override void DeserializeMembers(SalesRep instance) {
      string val = null;  // when using a property we need an intermediate variable
      Fmt.Deserialize<string>(ref val);
      instance.Name = val;
      Fmt.Deserialize<string>(ref instance.phoneNumber);
    }

    protected override void SkipValue() {
      if (Fmt.Skip<string>())
      Fmt.Skip<string>();
    }
  }

  public class StockItem
  {
    internal string name;
    internal string category;
    internal string vendor;
    internal string sku;
    internal float? price;
    internal int? quantity;

    public string Name {
      get { return name; }
      set { name = value; }
    }

    public string Category {
      get { return category; }
      set { category = value; }
    }

    public string Vendor {
      get { return vendor; }
      set { vendor = value; }
    }

    public string Sku {
      get { return sku; }
      set { sku = value; }
    }

    public float? Price {
      get { return price; }
      set { price = value; }
    }

    public int? Quantity {
      get { return quantity; }
      set { quantity = value; }
    }
  }

  public class StockItemField: ReferenceField<StockItem>
  {
    public StockItemField(Formatter fmt, bool isDefault) : base(fmt, isDefault) { }

    protected override void SerializeValue(StockItem value) {
      Fmt.Serialize<string>(value.name);
      Fmt.Serialize<string>(value.category);
      Fmt.Serialize<string>(value.vendor);
      Fmt.Serialize<string>(value.sku);
      Fmt.Serialize<float>(value.price);
      Fmt.Serialize<int>(value.quantity);
    }

    protected override void DeserializeInstance(ref StockItem instance) {
      if (instance == null)
        instance = new StockItem();
    }

    protected override void DeserializeMembers(StockItem instance) {
      Fmt.Deserialize<string>(ref instance.name);
      Fmt.Deserialize<string>(ref instance.category);
      Fmt.Deserialize<string>(ref instance.vendor);
      Fmt.Deserialize<string>(ref instance.sku);
      instance.price = Fmt.Deserialize<float>();
      instance.quantity = Fmt.Deserialize<int>();
    }

    protected override void SkipValue() {
      if (Fmt.Skip<string>())
      if (Fmt.Skip<string>())
      if (Fmt.Skip<string>())
      if (Fmt.Skip<string>())
      if (Fmt.Skip<float>())
      Fmt.Skip<int>();
    }
  }
}