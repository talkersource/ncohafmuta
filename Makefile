##########################################################################
##  Makefile for the code named  'Ncohafmuta 1.2.0'  by: Cygnus         ## 
##  for unix-type compiles                 Last modified:  Mar 18, 1999 ##
##########################################################################

# Directory where the distribution lies
# Also the name of the directory this Makefile is in
DIST-DIR	= ncohafmuta

# The directory where the exectuable,
# and all other main files/dirs lie
MAIN-DIR	= .

# Place where .o files should be placed
OBJS		= objs

# Name of the executable to be made
SERVBIN		= server

# Name of executable to be made for testing..in a 'make test'
TESTBIN		= a.out

##########################################################################
# Part 1
#
# Your compiler program. Must be 
# able to grok ANSI C
##########################################################################
#
CC               = gcc
#CC               = cc
#
##########################################################################
# Part 2
#
# Warning options for compiler, if any
# -Wall -pedantic compiles cleanly with:
#	Linux, FreeBSD 2.2.7, Digital UNIX 4.0 (OSF1), OpenBSD 2.2
# Other OSes might spew warnings out at you with those options
# Last 2 WARNs are VERY VERY STRICT and also may spew tons of warnings
# Email me output, if you have time to try those options on your OS
##########################################################################
#
WARN		=
#WARN		= -Wall -pedantic
#WARN		= -Wall -Winline -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-align -Wnested-externs -Wtraditional -pedantic
#WARN		= -Wall -Wpointer-arith -Wcast-qual -Wcast-align \
		-Waggregate-return -Wstrict-prototypes \
		-Wmissing-prototypes -Wmissing-declarations \
		-Wnested-externs -Winline -Wshadow -Wtraditional \
		-pedantic
# -Wredundant-decls returns libc .h errors
# -Wwrite-strings returns "discards const" errors
# -Wconversion returns "different width due to" errors
#
##########################################################################
# Part 3
#
# Debugging or optimization options go here
# Pick one or none and comment the rest out
#   Most will use -03 as long as it's supported.
#   SystemV and HP-UX WITH cc uses NONE (the blank one)
#   FreeBSD, OpenBSD, and NetBSD-x86 use -m486 -O2
##########################################################################
#
#OPTIM = -O2 -g
#OPTIM = -m486 -O2
#OPTIM =
OPTIM = -O3
#
##########################################################################
# Part 4
#
# This section is to define compiler flags 
# for your specific system
# Uncomment the *1* DEFS line for your system 
# and leave the rest out commented out
#
#
#    For LINUX, SunOS 4.x, Sunos 5.x(Solaris), FreeBSD, NetBSD, OpenBSD,
#        Digital Unix (OSF1 Alpha), UnixWare, DYNIX 4, SCO,
#        and most HP-UX systems
DEFS         =

#
# For IRIX 4.0.x
#
#DEFS        = -cckr

#
# For some *SPECIAL* HP-UX's if the above doesn't work or produces a
# lot of warnings or errors.
# (Remember you need the ANSI compiler, not the bundled K&R one)
#  If you use this option, use "cc" compiler instead.
#
#DEFS        = -Ae

##########################################################################
# Part 5
#
# INCLUDE LIBS
#
# Uncomment the *1* LIBS line for your system
# and comment the rest out
#
# For System V OSes (Solaris, AIX, etc..)
#LIBS        = -lmalloc -lsocket -lnsl

#
# For UnixWare and DYNIX systems
#LIBS        = -lnsl -lsocket

#
# For SCO systems
#LIBS        = -lsocket

#
# For Linux systems, only if you're going to do debugging
#LIBS        = -lg

# Everything else
# (Linux, FreeBSD, NetBSD, OpenBSD, bsdi, HP-UX, Sunos 4.x, Digital osf)
LIBS         =

#####                       END OF SYSTEM TYPE DEFS                  #####
#####          YOU SHOULD NOT NEED TO EDIT ANYTHING AFTER THIS LINE  #####
##########################################################################
CFLAGS       = $(OPTIM) $(WARN) $(DEFS)

# Header files
HDRS         =

# Files used by the program
CFILES        = server.c strfuncs.c signals.c datautils.c telopts.c resolve.c

# .o version of the above
OFILES        = $(OBJS)/server.o $(OBJS)/strfuncs.o \
		$(OBJS)/signals.o $(OBJS)/datautils.o \
		$(OBJS)/telopts.o $(OBJS)/resolve.o

# Makefile arguments
#
all:            $(SERVBIN)
				@echo 'Made all'
				@echo 'If you changed JUST .h files, do: make objclean before make to force recompile'

