/* $RCSfile: bom2stp.cpp,v $
 * $Revision: 1.1 $ $Date: 2015/01/06 03:02:06 $
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

#include "stdafx.h"
#include "stp_schema.h"
#include "utf8fns.h"

using namespace System;
using namespace System::Xml;

ref class CvtContext {
public:
    XmlDocument ^ src;
    RoseDesign * dst;
};

void convert_bomxml_to_ap242 (System::String ^ xmlfn, System::String ^ stpfn);
void cvtbom_header(CvtContext ^ cvt);
void cvtbom_contents(CvtContext ^ cvt);

const char * cvtbom_string_element (
	XmlNode ^ root, const char * domstr, 
	RoseObject * obj, const char* expatt
	);

int main(array<System::String ^> ^args)
{
    // Get filename, replace ext with _ap242.stp
    stplib_init();
    if (args->Length < 2) {
	    Console::WriteLine("usage: bom2stp <xmlfile> <stpfile>");
	return 1;
    }
    convert_bomxml_to_ap242(args[0], args[1]); 
    
    return 0;
}



void convert_bomxml_to_ap242 (System::String ^ xmlfn, System::String ^ stpfn)
{
    MARSHAL_WIDE_TO_UTF8(stpfn,stpfn_utf8);

    CvtContext ^ cvt = gcnew CvtContext;
    cvt-> src = gcnew XmlDocument();
    cvt-> dst = new RoseDesign;
    cvt-> dst-> path((const char*) stpfn_utf8);
    stplib_put_schema(cvt->dst, stplib_schema_ap242);
    
    MARSHAL_UTF8_DONE(stpfn,stpfn_utf8);

    try {
	cvt-> src-> Load(xmlfn);
    }
    catch (Exception ^ e) {
	Console::WriteLine(String::Format("Error reading BOM XML"));
	Console::WriteLine(e->Message);
    }

    cvtbom_header(cvt); 
    cvtbom_contents(cvt); 
    
    cvt->dst-> save();
}


void cvtbom_header(CvtContext ^ cvt)
{
    // Initialize the design with information from the BOM Xml Header element.  
    XmlNode ^ head = cvt->src->DocumentElement-> SelectSingleNode("//Header");
    XmlNode ^ n;
    if (head == nullptr) return;

    cvt->dst->initialize_header();

    cvtbom_string_element (head, "Name", cvt->dst-> header_name(), "name");
    cvtbom_string_element (head, "OriginatingSystem", cvt->dst-> header_name(), "originating_system");
    cvtbom_string_element (head, "Authorization", cvt->dst-> header_name(), "authorisation");
    
    n = head-> SelectSingleNode("Documentation");
    if (n != nullptr) {
	String ^ val = n->InnerText;
	MARSHAL_WIDE_TO_UTF8(val, val_utf8);
	cvt->dst-> header_description()-> description()-> add((const char*)val_utf8);
	MARSHAL_UTF8_DONE(val, val_utf8);
    }
}


void cvtbom_contents(CvtContext ^ cvt)
{
    // Go through the body of the file and start converting elements.  
    XmlNode ^ data = cvt->src->DocumentElement-> SelectSingleNode("//DataContainer");
    XmlNode ^ n;
    XmlNodeList^ nodes;
    System::Collections::IEnumerator^ itr;
    if (data == nullptr) return;

    nodes = data->SelectNodes( "//Part");
    itr = nodes->GetEnumerator();
    while ( itr->MoveNext() )
    {
	n = safe_cast<XmlNode^>(itr->Current);
	stp_product * p = pnewIn(cvt->dst) stp_product;
	stp_product_definition_formation * pdf = pnewIn(cvt->dst) stp_product_definition_formation;
	stp_product_definition * pdef = pnewIn(cvt->dst) stp_product_definition;

	pdef->formation(pdf);
	pdf-> of_product(p);
	
	cvtbom_string_element (n, "Name", p, "name");
	cvtbom_string_element (n, "Id", p, "id");
	
	cvtbom_string_element (n, "Id", pdef, "id");



	// views and versions as well

    }

}





// ===========================================================
// UTILITY FUNCTIONS 
//
// Set a rose attribute to the entire inner text contents of an XML element
//
const char * cvtbom_string_element (
	XmlNode ^ root, const char * domstr, 
	RoseObject * obj, const char* expatt
	)
{
    if (root == nullptr || !obj) return 0;

    XmlNode ^ n = root-> SelectSingleNode(gcnew String(domstr));
    if (n != nullptr) {
	String ^ val = n->InnerText;
	MARSHAL_WIDE_TO_UTF8(val, val_utf8);
	obj-> putString((const char*)val_utf8, expatt);
	MARSHAL_UTF8_DONE(val, val_utf8);

	// the pinned, utf8 converted string will disappear when we 
	// return from this function, so return the rose copy of it.
	return obj-> getString(expatt);
    }
    return 0;
}
