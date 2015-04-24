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
