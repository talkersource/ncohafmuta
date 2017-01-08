##########################################################################
##  Makefile for Ncohafmuta 1.2.0 utilities  by: Cygnus                 ## 
##                                         Last modified: Apr 12,1997   ##
##########################################################################

# Name of the executable to be made
ADDBIN           = addstruct

##########################################################################
# Your compiler program. Must be 
# able to grok ANSI C
##########################################################################
#
CC               = gcc
#CC               = cc

# Files used by the program
ADDCFILES         = addstruct.c

# Makefile arguments
#
all:		clean \
		$(ADDBIN)
				@echo 'Made all'

$(ADDBIN): $P $(OFILES) Makefile
	  $(CC) -o $(ADDBIN) $(ADDCFILES)

clean:
	rm -f $(ADDBIN)

# DO NOT REMOVE THIS LINE OR CHANGE ANYTHING AFTER IT #
addstruct.o: addstruct.c 


