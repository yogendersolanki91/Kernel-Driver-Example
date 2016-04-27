!IF "$(PROJECT)"==""
!ERROR Missing PROJECT
!ENDIF

!UNDEF TYPE
!IFDEF NODEBUG
TYPE=Release
!ELSE
TYPE=Debug
!ENDIF

!UNDEF MAKE_LINE
MAKE_LINE=$(MAKE) /$(MAKEFLAGS) /f $(PROJECT).mak CFG="$(PROJECT) - Win32 $(TYPE)" $(USERFLAGS)

all:
	$(MAKE_LINE)

clean:
	$(MAKE_LINE) clean

help:
	@echo Define NODEBUG to get release targets
	@echo Otherwise, you will get debug targets
	@echo nmake all - builds binaries (default)
	@echo nmake clean - removes binaries
	@echo nmake help - prints this message
