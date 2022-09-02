########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC = gcc
CXXFLAGS = -std=c17 -Wall -Wno-unused-function -Isrc/include
LDFLAGS = 

# Makefile settings - Can be customized.
APPNAME = PythOwOn
EXT = .c
SRCDIR = src
OBJDIR = obj

############## Do not change anything from here downwards! #############
SRC = $(wildcard $(SRCDIR)/*$(EXT))
OBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)/%.o)

# UNIX-based OS variables & settings
RM = rm
DELOBJ = $(OBJ)
# Windows OS variables & settings
DEL = del
EXE = .exe
WDELOBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)\\%.o)

########################################################################
####################### Targets beginning here #########################
########################################################################

all: build

build:
	@$(MAKE) clean
	@[ -d obj ] || mkdir obj
	@$(MAKE) $(APPNAME)

# Builds the app
$(APPNAME): $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Building rule for .o files and its .c/.cpp in combination with all .h
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT)
	$(CC) $(CXXFLAGS) -o $@ -c $<

################### Cleaning rules for Unix-based OS ###################
# Cleans complete project
.PHONY: clean
clean:
	-$(RM) $(DELOBJ) $(DEP) $(APPNAME) 2>/dev/null || true

#################### Cleaning rules for Windows OS #####################
# Cleans complete project
.PHONY: cleanw
cleanw:
	-$(DEL) $(WDELOBJ) $(DEP) $(APPNAME) $(EXE)
