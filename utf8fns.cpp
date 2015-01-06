/* $RCSfile: utf8fns.cpp,v $
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

#include "utf8fns.h"
#include <string.h>

using namespace System;
using namespace System::Text;
using namespace System::Runtime::InteropServices;

// Convert a managed UTF16 .NET string into a managed null-terminated
// UTF8 byte array.  When used with unmanaged code, you can then just
// pin the pointer to the byte array and use it as an unmanaged char*
//
array<unsigned char> ^ MakeNullTermUTF8 (String ^dotnet_str) 
{
    if (!dotnet_str) return nullptr;
    
    // Since we need to add a trailing null character, allocate the
    // character array ourself and then populate it with a variation
    // of the GetBytes call.

    // GetMaxByteCount is faster
    size_t bytesz = Encoding::UTF8->GetByteCount(dotnet_str);
    array<unsigned char> ^bytes = gcnew array<unsigned char>((int)bytesz + 1);
	
    // Copy to array, returns the number of bytes copies, use to null
    // terminate the array.
    size_t strlen = Encoding::UTF8->GetBytes(
	dotnet_str, 0, dotnet_str->Length, bytes, 0
	);
    bytes[(int)strlen] = 0;
    return bytes;
}


// Copy and convert an unmanaged null-terminated UTF8 string into a
// new managed UTF16 .NET String.
//
String ^ MakeStringFromUTF8 (const char * utf8_str)
{
    // Never return a null string from this interface
    // if (!utf8_str) return nullptr;
    if (!utf8_str) return String::Empty;

    size_t bytelen = strlen(utf8_str);
    if (!bytelen) return String::Empty;

    array<unsigned char> ^bytes = gcnew array<unsigned char>((int)bytelen);
    { 	
	pin_ptr<unsigned char> pinnedBytes = &bytes[0];
	memcpy(pinnedBytes, utf8_str, bytelen);
    }
    return Encoding::UTF8->GetString(bytes);
}

