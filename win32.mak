# $RCSfile: win32.mak,v $
# $Revision: 1.1 $ $Date: 2013/11/21 20:46:41 $
# Auth: Dave Loffredo (loffredo@steptools.com)
# 

!include $(ROSE_CONFIG)

NAME		= stepnc
SOLUTION	= $(NAME).sln

EXEC		= $(NAME).dll
EXEC_INSTDIR	= $(ROSE_BIN)
EXEC_VCDIR	= $(VC32DIR)

VC32DIR		= builds\Release-Win32
VC64DIR		= builds\Release-x64

MSBUILD		= MSBuild.exe
MSBUILD_FLAGS	= $(SOLUTION) /t:Rebuild /v:detailed



#========================================
# Standard Symbolic Targets
#
# Copy rather than move on install to keep the exec in the vc dirs in
# case we want to build a MSM from them.
#

default: $(EXEC_VCDIR)\$(EXEC)
install: $(EXEC_VCDIR)\$(EXEC) 
	@if not exist "$(EXEC_INSTDIR)"\$(NULL) $(MKDIR) "$(EXEC_INSTDIR)"

build-win32: "$(VC32DIR)\$(EXEC)"
build-win64: "$(VC64DIR)\$(EXEC)"

clean:
	- $(RMDIR) "builds"
	- $(RM) $(SOLUTION).cache

very-clean spotless: clean
	-attrib -h $(NAME).suo
	-$(RM) $(NAME).suo
	-$(RM) $(NAME).vcxproj.user


#========================================
# File Targets 
#

# Originally we tried vcbuild, but that does not work for mixed C++
# and .NET projects.  MSbuild is part of the .NET framework and works
# properly for the mixed ones, which we have here.
#
# /p:Platform="Mixed Platforms"
#
# Build both versions, but we just install one to rose bin.
"$(VC32DIR)\$(EXEC)":
	$(MSBUILD) $(MSBUILD_FLAGS) /p:Platform="Win32" /p:Configuration="Release" 

"$(VC64DIR)\$(EXEC)":
	$(MSBUILD) $(MSBUILD_FLAGS) /p:Platform="x64" /p:Configuration="Release"



#========================================
# Compiler and Platform Configurations  
#
!include $(ROSE)/config/release_win.mak


# We really do not need to change 32/64 configs because the project
# file has all of that, but we do need to set the compiler to VC10
#
release: very-clean
	@echo ------------------------------
	@echo BUILDING VS 2010  
	$(USECONFIG) -win32 -cxx vc10_md & $(WINMAKE) build-win32
	$(USECONFIG) -win64 -cxx vc10_md & $(WINMAKE) build-win64

