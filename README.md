# bom2stp
Convert AP242 BOM XML to a STEP file
--

This is a small C++/CLR program that uses the .NET DOM capabilities
to navigate an AP242 Business Object Model (BOM) XML file, extract
the product structure, and consrtuct a conventional STEP CAD assembly
using the ST-Developer libraries.

If the XML references separate files, such as JT files, the program
will preserve those using STEP external document references.  There is
no record in the XML as to the unit system so the resulting file just
assumes millimeters / degrees. 

