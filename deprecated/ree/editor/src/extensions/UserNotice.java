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
package extensions;
import orname.*;
import name.*;
import Algorithms.*;
// import serial_number.*;
import asn.*;
public class UserNotice extends AsnSequence
    {
    public NoticeReference noticeRef = new NoticeReference();
    public DisplayText explicitText = new DisplayText();
    public UserNotice()
        {
        _tag = AsnStatic.ASN_SEQUENCE;
        _type = (short)AsnStatic.ASN_SEQUENCE;
        _setup((AsnObj)null, noticeRef, (short)(AsnStatic.ASN_OPTIONAL_FLAG), (int)0x0);
        _setup(noticeRef, explicitText, (short)(AsnStatic.ASN_OPTIONAL_FLAG), (int)0x0);
        }
    public UserNotice set(UserNotice frobj)
        {
        ((AsnObj)this).set(frobj);
	return this;
	}
    }