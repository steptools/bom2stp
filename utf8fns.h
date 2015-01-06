/* $RCSfile: utf8fns.h,v $
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

#ifndef UTF8FNS_H
#define UTF8FNS_H


// Marshal the string to UTF8 for roselib usage. 
#define MARSHAL_WIDE_TO_UTF8(widevar,ptrvar) \
array<unsigned char>^widevar##_as_utf8 = MakeNullTermUTF8 (widevar); \
pin_ptr<unsigned char> ptrvar = &widevar##_as_utf8[0]

// Unpin and force cleanup so that we have predictable memory usage.
// The memory will eventually be garbage collected at some point but
// we know that we are done now.
#define MARSHAL_UTF8_DONE(widevar,ptrvar) \
    ptrvar = nullptr; delete widevar##_as_utf8;

// Convert a managed UTF16 .NET string into a managed null-terminated
// UTF8 byte array.  When used with unmanaged code, you can then just
// pin the pointer to the byte array and use it as an unmanaged char*
//
array<unsigned char> ^ MakeNullTermUTF8 (System::String ^dotnet_str);

// Copy and convert an unmanaged null-terminated UTF8 string into a
// new managed UTF16 .NET String.
//
System::String ^ MakeStringFromUTF8 (const char * utf8_str);


#endif
