# Microsoft Developer Studio Generated NMAKE File, Based on fsnfs.dsp
!IF "$(CFG)" == ""
CFG=fsnfs - Win32 Debug
!MESSAGE No configuration specified. Defaulting to fsnfs - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "fsnfs - Win32 Release" && "$(CFG)" != "fsnfs - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fsnfs.mak" CFG="fsnfs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fsnfs - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "fsnfs - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "fsnfs - Win32 Release"

OUTDIR=.\..\..\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\..\..\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\fsnfs.dll"

!ELSE 

ALL : "fshelper - Win32 Release" "$(OUTDIR)\fsnfs.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"fshelper - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\conf_nfs.obj"
	-@erase "$(INTDIR)\fs_nfs.obj"
	-@erase "$(INTDIR)\fsdt_nfs.obj"
	-@erase "$(INTDIR)\main_nfs.obj"
	-@erase "$(INTDIR)\mount_prot_clnt.obj"
	-@erase "$(INTDIR)\mount_prot_xdr.obj"
	-@erase "$(INTDIR)\nfs_prot_clnt.obj"
	-@erase "$(INTDIR)\nfs_prot_xdr.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\fsnfs.dll"
	-@erase "$(OUTDIR)\fsnfs.exp"
	-@erase "$(OUTDIR)\fsnfs.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\fsnfs.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=oncrpc.lib wsock32.lib fshelper.lib kernel32.lib advapi32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\fsnfs.pdb" /machine:I386 /def:".\fsnfs.def" /out:"$(OUTDIR)\fsnfs.dll" /implib:"$(OUTDIR)\fsnfs.lib" /libpath:"..\..\Release" /libpath:"..\..\..\frontrpc\bin\Release" 
DEF_FILE= \
	".\fsnfs.def"
LINK32_OBJS= \
	"$(INTDIR)\conf_nfs.obj" \
	"$(INTDIR)\fs_nfs.obj" \
	"$(INTDIR)\fsdt_nfs.obj" \
	"$(INTDIR)\main_nfs.obj" \
	"$(INTDIR)\mount_prot_clnt.obj" \
	"$(INTDIR)\mount_prot_xdr.obj" \
	"$(INTDIR)\nfs_prot_clnt.obj" \
	"$(INTDIR)\nfs_prot_xdr.obj" \
	"$(OUTDIR)\fshelper.lib"

"$(OUTDIR)\fsnfs.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"

OUTDIR=.\..\..\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\..\..\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\fsnfs.dll" ".\Debug\fsnfs.bsc"

!ELSE 

ALL : "fshelper - Win32 Debug" "$(OUTDIR)\fsnfs.dll" ".\Debug\fsnfs.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"fshelper - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\conf_nfs.obj"
	-@erase "$(INTDIR)\conf_nfs.sbr"
	-@erase "$(INTDIR)\fs_nfs.obj"
	-@erase "$(INTDIR)\fs_nfs.sbr"
	-@erase "$(INTDIR)\fsdt_nfs.obj"
	-@erase "$(INTDIR)\fsdt_nfs.sbr"
	-@erase "$(INTDIR)\main_nfs.obj"
	-@erase "$(INTDIR)\main_nfs.sbr"
	-@erase "$(INTDIR)\mount_prot_clnt.obj"
	-@erase "$(INTDIR)\mount_prot_clnt.sbr"
	-@erase "$(INTDIR)\mount_prot_xdr.obj"
	-@erase "$(INTDIR)\mount_prot_xdr.sbr"
	-@erase "$(INTDIR)\nfs_prot_clnt.obj"
	-@erase "$(INTDIR)\nfs_prot_clnt.sbr"
	-@erase "$(INTDIR)\nfs_prot_xdr.obj"
	-@erase "$(INTDIR)\nfs_prot_xdr.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\fsnfs.dll"
	-@erase "$(OUTDIR)\fsnfs.exp"
	-@erase "$(OUTDIR)\fsnfs.ilk"
	-@erase "$(OUTDIR)\fsnfs.lib"
	-@erase "$(OUTDIR)\fsnfs.pdb"
	-@erase ".\Debug\fsnfs.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"Debug/fsnfs.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\conf_nfs.sbr" \
	"$(INTDIR)\fs_nfs.sbr" \
	"$(INTDIR)\fsdt_nfs.sbr" \
	"$(INTDIR)\main_nfs.sbr" \
	"$(INTDIR)\mount_prot_clnt.sbr" \
	"$(INTDIR)\mount_prot_xdr.sbr" \
	"$(INTDIR)\nfs_prot_clnt.sbr" \
	"$(INTDIR)\nfs_prot_xdr.sbr"

