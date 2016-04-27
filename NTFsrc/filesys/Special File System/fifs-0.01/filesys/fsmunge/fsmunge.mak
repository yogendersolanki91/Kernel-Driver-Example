# Microsoft Developer Studio Generated NMAKE File, Based on fsmunge.dsp
!IF "$(CFG)" == ""
CFG=fsmunge - Win32 Debug
!MESSAGE No configuration specified. Defaulting to fsmunge - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "fsmunge - Win32 Release" && "$(CFG)" != "fsmunge - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fsmunge.mak" CFG="fsmunge - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fsmunge - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "fsmunge - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "fsmunge - Win32 Release"

OUTDIR=.\..\..\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\..\..\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\fsmunge.dll"

!ELSE 

ALL : "fshelper - Win32 Release" "$(OUTDIR)\fsmunge.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"fshelper - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\conf_munge.obj"
	-@erase "$(INTDIR)\fs_munge.obj"
	-@erase "$(INTDIR)\fsdt_munge.obj"
	-@erase "$(INTDIR)\main_munge.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\fsmunge.dll"
	-@erase "$(OUTDIR)\fsmunge.exp"
	-@erase "$(OUTDIR)\fsmunge.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\fsmunge.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=fshelper.lib kernel32.lib advapi32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\fsmunge.pdb" /machine:I386 /def:".\fsmunge.def" /out:"$(OUTDIR)\fsmunge.dll" /implib:"$(OUTDIR)\fsmunge.lib" /libpath:"..\..\Release" 
DEF_FILE= \
	".\fsmunge.def"
LINK32_OBJS= \
	"$(INTDIR)\conf_munge.obj" \
	"$(INTDIR)\fs_munge.obj" \
	"$(INTDIR)\fsdt_munge.obj" \
	"$(INTDIR)\main_munge.obj" \
	"$(OUTDIR)\fshelper.lib"

"$(OUTDIR)\fsmunge.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "fsmunge - Win32 Debug"

OUTDIR=.\..\..\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\..\..\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\fsmunge.dll" ".\Debug\fsmunge.bsc"

!ELSE 

ALL : "fshelper - Win32 Debug" "$(OUTDIR)\fsmunge.dll" ".\Debug\fsmunge.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"fshelper - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\conf_munge.obj"
	-@erase "$(INTDIR)\conf_munge.sbr"
	-@erase "$(INTDIR)\fs_munge.obj"
	-@erase "$(INTDIR)\fs_munge.sbr"
	-@erase "$(INTDIR)\fsdt_munge.obj"
	-@erase "$(INTDIR)\fsdt_munge.sbr"
	-@erase "$(INTDIR)\main_munge.obj"
	-@erase "$(INTDIR)\main_munge.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\fsmunge.dll"
	-@erase "$(OUTDIR)\fsmunge.exp"
	-@erase "$(OUTDIR)\fsmunge.ilk"
	-@erase "$(OUTDIR)\fsmunge.lib"
	-@erase "$(OUTDIR)\fsmunge.pdb"
	-@erase ".\Debug\fsmunge.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"Debug/fsmunge.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\conf_munge.sbr" \
	"$(INTDIR)\fs_munge.sbr" \
	"$(INTDIR)\fsdt_munge.sbr" \
	"$(INTDIR)\main_munge.sbr"

".\Debug\fsmunge.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=fshelper.lib kernel32.lib advapi32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\fsmunge.pdb" /debug /machine:I386 /def:".\fsmunge.def" /out:"$(OUTDIR)\fsmunge.dll" /implib:"$(OUTDIR)\fsmunge.lib" /pdbtype:sept /libpath:"..\..\Debug" 
DEF_FILE= \
	".\fsmunge.def"
LINK32_OBJS= \
	"$(INTDIR)\conf_munge.obj" \
	"$(INTDIR)\fs_munge.obj" \
	"$(INTDIR)\fsdt_munge.obj" \
	"$(INTDIR)\main_munge.obj" \
	"$(OUTDIR)\fshelper.lib"

"$(OUTDIR)\fsmunge.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("fsmunge.dep")
!INCLUDE "fsmunge.dep"
!ELSE 
!MESSAGE Warning: cannot find "fsmunge.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "fsmunge - Win32 Release" || "$(CFG)" == "fsmunge - Win32 Debug"

!IF  "$(CFG)" == "fsmunge - Win32 Release"

"fshelper - Win32 Release" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Release" 
   cd "..\..\filesys\fsmunge"

"fshelper - Win32 ReleaseCLEAN" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\filesys\fsmunge"

!ELSEIF  "$(CFG)" == "fsmunge - Win32 Debug"

"fshelper - Win32 Debug" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Debug" 
   cd "..\..\filesys\fsmunge"

"fshelper - Win32 DebugCLEAN" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\filesys\fsmunge"

!ENDIF 

SOURCE=.\conf_munge.cxx

!IF  "$(CFG)" == "fsmunge - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\conf_munge.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fsmunge - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\conf_munge.obj"	"$(INTDIR)\conf_munge.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\fs_munge.cxx

!IF  "$(CFG)" == "fsmunge - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fs_munge.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fsmunge - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fs_munge.obj"	"$(INTDIR)\fs_munge.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\fsdt_munge.cxx

!IF  "$(CFG)" == "fsmunge - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fsdt_munge.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fsmunge - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fsdt_munge.obj"	"$(INTDIR)\fsdt_munge.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\main_munge.cxx

!IF  "$(CFG)" == "fsmunge - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\main_munge.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fsmunge - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsmunge.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\main_munge.obj"	"$(INTDIR)\main_munge.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

