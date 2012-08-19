CXX = g++
SRC_DIR=.
SRCS = $(shell find $(SRC_DIR) -maxdepth 2 -name "*.cpp")

OBJS = $(SRCS:.cpp=.o)

#FAST_FLAGS  = -02
DEBUG_FLAGS = -O3 -ggdb3
PROF_FLAGS = $(DEBUG_FLAGS) -pg

LDFLAGS = -L/usr/local/lib -ltwitcurl -lcrypt -lmcheck -lz -lpq -lxmlrpc++
CYGWIN_LDFLAGS = -L/emul/linux/x86/lib -L/usr/local/lib32 -lcrypt -lz -lpq /usr/local/lib/libXmlRpc.a
IN_CXXFLAGS = -Wall -Wno-write-strings -DLINUX -m32 -Iinclude -I/usr/include/pgsql/

BASENAME = dcastle# Base product name -- NO SPACES

.SUFFIXES: .C

.C.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

default:
	@echo "Type make followed by one of:"
	@echo ""
	@echo "debug	Make with debug libraries/flags"
	@echo "cygwin     Make with debug libraries/flags on cygwin"
	@echo "simple	Make with no libraries/flags"
	@echo "fast	Make with optimization flags"
	@echo "profile  Make with debug & profile libraries/flags"
	@echo ""

install:
	mv /srv/dcastle/bin/$(BASENAME) /srv/dcastle/bin/$(BASENAME).old
	cp $(BASENAME).debug /srv/dcastle/bin/$(BASENAME)
	cp showplayer /srv/dcastle/bin/
	cp ../scripts/lastcore.sh /srv/dcastle/bin/

all:	debug

profile:
	@$(MAKE) darkcastle PROGNAME=$(BASENAME).prof CXXFLAGS="$(IN_CXXFLAGS) $(PROF_FLAGS)"

debug:
	@$(MAKE) darkcastle PROGNAME=$(BASENAME).debug CXXFLAGS="$(IN_CXXFLAGS) $(DEBUG_FLAGS)"

cygwin:
	@$(MAKE) cygwin_darkcastle PROGNAME=$(BASENAME)_debug.exe CXXFLAGS="$(IN_CXXFLAGS) $(DEBUG_FLAGS) -fpack-struct=4"

simple:
	@$(MAKE) darkcastle PROGNAME=$(BASENAME).simple CXXFLAGS=$(IN_CXXFLAGS)

fast:
	@$(MAKE) darkcastle PROGNAME=$(BASENAME).fast CXXFLAGS="$(IN_CXXFLAGS)"
#	$(FAST_FLAGS)"

darkcastle: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBDIRS) -o $(PROGNAME) $(OBJS) $(LDFLAGS)

cygwin_darkcastle: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBDIRS) -o $(PROGNAME) $(OBJS) $(CYGWIN_LDFLAGS)

# Now for the library makefile calls
#WizLib:
#	@echo "Making libWiz.a.."
#	@cd wizard; $(MAKE) CXX=$(CXX) CXXFLAGS="$(CXXFLAGS) -I../include"

#ClassLib:
#	@echo "Making libClass.a"
#	@cd class; $(MAKE) CXX=$(CXX) CXXFLAGS="$(CXXFLAGS) -I../include"

clean:
	/bin/rm -f `find . -name '*.[oa]' -print`