".\Debug\fsnfs.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=oncrpc.lib wsock32.lib fshelper.lib kernel32.lib advapi32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\fsnfs.pdb" /debug /machine:I386 /def:".\fsnfs.def" /out:"$(OUTDIR)\fsnfs.dll" /implib:"$(OUTDIR)\fsnfs.lib" /pdbtype:sept /libpath:"..\..\Debug" /libpath:"..\..\..\frontrpc\bin\debug" 
DEF_FILE= \
	".\fsnfs.def"
LINK32_OBJS= \
	"$(INTDIR)\conf_nfs.obj" \
	"$(INTDIR)\fs_nfs.obj" \
	"$(INTDIR)\fsdt_nfs.obj" \
	"$(INTDIR)\main_nfs.obj" \
	"$(INTDIR)\mount_prot_clnt.obj" \
	"$(INTDIR)\mount_prot_xdr.obj" \
	"$(INTDIR)\nfs_prot_clnt.obj" \
	"$(INTDIR)\nfs_prot_xdr.obj" \
	"$(OUTDIR)\fshelper.lib"

"$(OUTDIR)\fsnfs.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("fsnfs.dep")
!INCLUDE "fsnfs.dep"
!ELSE 
!MESSAGE Warning: cannot find "fsnfs.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "fsnfs - Win32 Release" || "$(CFG)" == "fsnfs - Win32 Debug"

!IF  "$(CFG)" == "fsnfs - Win32 Release"

"fshelper - Win32 Release" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Release" 
   cd "..\..\filesys\fsnfs"

"fshelper - Win32 ReleaseCLEAN" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\filesys\fsnfs"

!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"

"fshelper - Win32 Debug" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Debug" 
   cd "..\..\filesys\fsnfs"

"fshelper - Win32 DebugCLEAN" : 
   cd "..\..\main\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\filesys\fsnfs"

!ENDIF 

SOURCE=.\conf_nfs.cxx

!IF  "$(CFG)" == "fsnfs - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\conf_nfs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\conf_nfs.obj"	"$(INTDIR)\conf_nfs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\fs_nfs.cxx

!IF  "$(CFG)" == "fsnfs - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fs_nfs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fs_nfs.obj"	"$(INTDIR)\fs_nfs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\fsdt_nfs.cxx

!IF  "$(CFG)" == "fsnfs - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fsdt_nfs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fsdt_nfs.obj"	"$(INTDIR)\fsdt_nfs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\main_nfs.cxx

!IF  "$(CFG)" == "fsnfs - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\main_nfs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\..\frontrpc\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fsnfs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\main_nfs.obj"	"$(INTDIR)\main_nfs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\mount_prot_clnt.c

!IF  "$(CFG)" == "fsnfs - Win32 Release"


"$(INTDIR)\mount_prot_clnt.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"


"$(INTDIR)\mount_prot_clnt.obj"	"$(INTDIR)\mount_prot_clnt.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\mount_prot_xdr.c

!IF  "$(CFG)" == "fsnfs - Win32 Release"


"$(INTDIR)\mount_prot_xdr.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"


"$(INTDIR)\mount_prot_xdr.obj"	"$(INTDIR)\mount_prot_xdr.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\nfs_prot_clnt.c

!IF  "$(CFG)" == "fsnfs - Win32 Release"


"$(INTDIR)\nfs_prot_clnt.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"


"$(INTDIR)\nfs_prot_clnt.obj"	"$(INTDIR)\nfs_prot_clnt.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\nfs_prot_xdr.c

!IF  "$(CFG)" == "fsnfs - Win32 Release"


"$(INTDIR)\nfs_prot_xdr.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsnfs - Win32 Debug"


"$(INTDIR)\nfs_prot_xdr.obj"	"$(INTDIR)\nfs_prot_xdr.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

