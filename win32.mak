# $RCSfile: win32.mak,v $
# $Revision: 1.11 $ $Date: 2015/01/08 18:32:17 $
# Auth: Dave Loffredo (loffredo@steptools.com)
# 

!include $(ROSE_CONFIG)



#========================================
# Standard Symbolic Targets
#
# Copy rather than move on install to keep the exec in the vc dirs in
# case we want to build a MSM from them.
#

default: bom2stp-zip
install: bom2stp-zip

clean very-clean spotless: 
	$(RM) bom2stp.zip
	- $(RMDIR) bom2stp

FILES = \
	AssemblyInfo.cpp \
	app.ico \
	app.rc \
	bom2stp.cpp \
	bom2stp.h \
	bom2stp.sln \
	bom2stp.vcxproj \
	bom2stp.vcxproj.filters \
	resource.h \
	stdafx.cpp \
	stdafx.h \
	utf8fns.cpp \
	utf8fns.h

bom2stp-zip: clean
	$(MKDIR) "bom2stp"
	for $(FORVAR) in ($(FILES)) do $(CP) $(FORVAR) "bom2stp"
	$(ZIP) -r bom2stp bom2stp
