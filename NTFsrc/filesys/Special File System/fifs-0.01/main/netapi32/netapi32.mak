# Microsoft Developer Studio Generated NMAKE File, Based on netapi32.dsp
!IF "$(CFG)" == ""
CFG=netapi32 - Win32 Debug
!MESSAGE No configuration specified. Defaulting to netapi32 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "netapi32 - Win32 Release" && "$(CFG)" != "netapi32 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "netapi32.mak" CFG="netapi32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "netapi32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "netapi32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "netapi32 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\netapi32.dll"


CLEAN :
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\dumpsmb.obj"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\smbcom.obj"
	-@erase "$(INTDIR)\smbutil.obj"
	-@erase "$(INTDIR)\trans2.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\netapi32.dll"
	-@erase "$(OUTDIR)\netapi32.exp"
	-@erase "$(OUTDIR)\netapi32.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\include\smb" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\netapi32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\netapi32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\netapi32.pdb" /machine:I386 /def:".\netapi32.def" /out:"$(OUTDIR)\netapi32.dll" /implib:"$(OUTDIR)\netapi32.lib" 
DEF_FILE= \
	".\netapi32.def"
LINK32_OBJS= \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\dumpsmb.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\smbcom.obj" \
	"$(INTDIR)\smbutil.obj" \
	"$(INTDIR)\trans2.obj"

"$(OUTDIR)\netapi32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "netapi32 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\netapi32.dll"


CLEAN :
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\dumpsmb.obj"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\smbcom.obj"
	-@erase "$(INTDIR)\smbutil.obj"
	-@erase "$(INTDIR)\trans2.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\netapi32.dll"
	-@erase "$(OUTDIR)\netapi32.exp"
	-@erase "$(OUTDIR)\netapi32.ilk"
	-@erase "$(OUTDIR)\netapi32.lib"
	-@erase "$(OUTDIR)\netapi32.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\include\smb" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\netapi32.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\netapi32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\netapi32.pdb" /debug /machine:I386 /def:".\netapi32.def" /out:"$(OUTDIR)\netapi32.dll" /implib:"$(OUTDIR)\netapi32.lib" /pdbtype:sept 
DEF_FILE= \
	".\netapi32.def"
LINK32_OBJS= \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\dumpsmb.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\smbcom.obj" \
	"$(INTDIR)\smbutil.obj" \
	"$(INTDIR)\trans2.obj"

"$(OUTDIR)\netapi32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("netapi32.dep")
!INCLUDE "netapi32.dep"
!ELSE 
!MESSAGE Warning: cannot find "netapi32.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "netapi32 - Win32 Release" || "$(CFG)" == "netapi32 - Win32 Debug"
SOURCE=.\config.c

"$(INTDIR)\config.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dumpsmb.c

"$(INTDIR)\dumpsmb.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\hash.cpp

"$(INTDIR)\hash.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\main.c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\smbcom.c

"$(INTDIR)\smbcom.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\smbutil.c

"$(INTDIR)\smbutil.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\trans2.c

"$(INTDIR)\trans2.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

