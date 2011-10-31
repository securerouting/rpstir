/* ***** BEGIN LICENSE BLOCK *****
 * 
 * BBN Rule Editor/Engine for Address and AS Number PKI
 * Verison 1.0
 * 
 * COMMERCIAL COMPUTER SOFTWARE�RESTRICTED RIGHTS (JUNE 1987)
 * US government users are permitted restricted rights as
 * defined in the FAR.  
 *
 * This software is distributed on an "AS IS" basis, WITHOUT
 * WARRANTY OF ANY KIND, either express or implied.
 *
 * Copyright (C) Raytheon BBN Technologies Corp. 2007.  All Rights Reserved.
 *
 * Contributor(s):  Charlie Gardiner
 *
 * ***** END LICENSE BLOCK ***** */
package skaction;
import name.*;
import Algorithms.*;
import certificate.*;
import crlv2.*;
import asn.*;
public class ChangeCIKAudit extends AsnSequence
    {
    public HowMany how_many = new HowMany();
    public ToWhat to_what = new ToWhat();
    public CIKs ciks = new CIKs();
    public NamesAndKeys names = new NamesAndKeys();
    public ChangeCIKAudit()
        {
        _tag = AsnStatic.ASN_SEQUENCE;
        _type = (short)AsnStatic.ASN_SEQUENCE;
        _setup((AsnObj)null, how_many, (short)0, (int)0x0);
        _setup(how_many, to_what, (short)0, (int)0x0);
        _setup(to_what, ciks, (short)0, (int)0x0);
        _setup(ciks, names, (short)0, (int)0x0);
        }
    public ChangeCIKAudit set(ChangeCIKAudit frobj)
        {
        ((AsnObj)this).set(frobj);
	return this;
	}
    }