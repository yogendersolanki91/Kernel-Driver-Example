# Microsoft Developer Studio Generated NMAKE File, Based on fswin32.dsp
!IF "$(CFG)" == ""
CFG=fswin32 - Win32 Debug
!MESSAGE No configuration specified. Defaulting to fswin32 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "fswin32 - Win32 Release" && "$(CFG)" != "fswin32 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fswin32.mak" CFG="fswin32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fswin32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "fswin32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "fswin32 - Win32 Release"

OUTDIR=.\..\..\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\..\..\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\fswin32.dll"

!ELSE 

ALL : "fshelper - Win32 Release" "$(OUTDIR)\fswin32.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"fshelper - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\conf_win32.obj"
	-@erase "$(INTDIR)\fs_win32.obj"
	-@erase "$(INTDIR)\fsdt_win32.obj"
	-@erase "$(INTDIR)\main_win32.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\fswin32.dll"
	-@erase "$(OUTDIR)\fswin32.exp"
	-@erase "$(OUTDIR)\fswin32.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\fswin32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=fshelper.lib kernel32.lib advapi32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\fswin32.pdb" /machine:I386 /def:".\fswin32.def" /out:"$(OUTDIR)\fswin32.dll" /implib:"$(OUTDIR)\fswin32.lib" /libpath:"..\..\Release" 
DEF_FILE= \
	".\fswin32.def"
LINK32_OBJS= \
	"$(INTDIR)\conf_win32.obj" \
	"$(INTDIR)\fs_win32.obj" \
	"$(INTDIR)\fsdt_win32.obj" \
	"$(INTDIR)\main_win32.obj" \
	"$(OUTDIR)\fshelper.lib"

"$(OUTDIR)\fswin32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "fswin32 - Win32 Debug"

OUTDIR=.\..\..\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\..\..\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\fswin32.dll" ".\Debug\fswin32.bsc"

!ELSE 

ALL : "fshelper - Win32 Debug" "$(OUTDIR)\fswin32.dll" ".\Debug\fswin32.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"fshelper - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\conf_win32.obj"
	-@erase "$(INTDIR)\conf_win32.sbr"
	-@erase "$(INTDIR)\fs_win32.obj"
	-@erase "$(INTDIR)\fs_win32.sbr"
	-@erase "$(INTDIR)\fsdt_win32.obj"
	-@erase "$(INTDIR)\fsdt_win32.sbr"
	-@erase "$(INTDIR)\main_win32.obj"
	-@erase "$(INTDIR)\main_win32.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\fswin32.dll"
	-@erase "$(OUTDIR)\fswin32.exp"
	-@erase "$(OUTDIR)\fswin32.ilk"
	-@erase "$(OUTDIR)\fswin32.lib"
	-@erase "$(OUTDIR)\fswin32.pdb"
	-@erase ".\Debug\fswin32.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"Debug/fswin32.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\conf_win32.sbr" \
	"$(INTDIR)\fs_win32.sbr" \
	"$(INTDIR)\fsdt_win32.sbr" \
	"$(INTDIR)\main_win32.sbr"

".\Debug\fswin32.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=fshelper.lib kernel32.lib advapi32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\fswin32.pdb" /debug /machine:I386 /def:".\fswin32.def" /out:"$(OUTDIR)\fswin32.dll" /implib:"$(OUTDIR)\fswin32.lib" /pdbtype:sept /libpath:"..\..\Debug" 
DEF_FILE= \
	".\fswin32.def"
LINK32_OBJS= \
	"$(INTDIR)\conf_win32.obj" \
	"$(INTDIR)\fs_win32.obj" \
	"$(INTDIR)\fsdt_win32.obj" \
	"$(INTDIR)\main_win32.obj" \
	"$(OUTDIR)\fshelper.lib"

"$(OUTDIR)\fswin32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("fswin32.dep")
!INCLUDE "fswin32.dep"
!ELSE 
!MESSAGE Warning: cannot find "fswin32.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "fswin32 - Win32 Release" || "$(CFG)" == "fswin32 - Win32 Debug"

!IF  "$(CFG)" == "fswin32 - Win32 Release"

"fshelper - Win32 Release" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Release" 
   cd "..\..\filesys\fswin32"

"fshelper - Win32 ReleaseCLEAN" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\filesys\fswin32"

!ELSEIF  "$(CFG)" == "fswin32 - Win32 Debug"

"fshelper - Win32 Debug" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Debug" 
   cd "..\..\filesys\fswin32"

"fshelper - Win32 DebugCLEAN" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\filesys\fswin32"

!ENDIF 

SOURCE=.\conf_win32.cxx

!IF  "$(CFG)" == "fswin32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\conf_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fswin32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\conf_win32.obj"	"$(INTDIR)\conf_win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\fs_win32.cxx

!IF  "$(CFG)" == "fswin32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fs_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fswin32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fs_win32.obj"	"$(INTDIR)\fs_win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\fsdt_win32.cxx

!IF  "$(CFG)" == "fswin32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fsdt_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fswin32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fsdt_win32.obj"	"$(INTDIR)\fsdt_win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\main_win32.cxx

!IF  "$(CFG)" == "fswin32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\main_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fswin32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fswin32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\main_win32.obj"	"$(INTDIR)\main_win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

