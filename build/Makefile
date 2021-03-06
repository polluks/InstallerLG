#---------------------------------------------------------------------------------#
# Universal
#---------------------------------------------------------------------------------#
VPATH := ../src
OBJ := $(subst .c,.o,$(notdir $(wildcard $(VPATH)/*.c)))
OS := $(shell uname)
EXE := installer

#---------------------------------------------------------------------------------#
# OS
#---------------------------------------------------------------------------------#
ifeq ($(OS),Linux)
CWRN = -Wall -Wno-unused-but-set-variable
CGEN = -g -std=c99
CDEF = -D_POSIX_SOURCE -D_DEFAULT_SOURCE
else
ifeq ($(OS),Darwin)
CGEN = -g -std=c99
CWRN = -Weverything -Wno-format-nonliteral -Wno-switch-enum
CWRN += -Wno-format-security -Wno-vla -Werror
else
CC := gcc
AMIGA = true
CWRN = -Wall -Wno-pointer-sign
CGEN = -Os -std=gnu99 -DAMIGA
LDLIBS = -ldebug
I18N = strings.h
ifeq ($(OS),MorphOS)
LDLIBS += -noixemul
endif
endif
endif

CINC += -I . -I $(VPATH)
CFLAGS += $(CINC) $(CGEN) $(CWRN) $(CDEF)

#---------------------------------------------------------------------------------#
# Targets
#---------------------------------------------------------------------------------#
$(EXE): $(OBJ) parser.o lexer.o
	$(CC) $^ -o $@ $(LDLIBS)

parser.o: parser.c
	$(CC) $(CINC) $(CDEF) $(CGEN) -c $< -DYYERROR_VERBOSE

lexer.o: lexer.c
	$(CC) $(CINC) $(CDEF) $(CGEN) -c $<

parser.c: parser.y
ifneq ($(AMIGA),true)
	$(YACC) $(YFLAGS) -d $< -o $@
else
	@echo "[SKIPPED] $(YACC) $(YFLAGS) -d $< -o $@"
endif

parser.h: parser.c

lexer.c: lexer.l parser.h
ifneq ($(AMIGA),true)
	$(LEX) $(LFLAGS) -t $< > $@
else
	@echo "[SKIPPED] $(LEX) $(LFLAGS) -t $< > $@"
endif

gui.o: gui.c gui.h
	$(CC) -Os -c $< -DLLVL=2 $(CWRN)

resource.c: $(I18N)

strings.h: ../src/Installer.cd
	catcomp /src/Installer.cd CFILE /src/$@ NONUMBERS

deutsch: ../src/Installer.cd ../src/Installer.ct
	catcomp /src/Installer.cd /src/Installer.ct CATALOG Catalogs/$@/Installer.catalog

#---------------------------------------------------------------------------------#
# Misc
#---------------------------------------------------------------------------------#
.PHONY: test
test: installer
ifneq ($(AMIGA),true)
	@sh ../test/run.sh ./$(EXE) ../test $(REXTRA)
else
	@echo "[SKIPPED] @sh ../test/run.sh ./$(EXE) ../test $(REXTRA)"
endif

.PHONY: clean
clean:
	$(RM) -R *.o $(EXE) $(EXE).tmp.* leak.* err.* _??_ _???_
