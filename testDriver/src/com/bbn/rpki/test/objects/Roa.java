/*
 * Created on Nov 8, 2011
 */
package com.bbn.rpki.test.objects;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;


/**
 * <Enter the description of this type here>
 *
 * @author RTomlinson
 */
public class Roa extends CMS {

  /** asID */
  public final List<String> asID;

  /** roaipv4 */
  public final IPRangeList roaipv4;

  /** roaipv6 */
  public final IPRangeList roaipv6;

  private final EE_Object ee_object;

  /**
   * @param myFactory
   * @param ee_object
   */
  public Roa(RoaFactory myFactory, EE_Object ee_object) {
    super("ROA");
    // Pull the info we need from our ee_object
    this.asID           = new ArrayList<String>(myFactory.asid.size());
    for (Pair pair : myFactory.asid) {
      asID.add(String.format("'%s'", pair.arg.toString()));
    }
    this.roaipv4        = ee_object.subAllocateIPv4(myFactory.ROAipv4List);
    this.roaipv6        = ee_object.subAllocateIPv6(myFactory.ROAipv6List);
    this.outputfilename = REPO_PATH + ee_object.path_ROA;
    this.ee_object = ee_object;
    // Make our directory to place our ROA if it doesn't already exist
    //    String dir_path = REPO_PATH + ee_object.parent.SIA_path;
  }

  /**
   * @see com.bbn.rpki.test.objects.CA_Obj#getFieldMap(java.util.Map)
   */
  @Override
  public void getFieldMap(Map<String, Object> map) {
    super.getFieldMap(map);
    map.put("asID", asID);
    map.put("roaipv4", roaipv4);
    map.put("roaipv6", roaipv6);
  }

  /**
   * @see com.bbn.rpki.test.objects.CMS#getEECert()
   */
  @Override
  protected EE_cert getEECert() {
    return ee_object.getCertificate();
  }

  /**
   * @see java.lang.Object#toString()
   */
  @Override
  public String toString() {
    return String.format("Manifest(%s)", ee_object.parent.commonName);
  }
}