$(SERVBIN): $(OFILES) Makefile
	  $(CC) $(CFLAGS) $(HDRS) -o $(SERVBIN) $(OFILES) $(LIBS)
	  chmod 700 $(SERVBIN) restart shutdown

$(TESTBIN): $(OFILES) Makefile
	  $(CC) $(CFLAGS) $(HDRS) -o $(TESTBIN) $(OFILES) $(LIBS)
	  chmod 700 $(TESTBIN) restart shutdown

clean:
	rm -f $(SERVBIN) $(TESTBIN) core
	rm -f $(OBJS)/*

objclean:
	rm -f $(OBJS)/*

logclean:
	rm -f $(MAIN-DIR)/syslog.* $(MAIN-DIR)/lastcommand.* lib/lastlogs

test:		$(TESTBIN)
				@echo 'Made test binary'

mkdist:
	mkdir $(DIST-DIR)
	mkdir $(DIST-DIR)/users
	mkdir $(DIST-DIR)/maildir
	mkdir $(DIST-DIR)/prodir
	mkdir $(DIST-DIR)/wizinfo
	mkdir $(DIST-DIR)/messboards
	mkdir $(DIST-DIR)/picture
	mkdir $(DIST-DIR)/restrict
	mkdir $(DIST-DIR)/newrestrict
	mkdir $(DIST-DIR)/lib
	mkdir $(DIST-DIR)/config
	mkdir $(DIST-DIR)/helpfiles
	mkdir $(DIST-DIR)/utils
	mkdir $(DIST-DIR)/utils/backupd
	mkdir $(DIST-DIR)/utils/backupd/restored
	mkdir $(DIST-DIR)/warnings
	mkdir $(DIST-DIR)/bot
	mkdir $(DIST-DIR)/www
	mkdir $(DIST-DIR)/webfiles
	mkdir $(DIST-DIR)/webfiles/userpics
	mkdir $(DIST-DIR)/tzinfo
	mkdir $(DIST-DIR)/docs
	mkdir $(DIST-DIR)/frags
	mkdir $(DIST-DIR)/objs
	mkdir $(DIST-DIR)/logs
	mkdir $(DIST-DIR)/junk ;\
	cp $(MAIN-DIR)/server.c $(DIST-DIR)/
	cp $(MAIN-DIR)/strfuncs.c $(DIST-DIR)/
	cp $(MAIN-DIR)/signals.c $(DIST-DIR)/
	cp $(MAIN-DIR)/datautils.c $(DIST-DIR)/
	cp $(MAIN-DIR)/telopts.c $(DIST-DIR)/
	cp $(MAIN-DIR)/resolve.c $(DIST-DIR)/
	touch syslog
	cp $(MAIN-DIR)/constants.h $(DIST-DIR)/
	cp $(MAIN-DIR)/protos.h $(DIST-DIR)/
	cp $(MAIN-DIR)/text.h $(DIST-DIR)/
	cp $(MAIN-DIR)/netdb.h $(DIST-DIR)/
	cp $(MAIN-DIR)/authuser.h $(DIST-DIR)/
	cp $(MAIN-DIR)/telnet.h $(DIST-DIR)/
	cp $(MAIN-DIR)/restart $(DIST-DIR)/
	cp $(MAIN-DIR)/shutdown $(DIST-DIR)/
	cp $(MAIN-DIR)/Makefile $(DIST-DIR)/
	cp $(MAIN-DIR)/READ_docs_DIR $(DIST-DIR)/
	cp -r $(MAIN-DIR)/docs/* $(DIST-DIR)/docs
	cp $(MAIN-DIR)/picture/* $(DIST-DIR)/picture
	cp -r $(MAIN-DIR)/lib/* $(DIST-DIR)/lib
	rm -f $(DIST-DIR)/lib/lastlogs
	cp -r $(MAIN-DIR)/bot/* $(DIST-DIR)/bot
	rm -fr $(DIST-DIR)/bot/Stories/*
	rm -f $(DIST-DIR)/bot/storybot
	rm -f $(DIST-DIR)/bot/botlog.*
	rm -f $(DIST-DIR)/bot/botlog
	cp -r $(MAIN-DIR)/bot/Stories/'Using spokes' $(DIST-DIR)/bot/Stories/
	cp -r $(MAIN-DIR)/www/* $(DIST-DIR)/www
	cp -r $(MAIN-DIR)/tzinfo/* $(DIST-DIR)/tzinfo
	cp $(MAIN-DIR)/config/* $(DIST-DIR)/config
	cp $(MAIN-DIR)/utils/restore $(DIST-DIR)/utils
	cp $(MAIN-DIR)/utils/backup $(DIST-DIR)/utils
	cp $(MAIN-DIR)/utils/*.c $(DIST-DIR)/utils
	cp $(MAIN-DIR)/utils/Makefile $(DIST-DIR)/utils
	cp -r $(MAIN-DIR)/webfiles/* $(DIST-DIR)/webfiles
	rm -f $(DIST-DIR)/webfiles/userpics/*
	cp $(MAIN-DIR)/helpfiles/* $(DIST-DIR)/helpfiles
	cp $(MAIN-DIR)/frags/* $(DIST-DIR)/frags
	rm -f $(DIST-DIR)/lib/activity

dist:	mkdist
	mkdir -p /tmp/ncohafmuta
	(	echo Tarring.. ;\
		rm -fr /tmp/ncohafmuta/ncohafmuta.tar.gz ;\
		tar cpf /tmp/ncohafmuta/ncohafmuta.tar $(DIST-DIR) ;\
		echo Compressing.. ;\
		gzip -9 /tmp/ncohafmuta/ncohafmuta.tar ;\
		rm -f /tmp/ncohafmuta/ncohafmuta.tar ;\
		chmod 600 /tmp/ncohafmuta/ncohafmuta.tar.gz ;\
		echo Removing buffer directory.. ;\
		rm -fr $(DIST-DIR) ;\
		echo Archive is in /tmp/ncohafmuta/ncohafmuta.tar.gz ;\
	)

distzip:	mkdist
		mkdir -p /tmp/ncohafmuta
	(	echo Zipping.. ;\
		rm -fr /tmp/ncohafmuta/ncohafmuta.zip ;\
		zip -9 -v -r /tmp/ncohafmuta/ncohafmuta.zip $(DIST-DIR) ;\
		chmod 600 /tmp/ncohafmuta/ncohafmuta.zip ;\
		echo Removing buffer directory.. ;\
		rm -fr $(DIST-DIR) ;\
		echo Archive is in /tmp/ncohafmuta/ncohafmuta.zip ;\
	)

distuu:	mkdist
	mkdir -p /tmp/ncohafmuta
	(	echo Tarring.. ;\
		rm -fr /tmp/ncohafmuta/ncohafmuta.uu ;\
		rm -fr /tmp/ncohafmuta/ncohafmuta.tar.gz ;\
		tar cpf /tmp/ncohafmuta/ncohafmuta.tar $(DIST-DIR) ;\
		echo Compressing.. ;\
		gzip -9 /tmp/ncohafmuta/ncohafmuta.tar ;\
		rm -f /tmp/ncohafmuta/ncohafmuta.tar ;\
		uuencode /tmp/ncohafmuta/ncohafmuta.tar.gz ncohafmuta.tar.gz > /tmp/ncohafmuta/ncohafmuta.uu ;\
		chmod 600 /tmp/ncohafmuta/ncohafmuta.uu ;\
		echo Removing buffer directory.. ;\
		rm -fr $(DIST-DIR) ;\
		echo Archive is in /tmp/ncohafmuta/ncohafmuta.uu ;\
	)

love:
	@echo "Not war?" ; sleep 2
	@echo "Look, I'm not equipped for that, okay?" ; sleep 2
	@echo "Contact your hardware vendor for appropriate mods."

# DO NOT REMOVE THIS LINE OR CHANGE ANYTHING AFTER IT #
$(OBJS)/server.o: server.c Makefile
	$(CC) $(CFLAGS) $(HDRS) -o $(OBJS)/server.o	-c server.c $(LIBS)
$(OBJS)/strfuncs.o: strfuncs.c Makefile
	$(CC) $(CFLAGS) $(HDRS) -o $(OBJS)/strfuncs.o	-c strfuncs.c $(LIBS)
$(OBJS)/signals.o: signals.c Makefile
	$(CC) $(CFLAGS) $(HDRS) -o $(OBJS)/signals.o	-c signals.c $(LIBS)
$(OBJS)/datautils.o: datautils.c Makefile
	$(CC) $(CFLAGS) $(HDRS) -o $(OBJS)/datautils.o	-c datautils.c $(LIBS)
$(OBJS)/telopts.o: telopts.c Makefile
	$(CC) $(CFLAGS) $(HDRS) -o $(OBJS)/telopts.o	-c telopts.c $(LIBS)
$(OBJS)/resolve.o: resolve.c Makefile
	$(CC) $(CFLAGS) $(HDRS) -o $(OBJS)/resolve.o	-c resolve.c $(LIBS)

