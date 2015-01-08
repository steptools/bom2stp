/* $RCSfile: bom2stp.cpp,v $
 * $Revision: 1.3 $ $Date: 2015/01/08 17:48:33 $
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
#include "stix.h"
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
    cvt-> dflt_repctx = 0;
    cvt-> dflt_prodctx = 0;
    cvt-> dflt_pdefctx = 0;

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

    cvt_make_common(cvt);
    cvt_make_files(cvt);

    cvt_make_parts(cvt);
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


void cvt_make_common(CvtContext ^ cvt)
{
    // create shared common things for file.

    // At the moment there doesn't seem to be anything that tells 
    // us what unit system is used for transforms and such, so just 
    // assume everything is millimeters and degrees for now.

    cvt-> dflt_repctx = stix_make_geometry_context(
	cvt->dst, "ID1", 3, stixunit_mm, stixunit_deg, stixunit_steradian
	);


    stp_application_context * apc = stix_make_ap_context(cvt->dst);

    cvt-> dflt_prodctx = pnewIn(cvt->dst) stp_product_context;
    cvt-> dflt_prodctx-> name ("");
    cvt-> dflt_prodctx-> discipline_type ("mechanical");
    cvt-> dflt_prodctx-> frame_of_reference (apc);

    cvt-> dflt_pdefctx = pnewIn(cvt->dst) stp_product_definition_context;

    cvt-> dflt_pdefctx-> name ("part definition");
    cvt-> dflt_pdefctx-> life_cycle_stage ("design");
    cvt-> dflt_pdefctx-> frame_of_reference (apc);

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
	p-> frame_of_reference()-> add (cvt->dflt_prodctx);

	cvt_string_element (root, "Name", p, "name");
	cvt_string_element (root, "Id/@id", p, "id");

	XmlNodeList^ vers = root->SelectNodes( "Versions/PartVersion");
	System::Collections::IEnumerator^ veritr = vers->GetEnumerator();
	while (veritr->MoveNext() ) {
	    cvt_make_part_version(cvt, safe_cast<XmlNode^>(veritr->Current), p);
	}
    }
        
    // After creating the parts and shape props, tag the data so that 
    // we can find the shape reps for these again quickly
    stix_tag_props(cvt->dst);
}


void cvt_make_part_version(CvtContext ^ cvt, XmlNode ^ root, stp_product * p)
{
    if (root == nullptr || cvt_is_uidref(root))
	return;

    stp_product_definition_formation * pdf = pnewIn(cvt->dst) stp_product_definition_formation;
    pdf-> of_product(p);

    cvt_register_uid(root, pdf);   // for now
    cvt_string_element (root, "Id/@id", pdf, "id");

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
    pdef-> formation(pdf);
    pdef-> frame_of_reference (cvt->dflt_pdefctx);

    cvt_register_uid(root, pdef);  
    cvt_string_element (root, "Id/@id", pdef, "id");
    if (!pdef->id()) pdef->id("");  // required att
    
    // Occurrence is use by someone else
    // ViewOccurrenceRelationship is nauo 
    // Create all product data first, then link in a separate pass

    // ================================
    // SHAPE - give it a shape property too.   We will tag these later for
    // easier querying when we attach the external files and nauo transforms.

    stp_shape_representation * rep = pnewIn(cvt->dst) stp_shape_representation;
    rep-> name ("");
    rep-> context_of_items (cvt->dflt_repctx);

    // Give the product a shape property.  Product_definition_shape is
    // a subtype of property_definition used for shape properties.  It 
    // refers to the product through the product_definition.
    //
    stp_product_definition_shape * pds = 
	pnewIn(cvt->dst) stp_product_definition_shape;

    pds-> name ("");
    pds-> definition (pnewIn(cvt->dst) stp_characterized_definition);
    pds-> definition ()-> _characterized_product_definition
	(pnewIn(cvt->dst) stp_characterized_product_definition);
    pds-> definition ()-> _characterized_product_definition()-> 
	_product_definition (pdef);


    // Attach the shape representation to the property.  The
    // shape_definition_representation subtype is used for shape
    // properties and the property_definition_representation supertype
    // is used for other types of properties.
    //
    stp_shape_definition_representation * sdr = 
	pnewIn(cvt->dst) stp_shape_definition_representation;

    sdr-> definition (pnewIn(cvt->dst) stp_represented_definition);
    sdr-> definition ()-> _property_definition (pds);
    sdr-> used_representation (rep);


    // ==================================
    // EXTERNAL FILES -- Attach external document ref if one is present.
    // This assumes that we have already gone and made document files for
    // all of the File elements in the document
    //
    XmlNodeList^ nodes = root->SelectNodes( "DocumentAssignment");
    System::Collections::IEnumerator^ itr = nodes->GetEnumerator();
    while ( itr->MoveNext() ) {
	XmlNode ^ da = safe_cast<XmlNode^>(itr->Current);
	stp_document_file * df = ROSE_CAST(stp_document_file,cvt_find_stpobj(
		cvt, da-> SelectSingleNode("AssignedDocument ")
		));

	if (!df) continue;

	stp_applied_document_reference * adr = pnewIn(cvt->dst) stp_applied_document_reference;
	adr-> assigned_document(df);
	adr-> source("");

	stp_document_reference_item * dri = pnewIn(cvt->dst) stp_document_reference_item;
	dri-> _product_definition(pdef);
	adr-> items()-> add(dri);
	// skip the associated role

	// Attach document file to the rep that it describes using an
	// ordinary property.
	//
	stp_property_definition * prop = pnewIn(cvt->dst) stp_property_definition;
	prop-> name ("external definition");
	prop-> definition (pnewIn(cvt->dst) stp_characterized_definition);
	prop-> definition()-> _characterized_object(df);

	stp_property_definition_representation * pdr = 
	    pnewIn(cvt->dst) stp_property_definition_representation;
	pdr-> used_representation(rep);
	pdr-> definition (pnewIn(cvt->dst) stp_represented_definition);
	pdr-> definition()-> _property_definition(prop);
    }
}




void cvt_make_part_nauos(CvtContext ^ cvt)
{
  // ViewOccurrenceRelationship is nauo 
    // Occurrence is use by someone else

    array <Char> ^a_separators = gcnew array <Char> {' '};

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

	cvt_string_element (occur, "Id/@id", nauo, "id");
	if (!nauo-> id()) nauo->id("");  // required atts 
	if (!nauo-> name()) nauo-> name("");

	// Attach the placement information

	XmlNode ^ aprot = vor-> SelectSingleNode("Placement/CartesianTransformation/RotationMatrix");
	XmlNode ^ aploc = vor-> SelectSingleNode("Placement/CartesianTransformation/TranslationVector");

	if (aploc ==nullptr || aprot == nullptr)
	    continue;

	// <RotationMatrix>0.000000 0.000000 -1.000000 -0.000000 1.000000 0.000000 1.000000 0.000000 0.000000</RotationMatrix>
	//  <TranslationVector>-10.000000 75.000000 60.000000</TranslationVector>
	double rot[9] = {1.,0.,0., 0.,1.,0., 0.,0.,1.};
	double xyz[3] = {0.,0.,0.};

	unsigned i,sz;
	array <String^> ^a_doubles = aprot-> InnerText->Split (a_separators, StringSplitOptions::RemoveEmptyEntries);
	for (i=0,sz=a_doubles->Length; i<sz && i<9; i++) {
	    if (!double::TryParse (a_doubles [i], rot[i]))
		break;
	}
	a_doubles = aploc-> InnerText->Split (a_separators, StringSplitOptions::RemoveEmptyEntries);
	for (i=0,sz=a_doubles->Length; i<sz && i<3; i++) {
	    if (!double::TryParse (a_doubles [i], xyz[i]))
		break;
	}

	StixMtrx mtrx = stix_make_normalized_matrix (
	    xyz[0], xyz[1], xyz[2],  
	    rot[6], rot[7], rot[8],   // Z dir
	    rot[0], rot[1], rot[2]    // X dir
	);
	
	// Build the shape side of the assembly to specify the placement
	cvt_link_shapes(nauo, mtrx);
    }
}




void cvt_make_files(CvtContext ^ cvt)
{
    stp_document_type * doctyp = 0;
    stp_identification_role * idrole = 0;

    XmlNodeList^ nodes = cvt->src->DocumentElement->SelectNodes( "//File");
    System::Collections::IEnumerator^ itr = nodes->GetEnumerator();
    while ( itr->MoveNext() ) {
	XmlNode ^ root = safe_cast<XmlNode^>(itr->Current);

	if (root == nullptr || cvt_is_uidref(root))
	    continue;

	// Only one instance needed, share across all files
	if (!doctyp) {
	    doctyp = pnewIn(cvt->dst) stp_document_type;
	    doctyp-> product_data_type ("");
	}

	if (!idrole) {
	    idrole = pnewIn(cvt->dst) stp_identification_role;
	    idrole-> name ("external document id and location");
	}

	stp_document_file * df = pnewIn(cvt->dst) stp_document_file;

	cvt_register_uid(root, df);  
	cvt_string_element (root, "Id/@id", df, "id");

	df-> stp_document::name("");
	df-> stp_document::description(NULL);

	df-> stp_characterized_object::name("");
	df-> stp_characterized_object::description(NULL);
	df-> kind(doctyp);


	// shown in recprat, does this add anything useful?
	stp_document_representation_type * drt = 
	    pnewIn(cvt->dst) stp_document_representation_type;
	drt-> name("digital");
	drt-> represented_document(df);


	// This is apparently a new way of attaching the name to a
	// document file, and should have the same value as the document
	// file id, not clear to me why this was done.
	//
	stp_applied_external_identification_assignment * aeia = 
	    pnewIn(cvt->dst) stp_applied_external_identification_assignment;

	aeia-> assigned_id (df->id());
	aeia-> role(idrole); 

	// this is for path or URL information?
	aeia-> source(pnewIn(cvt->dst) stp_external_source); 
	aeia-> source()-> source_id(pnewIn(cvt->dst) stp_source_item);
	aeia-> source()-> source_id()-> _Identifier(df->id());

	stp_external_identification_item * eii = pnewIn(cvt->dst)
	    stp_external_identification_item;

	eii-> _document_file(df);
	aeia-> items()-> add(eii);
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


XmlNode ^ cvt_find_refnode(CvtContext ^ cvt, XmlNode ^ n)
{
    // There is probably a simpler XPath expression to do this 
    // using ancestor:: and @uid = @uidRef or the id() function.  
    // This is kind of brute force, but it works.
    if (n == nullptr) return nullptr;

    XmlNode ^ idref = n->Attributes->GetNamedItem("uidRef");
    if (idref == nullptr) return nullptr;

    return cvt->src->DocumentElement->SelectSingleNode(
	String::Format("//*[@uid = '{0}']", idref->InnerText)
	);}


RoseObject * cvt_find_stpobj(CvtContext ^ cvt, XmlNode ^ n)
{
    // got a pdef thing, go find by the uid in the step data
    XmlNode ^ uid = n->Attributes->GetNamedItem("uid");
    if (uid == nullptr) uid = n->Attributes->GetNamedItem("uidRef");
    if (uid == nullptr) return 0;

    String ^ val = uid->InnerText;
    MARSHAL_WIDE_TO_UTF8(val, val_utf8);
    RoseObject * obj = cvt-> dst-> findObject((const char*)val_utf8);
    MARSHAL_UTF8_DONE(val, val_utf8);

    return obj;
}


stp_product_definition * cvt_find_parent_pdef(CvtContext ^ cvt, XmlNode ^ n)
{
    while (n != nullptr) 
    {
	if (n->Name == "PartView") 
	    return ROSE_CAST(stp_product_definition,cvt_find_stpobj(cvt,n));

	n = n->ParentNode;
    }
    return 0;
}


// just get first shape rep that we see
stp_representation * find_shape_rep (stp_product_definition * pd)
{
    StixMgrPropertyRep * mgr = StixMgrPropertyRep::find(stix_get_shape_property(pd));
    return (mgr? mgr->get_rep(0): 0);
}


// creates the geometric portion of the assembly using a
// context_dependent_shape_representation.
//
void cvt_link_shapes(
    stp_next_assembly_usage_occurrence* nauo,
    StixMtrx child_placement
    )
{
    /* We pass in representations to avoid having to do casts,
     * but this should only be used to relate instances of the
     * shape representation subtype.
     */
    RoseDesign * d = nauo->design();
    
    // Create a product_definition_shape to link the cdsr to the nauo
    stp_product_definition_shape* pds = pnewIn(d) stp_product_definition_shape();

    // There's no standard mapping for the name or description.
    pds->name("");
    pds->description("");

    // The definition should point to next_assembly_usage_occurrence.
    stp_characterized_definition* cd = pnewIn(d) stp_characterized_definition();
    stp_characterized_product_definition* cpd = 
	pnewIn(d) stp_characterized_product_definition();
    cd->_characterized_product_definition(cpd);
    cpd->_product_definition_relationship(nauo);
    pds->definition(cd);

    // Create a context_dependent_shape_representation.
    stp_context_dependent_shape_representation* cdsr = 
	pnewIn(d) stp_context_dependent_shape_representation();

    // The represented_product_relation is the pds.
    cdsr->represented_product_relation(pds);

    // A complex entity is used for the shape_representation_relationship.
    stp_representation_relationship_with_transformation_and_shape_representation_relationship * repRel = 
	pnewIn(d) stp_representation_relationship_with_transformation_and_shape_representation_relationship();
    cdsr->representation_relation(repRel);

    // The name and description attributes have no standard mapping
    // rep_1 is the assembly shape and rep_2 is the component.
    repRel->name("");
    repRel->description("");
    repRel->rep_1(find_shape_rep(stix_get_relating_pdef(nauo)));
    repRel->rep_2(find_shape_rep(stix_get_related_pdef(nauo)));

    // The first transform_item is the placement of the child in the assembly 
    // and the second is the origin of the component that it is measured from.
    StixMtrx origin;
    stp_axis2_placement_3d * ap_ing = child_placement.makeAp3dIn(d, nauo->id());
    stp_axis2_placement_3d * ap_ed = origin.makeAp3dIn(d, nauo->id());

    repRel->rep_1()-> items()-> add(ap_ing);
    repRel->rep_2()-> items()-> add(ap_ed);

    // The transformation_operator should be an item_defined_transform.
    stp_item_defined_transformation* xform = pnewIn(d) stp_item_defined_transformation();
    stp_transformation* trans = pnewIn(d) stp_transformation();
    trans->_item_defined_transformation(xform);
    repRel->transformation_operator(trans);

    // The name and description fields have no standard mapping.
    xform->name("");
    xform->description("");
    xform->transform_item_1(ap_ing);
    xform->transform_item_2(ap_ed); // identity matrix
}
