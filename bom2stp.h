/* $RCSfile: bom2stp.h,v $
 * $Revision: 1.1 $ $Date: 2015/01/07 04:18:33 $
 * Auth: David Loffredo (loffredo@steptools.com)
 * 
 * Copyright (c) 1991-2015 by STEP Tools Inc. 
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted, provided that this copyright
 * notice and license appear on all copies of the software.
 * 
 * STEP TOOLS MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. STEP TOOLS
 * SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A
 * RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS
 * DERIVATIVES.
 */

#pragma once

using namespace System;
using namespace System::Xml;

ref class CvtContext {
public:
    XmlDocument ^ src;
    RoseDesign * dst;
};

void convert_bomxml_to_ap242 (System::String ^ xmlfn, System::String ^ stpfn);
void cvt_header(CvtContext ^ cvt);

void cvt_make_parts(CvtContext ^ cvt);
void cvt_make_files(CvtContext ^ cvt);
void cvt_make_part_nauos(CvtContext ^ cvt);

void cvt_make_part_version(
    CvtContext ^ cvt, XmlNode ^ root, stp_product * p
    );
void cvt_make_part_view(
    CvtContext ^ cvt, XmlNode ^ root, stp_product_definition_formation * pdf
    );




String ^ cvt_string_element (
	XmlNode ^ root, const char * domstr, 
	RoseObject * obj, const char* expatt
	);

String ^ cvt_string_attribute (
	XmlNode ^ root, const char * xmlatt, 
	RoseObject * obj, const char* expatt
	);

String ^ cvt_register_uid (
	XmlNode ^ root, 	
	RoseObject * obj
	);

int cvt_is_uidref(XmlNode ^ root);

stp_product_definition * cvt_find_parent_pdef(CvtContext ^ cvt, XmlNode ^ n);
XmlNode ^ cvt_find_refnode(CvtContext ^ cvt, XmlNode ^ n);
