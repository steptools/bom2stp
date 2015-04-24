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


## Building

This package contains a Visual Studio 2013 project file for building.
When using with ST-Developer Personal Edition, set the platform to
either "Release DLL" or "Debug DLL".

The package uses the ST-Developer libraries to read/write STEP files.
A downloadable version that is free for personal use can be found at:

ST-Developer Personal Edition 
- Download - http://www.steptools.com/products/stdev/personal.html
- API Docs - http://www.steptools.com/support/stdev_docs/


## Extending

Is there some part of the BOM model that the code does not handle?
Consider adding support for it!  See
[CONTRIBUTING.md](CONTRIBUTING.md) if you would like to send a pull
request with your changes.
