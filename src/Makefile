# New DC Makefile
# Can't do much of anythin' else, so why the hell not? ;) -Urizen
# April 2004
# Based on the old one(Makefile.old)
# This requires dependencies, but it keeps the source dir ALOT cleaner, so I 
# figured it was worth it. 
CC = g++
DFLAGS = -ggdb # Debug flags (changed to gdb2 from ordinary gdb)
CFLAGS = -I./include -Wall -DLINUX # Compile flags
LFLAGS = -lcrypt -lmcheck # Link flags
PFLAGS = $(DFLAGS) -pg # Profiling flags
BASENAME = research# Base name.
OBJDIR = ./obj/

SRC_DIR =.
SRCS=$(shell find $(SRC_DIR) -name "*.cpp" -maxdepth 2) # Find files..
#I'm sure the SRCS and SRCS2 assignments could be done on one
#line, but I'm unfamiliar with how. Not like it really matters.
SRCS2=$(SRCS:.cpp='')  # .cpp = 
#OBJS1=$(SRCS:.cpp=.o))  # ./ = ./obj/
OBJS=$(shell find $(SRC?DIR) -name "*.o" -maxdepth 3) # Find Objects

default: 
#	@echo $(OBJS)
	@echo "Type make followed by one of:"
	@echo ""
	@echo "debug    Make with debug libraries/flags"
	@echo "simple   Make with no libraries/flags"
    #    @echo "fast     Make with optimization flags"
	@echo "profile  Make with debug & profile libraries/flags"
	@echo ""

simple: 
	@$(MAKE) darkcastle PROGNAME="$(BASENAME).simple" RCFLAGS="$(CFLAGS)"

debug: 
	@$(MAKE) darkcastle PROGNAME="$(BASENAME).debug" RCFLAGS="$(CFLAGS) $(DFLAGS)"

profile: 
	@$(MAKE) darkcastle PROGNAME="$(BASENAME).profile" RCFLAGS="$(CFLAGS) $(PFLAGS)"

darkcastle: #$(OBJS)
	@for X in $(SRCS2) ; do \
	        echo $$X.cpp;\
		$(CC) -c -o $(OBJDIR)$$X.o $(RCFLAGS) $$X.cpp; \
                echo "-----------------------------";done
	@$(MAKE) linkit PROGNAME="$(PROGNAME)" RCFLAGS="$(RCFLAGS)"

linkit:
	$(CC) $(RCFLAGS) -o $(PROGNAME) $(OBJS) $(LFLAGS)

clean:
	/bin/rm -f `find . -name '*.[oa]' -print`
