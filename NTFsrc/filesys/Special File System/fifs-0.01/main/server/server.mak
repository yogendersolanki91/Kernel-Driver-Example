# Microsoft Developer Studio Generated NMAKE File, Based on server.dsp
!IF "$(CFG)" == ""
CFG=server - Win32 Debug
!MESSAGE No configuration specified. Defaulting to server - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "server - Win32 Release" && "$(CFG)" != "server - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "server.mak" CFG="server - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "server - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "server - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "server - Win32 Release"

OUTDIR=.\..\..\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\..\..\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\server.exe"

!ELSE 

ALL : "fsnfs - Win32 Release" "fsmunge - Win32 Release" "fswin32 - Win32 Release" "fshelper - Win32 Release" "helplib - Win32 Release" "$(OUTDIR)\server.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"helplib - Win32 ReleaseCLEAN" "fshelper - Win32 ReleaseCLEAN" "fswin32 - Win32 ReleaseCLEAN" "fsmunge - Win32 ReleaseCLEAN" "fsnfs - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\filter.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\nbhelp.obj"
	-@erase "$(INTDIR)\queue.obj"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\smbcom.obj"
	-@erase "$(INTDIR)\smbglue.obj"
	-@erase "$(INTDIR)\smbutil.obj"
	-@erase "$(INTDIR)\trans2.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\server.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\include\smb" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\server.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"Release/server.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=helplib.lib fshelper.lib kernel32.lib advapi32.lib user32.lib netapi32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\server.pdb" /machine:I386 /out:"$(OUTDIR)\server.exe" /libpath:"..\..\Release" 
LINK32_OBJS= \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\filter.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\nbhelp.obj" \
	"$(INTDIR)\queue.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\smbcom.obj" \
	"$(INTDIR)\smbglue.obj" \
	"$(INTDIR)\smbutil.obj" \
	"$(INTDIR)\trans2.obj" \
	"$(OUTDIR)\helplib.lib" \
	"$(OUTDIR)\fshelper.lib" \
	"$(OUTDIR)\fswin32.lib" \
	"$(OUTDIR)\fsmunge.lib" \
	"$(OUTDIR)\fsnfs.lib"

"$(OUTDIR)\server.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "server - Win32 Debug"

OUTDIR=.\..\..\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\..\..\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\server.exe" ".\Debug\server.bsc"

!ELSE 

ALL : "fsnfs - Win32 Debug" "fsmunge - Win32 Debug" "fswin32 - Win32 Debug" "fshelper - Win32 Debug" "helplib - Win32 Debug" "$(OUTDIR)\server.exe" ".\Debug\server.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"helplib - Win32 DebugCLEAN" "fshelper - Win32 DebugCLEAN" "fswin32 - Win32 DebugCLEAN" "fsmunge - Win32 DebugCLEAN" "fsnfs - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\config.sbr"
	-@erase "$(INTDIR)\filter.obj"
	-@erase "$(INTDIR)\filter.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\nbhelp.obj"
	-@erase "$(INTDIR)\nbhelp.sbr"
	-@erase "$(INTDIR)\queue.obj"
	-@erase "$(INTDIR)\queue.sbr"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\server.sbr"
	-@erase "$(INTDIR)\smbcom.obj"
	-@erase "$(INTDIR)\smbcom.sbr"
	-@erase "$(INTDIR)\smbglue.obj"
	-@erase "$(INTDIR)\smbglue.sbr"
	-@erase "$(INTDIR)\smbutil.obj"
	-@erase "$(INTDIR)\smbutil.sbr"
	-@erase "$(INTDIR)\trans2.obj"
	-@erase "$(INTDIR)\trans2.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\server.exe"
	-@erase "$(OUTDIR)\server.ilk"
	-@erase "$(OUTDIR)\server.pdb"
	-@erase ".\Debug\server.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\include\smb" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\server.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"Debug/server.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\config.sbr" \
	"$(INTDIR)\filter.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\nbhelp.sbr" \
	"$(INTDIR)\queue.sbr" \
	"$(INTDIR)\server.sbr" \
	"$(INTDIR)\smbcom.sbr" \
	"$(INTDIR)\smbglue.sbr" \
	"$(INTDIR)\smbutil.sbr" \
	"$(INTDIR)\trans2.sbr"

".\Debug\server.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=helplib.lib fshelper.lib kernel32.lib advapi32.lib user32.lib netapi32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\server.pdb" /debug /machine:I386 /out:"$(OUTDIR)\server.exe" /pdbtype:sept /libpath:"..\..\Debug" 
LINK32_OBJS= \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\filter.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\nbhelp.obj" \
	"$(INTDIR)\queue.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\smbcom.obj" \
	"$(INTDIR)\smbglue.obj" \
	"$(INTDIR)\smbutil.obj" \
	"$(INTDIR)\trans2.obj" \
	"$(OUTDIR)\helplib.lib" \
	"$(OUTDIR)\fshelper.lib" \
	"$(OUTDIR)\fswin32.lib" \
	"$(OUTDIR)\fsmunge.lib" \
	"$(OUTDIR)\fsnfs.lib"

"$(OUTDIR)\server.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("server.dep")
!INCLUDE "server.dep"
!ELSE 
!MESSAGE Warning: cannot find "server.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "server - Win32 Release" || "$(CFG)" == "server - Win32 Debug"

!IF  "$(CFG)" == "server - Win32 Release"

"helplib - Win32 Release" : 
   cd "..\helplib"
   $(MAKE) /$(MAKEFLAGS) /F .\helplib.mak CFG="helplib - Win32 Release" 
   cd "..\server"

