/* $RCSfile: $
 * $Revision: $ $Date: $
 * Auth: David Loffredo (loffredo@steptools.com)
 * 
 * Copyright (c) 1991-2015 by STEP Tools Inc. 
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

using namespace System;
using namespace System::Xml;

ref class CvtContext {
public:
    XmlDocument ^ src;
    RoseDesign * dst;

    // shared step data objects
    stp_representation_context * dflt_repctx;
    stp_product_context * dflt_prodctx;
    stp_product_definition_context * dflt_pdefctx;
};

void convert_bomxml_to_ap242 (System::String ^ xmlfn, System::String ^ stpfn);
void cvt_header(CvtContext ^ cvt);
void cvt_make_common(CvtContext ^ cvt);

void cvt_make_parts(CvtContext ^ cvt);
void cvt_make_files(CvtContext ^ cvt);
void cvt_make_part_nauos(CvtContext ^ cvt);

void cvt_make_part_version(
    CvtContext ^ cvt, XmlNode ^ root, stp_product * p
    );
void cvt_make_part_view(
    CvtContext ^ cvt, XmlNode ^ root, stp_product_definition_formation * pdf
    );

void cvt_link_shapes(
    stp_next_assembly_usage_occurrence* nauo,
    StixMtrx child_placement
    );



String ^ cvt_string_element (
	XmlNode ^ root, const char * domstr, 
	RoseObject * obj, const char* expatt
	);

String ^ cvt_register_uid (
	XmlNode ^ root, 	
	RoseObject * obj
	);

int cvt_is_uidref(XmlNode ^ root);

XmlNode ^ cvt_find_refnode(CvtContext ^ cvt, XmlNode ^ n);
RoseObject * cvt_find_stpobj(CvtContext ^ cvt, XmlNode ^ n);

stp_product_definition * cvt_find_parent_pdef(CvtContext ^ cvt, XmlNode ^ n);
