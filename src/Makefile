#---
# New makefile for DC
# 11/24/95
#---
CXX = g++
SRC_DIR=.
SRCS = $(shell find $(SRC_DIR) -maxdepth 2 -name "*.cpp")

OBJS = $(SRCS:.cpp=.o)

#FAST_FLAGS  = -02
DEBUG_FLAGS = -ggdb2
PROF_FLAGS = $(DEBUG_FLAGS) -pg

#LDFLAGS = -lg++ -lgcc -lm -lc -lg -lWiz -lClass
#Changed when we switched to Linux Mandrake - Pir
#LDFLAGS = -lstdc++ -lcrypt -lgcc -lm -lc -lg -lWiz -lClass
LDFLAGS = -L/emul/linux/x86/lib -L/usr/local/lib32 -lcrypt -lmcheck -lz -lpq -lxmlrpc++
IN_CXXFLAGS = -Wall -I./include -DLINUX -m32 -Wno-write-strings -DUSE_SQL -I/usr/include/pgsql/

BASENAME = research# Base product name -- NO SPACES

.SUFFIXES: .C

.C.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

default:
	@echo "Type make followed by one of:"
	@echo ""
	@echo "debug	Make with debug libraries/flags"
	@echo "simple	Make with no libraries/flags"
	@echo "fast	Make with optimization flags"
	@echo "profile  Make with debug & profile libraries/flags"
	@echo ""

profile:
	@$(MAKE) darkcastle PROGNAME=$(BASENAME).prof CXXFLAGS="$(IN_CXXFLAGS) $(PROF_FLAGS)"
debug:
	@$(MAKE) darkcastle PROGNAME=$(BASENAME).debug CXXFLAGS="$(IN_CXXFLAGS) $(DEBUG_FLAGS)"

simple:
	@$(MAKE) darkcastle PROGNAME=$(BASENAME).simple CXXFLAGS=$(IN_CXXFLAGS)

fast:
	@$(MAKE) darkcastle PROGNAME=$(BASENAME).fast CXXFLAGS="$(IN_CXXFLAGS)"
#	$(FAST_FLAGS)"

darkcastle: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBDIRS) -o $(PROGNAME) $(OBJS) $(LDFLAGS)

# Now for the library makefile calls
#WizLib:
#	@echo "Making libWiz.a.."
#	@cd wizard; $(MAKE) CXX=$(CXX) CXXFLAGS="$(CXXFLAGS) -I../include"

#ClassLib:
#	@echo "Making libClass.a"
#	@cd class; $(MAKE) CXX=$(CXX) CXXFLAGS="$(CXXFLAGS) -I../include"

clean:
	/bin/rm -f `find . -name '*.[oa]' -print`