"helplib - Win32 ReleaseCLEAN" : 
   cd "..\helplib"
   $(MAKE) /$(MAKEFLAGS) /F .\helplib.mak CFG="helplib - Win32 Release" RECURSE=1 CLEAN 
   cd "..\server"

!ELSEIF  "$(CFG)" == "server - Win32 Debug"

"helplib - Win32 Debug" : 
   cd "..\helplib"
   $(MAKE) /$(MAKEFLAGS) /F .\helplib.mak CFG="helplib - Win32 Debug" 
   cd "..\server"

"helplib - Win32 DebugCLEAN" : 
   cd "..\helplib"
   $(MAKE) /$(MAKEFLAGS) /F .\helplib.mak CFG="helplib - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\server"

!ENDIF 

!IF  "$(CFG)" == "server - Win32 Release"

"fshelper - Win32 Release" : 
   cd "..\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Release" 
   cd "..\server"

"fshelper - Win32 ReleaseCLEAN" : 
   cd "..\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Release" RECURSE=1 CLEAN 
   cd "..\server"

!ELSEIF  "$(CFG)" == "server - Win32 Debug"

"fshelper - Win32 Debug" : 
   cd "..\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Debug" 
   cd "..\server"

"fshelper - Win32 DebugCLEAN" : 
   cd "..\helpdll"
   $(MAKE) /$(MAKEFLAGS) /F .\fshelper.mak CFG="fshelper - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\server"

!ENDIF 

!IF  "$(CFG)" == "server - Win32 Release"

"fswin32 - Win32 Release" : 
   cd "..\..\filesys\fswin32"
   $(MAKE) /$(MAKEFLAGS) /F .\fswin32.mak CFG="fswin32 - Win32 Release" 
   cd "..\..\main\server"

"fswin32 - Win32 ReleaseCLEAN" : 
   cd "..\..\filesys\fswin32"
   $(MAKE) /$(MAKEFLAGS) /F .\fswin32.mak CFG="fswin32 - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\main\server"

!ELSEIF  "$(CFG)" == "server - Win32 Debug"

"fswin32 - Win32 Debug" : 
   cd "..\..\filesys\fswin32"
   $(MAKE) /$(MAKEFLAGS) /F .\fswin32.mak CFG="fswin32 - Win32 Debug" 
   cd "..\..\main\server"

"fswin32 - Win32 DebugCLEAN" : 
   cd "..\..\filesys\fswin32"
   $(MAKE) /$(MAKEFLAGS) /F .\fswin32.mak CFG="fswin32 - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\main\server"

!ENDIF 

!IF  "$(CFG)" == "server - Win32 Release"

"fsmunge - Win32 Release" : 
   cd "..\..\filesys\fsmunge"
   $(MAKE) /$(MAKEFLAGS) /F .\fsmunge.mak CFG="fsmunge - Win32 Release" 
   cd "..\..\main\server"

"fsmunge - Win32 ReleaseCLEAN" : 
   cd "..\..\filesys\fsmunge"
   $(MAKE) /$(MAKEFLAGS) /F .\fsmunge.mak CFG="fsmunge - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\main\server"

!ELSEIF  "$(CFG)" == "server - Win32 Debug"

"fsmunge - Win32 Debug" : 
   cd "..\..\filesys\fsmunge"
   $(MAKE) /$(MAKEFLAGS) /F .\fsmunge.mak CFG="fsmunge - Win32 Debug" 
   cd "..\..\main\server"

"fsmunge - Win32 DebugCLEAN" : 
   cd "..\..\filesys\fsmunge"
   $(MAKE) /$(MAKEFLAGS) /F .\fsmunge.mak CFG="fsmunge - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\main\server"

!ENDIF 

!IF  "$(CFG)" == "server - Win32 Release"

"fsnfs - Win32 Release" : 
   cd "..\..\filesys\fsnfs"
   $(MAKE) /$(MAKEFLAGS) /F .\fsnfs.mak CFG="fsnfs - Win32 Release" 
   cd "..\..\main\server"

"fsnfs - Win32 ReleaseCLEAN" : 
   cd "..\..\filesys\fsnfs"
   $(MAKE) /$(MAKEFLAGS) /F .\fsnfs.mak CFG="fsnfs - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\main\server"

!ELSEIF  "$(CFG)" == "server - Win32 Debug"

"fsnfs - Win32 Debug" : 
   cd "..\..\filesys\fsnfs"
   $(MAKE) /$(MAKEFLAGS) /F .\fsnfs.mak CFG="fsnfs - Win32 Debug" 
   cd "..\..\main\server"

"fsnfs - Win32 DebugCLEAN" : 
   cd "..\..\filesys\fsnfs"
   $(MAKE) /$(MAKEFLAGS) /F .\fsnfs.mak CFG="fsnfs - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\main\server"

!ENDIF 

SOURCE=.\config.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\config.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\config.obj"	"$(INTDIR)\config.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\filter.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\filter.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\filter.obj"	"$(INTDIR)\filter.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\main.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\nbhelp.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\nbhelp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\nbhelp.obj"	"$(INTDIR)\nbhelp.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\queue.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\queue.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\queue.obj"	"$(INTDIR)\queue.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\server.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\server.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\server.obj"	"$(INTDIR)\server.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\smbcom.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\smbcom.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\smbcom.obj"	"$(INTDIR)\smbcom.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\smbglue.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\smbglue.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\smbglue.obj"	"$(INTDIR)\smbglue.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\smbutil.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\smbutil.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\smbutil.obj"	"$(INTDIR)\smbutil.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\trans2.cxx

!IF  "$(CFG)" == "server - Win32 Release"


"$(INTDIR)\trans2.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "server - Win32 Debug"


"$(INTDIR)\trans2.obj"	"$(INTDIR)\trans2.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

