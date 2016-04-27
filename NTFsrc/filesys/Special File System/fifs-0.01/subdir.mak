!IF "$(SUBDIR)"==""
!ERROR SUBDIR not defined
!ENDIF

!IFDEF NODEBUG
USERFLAGS=$(USERFLAGS) NODEBUG=$(NODEBUG)
!ENDIF

!UNDEF MAKE_LINE
MAKE_LINE=$(MAKE) /$(MAKEFLAGS) $(USERFLAGS)

all: $(SUBDIR)

clean:
	@for %%a in ($(SUBDIR)) do @( cd %%a & echo Processing subdir %%a... & $(MAKE_LINE) clean & cd .. & echo .)

help:
	@echo Define NODEBUG to get release targets
	@echo Otherwise, you will get debug targets
	@echo nmake all - builds binaries (default)
	@echo nmake clean - removes binaries
	@echo nmake help - prints this message

$(SUBDIR): _pseudo_target
	@cd $@
	$(MAKE_LINE)
	@cd ..

_pseudo_target:
