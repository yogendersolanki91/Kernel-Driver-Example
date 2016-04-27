# Microsoft Developer Studio Generated NMAKE File, Based on fst.dsp
!IF "$(CFG)" == ""
CFG=fst - Win32 Debug
!MESSAGE No configuration specified. Defaulting to fst - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "fst - Win32 Release" && "$(CFG)" != "fst - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fst.mak" CFG="fst - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fst - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "fst - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "fst - Win32 Release"

OUTDIR=.\..\..\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\..\..\Release
# End Custom Macros

ALL : "$(OUTDIR)\fst.exe"


CLEAN :
	-@erase "$(INTDIR)\dispatch.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\tests.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\fst.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\fst.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"Release/fst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\fst.pdb" /machine:I386 /out:"$(OUTDIR)\fst.exe" 
LINK32_OBJS= \
	"$(INTDIR)\dispatch.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\tests.obj"

"$(OUTDIR)\fst.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "fst - Win32 Debug"

OUTDIR=.\..\..\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\..\..\Debug
# End Custom Macros

ALL : "$(OUTDIR)\fst.exe"


CLEAN :
	-@erase "$(INTDIR)\dispatch.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\tests.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\fst.exe"
	-@erase "$(OUTDIR)\fst.ilk"
	-@erase "$(OUTDIR)\fst.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\fst.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"Debug/fst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\fst.pdb" /debug /machine:I386 /out:"$(OUTDIR)\fst.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\dispatch.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\tests.obj"

"$(OUTDIR)\fst.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("fst.dep")
!INCLUDE "fst.dep"
!ELSE 
!MESSAGE Warning: cannot find "fst.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "fst - Win32 Release" || "$(CFG)" == "fst - Win32 Debug"
SOURCE=.\dispatch.cpp

"$(INTDIR)\dispatch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\main.cpp

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tests.cpp

"$(INTDIR)\tests.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

