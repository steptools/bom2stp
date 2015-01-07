/* $RCSfile: bom2stp.cpp,v $
 * $Revision: 1.2 $ $Date: 2015/01/07 04:18:33 $
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
#include "bom2stp.h"
#include "utf8fns.h"



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

    cvt_header(cvt); 

    cvt_make_parts(cvt);
    cvt_make_files(cvt);
    cvt_make_part_nauos(cvt);

    // uncomment to see the xml uid strings
    // RoseP21Writer::max_spec_version(PART21_ED3);
    cvt->dst-> save();
}


void cvt_header(CvtContext ^ cvt)
{
    // Initialize the design with information from the BOM Xml Header element.  
    XmlNode ^ head = cvt->src->DocumentElement-> SelectSingleNode("//Header");
    XmlNode ^ n;
    if (head == nullptr) return;

    cvt->dst->initialize_header();

    cvt_string_element (head, "Name", cvt->dst-> header_name(), "name");
    cvt_string_element (head, "OriginatingSystem", cvt->dst-> header_name(), "originating_system");
    cvt_string_element (head, "Authorization", cvt->dst-> header_name(), "authorisation");
    
    n = head-> SelectSingleNode("Documentation");
    if (n != nullptr) {
	String ^ val = n->InnerText;
	MARSHAL_WIDE_TO_UTF8(val, val_utf8);
	cvt->dst-> header_description()-> description()-> add((const char*)val_utf8);
	MARSHAL_UTF8_DONE(val, val_utf8);
    }
}



void cvt_make_parts(CvtContext ^ cvt)
{
    XmlNodeList^ nodes = cvt->src->DocumentElement->SelectNodes( "//Part");
    System::Collections::IEnumerator^ itr = nodes->GetEnumerator();
    while ( itr->MoveNext() ) {
	XmlNode ^ root = safe_cast<XmlNode^>(itr->Current);

	if (root == nullptr || cvt_is_uidref(root))
	    continue;

	stp_product * p = pnewIn(cvt->dst) stp_product;

	cvt_string_element (root, "/Name", p, "name");
	cvt_string_attribute (root-> SelectSingleNode("Id"), "id", p, "id");

	XmlNodeList^ vers = root->SelectNodes( "Versions/PartVersion");
	System::Collections::IEnumerator^ veritr = vers->GetEnumerator();
	while (veritr->MoveNext() ) {
	    cvt_make_part_version(cvt, safe_cast<XmlNode^>(veritr->Current), p);
	}
    }
}


void cvt_make_part_version(CvtContext ^ cvt, XmlNode ^ root, stp_product * p)
{
    if (root == nullptr || cvt_is_uidref(root))
	return;

    stp_product_definition_formation * pdf = pnewIn(cvt->dst) stp_product_definition_formation;
    pdf-> of_product(p);

    cvt_register_uid(root, pdf);   // for now
    cvt_string_attribute (root-> SelectSingleNode("Id"), "id", pdf, "id");

    XmlNodeList^ nodes = root->SelectNodes( "Views/PartView");
    System::Collections::IEnumerator^ itr = nodes->GetEnumerator();
    while ( itr->MoveNext() ) {
	cvt_make_part_view(cvt, safe_cast<XmlNode^>(itr->Current), pdf);
    }
}


void cvt_make_part_view(CvtContext ^ cvt, XmlNode ^ root, stp_product_definition_formation * pdf)
{
  if (root == nullptr || cvt_is_uidref(root))
	return;

    stp_product_definition * pdef = pnewIn(cvt->dst) stp_product_definition;
    pdef->formation(pdf);

    cvt_register_uid(root, pdef);  
    cvt_string_attribute (root-> SelectSingleNode("Id"), "id", pdef, "id");
    
    // Occurrence is use by someone else
    // ViewOccurrenceRelationship is nauo 
    // Create all product data first, then link in a separate pass
}




void cvt_make_part_nauos(CvtContext ^ cvt)
{
  // ViewOccurrenceRelationship is nauo 
    // Occurrence is use by someone else
    XmlNodeList^ nodes = cvt->src->DocumentElement-> SelectNodes( "//ViewOccurrenceRelationship");
    System::Collections::IEnumerator^ itr = nodes->GetEnumerator();
    while ( itr->MoveNext() ) {
	XmlNode ^ vor = safe_cast<XmlNode^>(itr->Current);

	if (vor == nullptr || cvt_is_uidref(vor)) continue;

	stp_next_assembly_usage_occurrence * nauo = 
	    pnewIn(cvt->dst) stp_next_assembly_usage_occurrence;

	cvt_register_uid(vor, nauo);  

	// get enclosing PartView, that is the relating
	stp_product_definition * pdef_ing = cvt_find_parent_pdef(cvt, vor);
	if (!pdef_ing) {
	    Console::WriteLine("Could not find relating PartView for {0}", vor->OuterXml);
	}
	nauo-> relating_product_definition(pnewIn(cvt->dst) stp_product_definition_or_reference);
	nauo-> relating_product_definition()-> _product_definition(pdef_ing);

	// get the related element, that is the related and has the id
	XmlNode ^ occur = cvt_find_refnode(cvt, vor-> SelectSingleNode("Related"));
	stp_product_definition * pdef_ed = cvt_find_parent_pdef(cvt, occur);

	if (!pdef_ed) {
	    Console::WriteLine("Could not find related PartView for {0}", vor->OuterXml);
	}
	nauo-> related_product_definition(pnewIn(cvt->dst) stp_product_definition_or_reference);
	nauo-> related_product_definition()-> _product_definition(pdef_ed);

	cvt_string_attribute (occur-> SelectSingleNode("Id"), "id", nauo, "id");
    }
}




void cvt_make_files(CvtContext ^ cvt)
{
    XmlNodeList^ nodes = cvt->src->DocumentElement->SelectNodes( "//File");
    System::Collections::IEnumerator^ itr = nodes->GetEnumerator();
    while ( itr->MoveNext() ) {
	XmlNode ^ root = safe_cast<XmlNode^>(itr->Current);

	if (root == nullptr || cvt_is_uidref(root))
	    continue;
	
	stp_document_file * df = pnewIn(cvt->dst) stp_document_file;

	cvt_register_uid(root, df);  
	cvt_string_attribute (root-> SelectSingleNode("Id"), "id", df, "id");

	df-> stp_document::name("");
	df-> stp_document::description(NULL);

	df-> stp_characterized_object::name("");
	df-> stp_characterized_object::description(NULL);
	//df-> kind(doctyp);
    }
}

// ===========================================================
// UTILITY FUNCTIONS 
//

String ^ cvt_string_element (
	XmlNode ^ root, const char * domstr, 
	RoseObject * obj, const char* expatt
	)
{
    // Set a rose attribute to the entire inner text contents of an
    // XML element
    //

    if (root == nullptr || !obj) return nullptr;

    XmlNode ^ n = root-> SelectSingleNode(gcnew String(domstr));
    if (n != nullptr) {
	String ^ val = n->InnerText;
	MARSHAL_WIDE_TO_UTF8(val, val_utf8);
	obj-> putString((const char*)val_utf8, expatt);
	MARSHAL_UTF8_DONE(val, val_utf8);
	return val;
    }
    return nullptr;
}

String ^ cvt_string_attribute (
	XmlNode ^ root, const char * xmlatt, 
	RoseObject * obj, const char* expatt
	)
{
    // Set a rose attribute to the entire inner text contents of an
    // XML attribute
    //
    if (root == nullptr || !obj) return nullptr;

    XmlNode ^ n = root->Attributes->GetNamedItem(gcnew String(xmlatt));
    if (n != nullptr) {
	String ^ val = n->InnerText;
	MARSHAL_WIDE_TO_UTF8(val, val_utf8);
	obj-> putString((const char*)val_utf8, expatt);
	MARSHAL_UTF8_DONE(val, val_utf8);
	return val;
    }
    return nullptr;
}

String ^ cvt_register_uid (
	XmlNode ^ root, 	
	RoseObject * obj
	)
{
    // find the UID attribute and register it with the design
    // by assigning it as the name of the rose object.
    //
    if (root == nullptr || !obj) return nullptr;
    XmlNode ^ n = root->Attributes->GetNamedItem("uid");
    if (n == nullptr) return nullptr;

    String ^ uid = n->InnerText;
    if (String::IsNullOrWhiteSpace(uid)) return nullptr;

    MARSHAL_WIDE_TO_UTF8(uid, uid_utf8);
    obj-> design()-> addName((const char*)uid_utf8, obj);
    MARSHAL_UTF8_DONE(uid, uid_utf8);

    return uid;
}


int cvt_is_uidref (XmlNode ^ root)
{
    return (root != nullptr && root->Attributes->GetNamedItem("uidRef") != nullptr);
}

stp_product_definition * cvt_find_parent_pdef(CvtContext ^ cvt, XmlNode ^ n)
{
    while (n != nullptr) 
    {
	if (n->Name != "PartView") {
	    n = n->ParentNode;
	    continue;
	}
	// got a pdef thing, go find by the uid in the step data
	XmlNode ^ uid = n->Attributes->GetNamedItem("uid");
	if (uid == nullptr) return 0;

	String ^ val = uid->InnerText;
	MARSHAL_WIDE_TO_UTF8(val, val_utf8);
	RoseObject * obj = cvt-> dst-> findObject((const char*)val_utf8);
	MARSHAL_UTF8_DONE(val, val_utf8);

	return ROSE_CAST(stp_product_definition,obj);
    }
    return 0;
}


XmlNode ^ cvt_find_refnode(CvtContext ^ cvt, XmlNode ^ n)
{
    if (n == nullptr) return nullptr;

    XmlNode ^ idref = n->Attributes->GetNamedItem("uidRef");
    if (idref == nullptr) return nullptr;

    return cvt->src->DocumentElement->SelectSingleNode(
	String::Format("//*[@uid = '{0}']", idref->InnerText)
	);
}