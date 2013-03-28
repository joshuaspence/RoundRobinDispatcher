################################################################################
# Makefile for building the program  'hostd'
#-------------------------------------------------------------------------------
# Author:	Joshua Spence
# SID:		308216350
#===============================================================================
# Targets are:
#    hostd - create the program  'hostd'.
#	 sigtrap - create the program 'sigtrap'.
#    clean - remove all object files, temporary files, target executable and tar files.
#	 debug - create the debug version of 'hostd' with capability to output useful debug information.
#	 tar - create a tar file containing all files currently in the directory.
#	 help - display the help file for instructions on how to make this project.
################################################################################

CC = gcc
CFLAGS = -W -Wall -std=c99 -pedantic -c
LDFLAGS = -W -Wall -std=c99 -pedantic
CFLAGS_DEBUG = -DDEBUG -g

SRCDIR = src
INCDIR = inc
OBJDIR = obj

TAR_FILE = Assignment2_308216350.tar

DEST = hostd
FILES = hostd PCB MAB RAS input
OBJS = $(FILES:%=$(OBJDIR)/%.o)
INCS = $(FILES:%=$(INCDIR)/%.h) $(INCDIR)/boolean.h $(INCDIR)/output.h
SRCS = $(FILES:%=$(SRCDIR)/%.c)

# Create the program  'hostd'
$(DEST): $(OBJS)
	@echo "====================================================="
	@echo "Linking the target $@"
	@echo "====================================================="
	$(CC) $(LDFLAGS) $^ -o $@
	@echo "------------------- Link finished -------------------"
	@echo

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h $(INCDIR)/output.h $(INCDIR)/boolean.h
	@echo "====================================================="
	@echo "Compiling $<"
	@echo "====================================================="
# Create OBJDIR if it doesn't exist
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $< -o $(OBJDIR)/$*.o
	@echo "--------------- Compilation finished ----------------"
	@echo

# The following targets are phony
.PHONY: clean help

# Remove all object files, temporary files, backup files, striped files, target executable and tar files
clean:
	@echo "====================================================="
	@echo "Cleaning directory."
	@echo "====================================================="
	rm -rfv $(OBJDIR)/*.o *~ $(INCDIR)/*~ $(INCDIR_BACKUP) $(INCDIR_STRIPED) $(SRCDIR)/*~ $(SRCDIR_BACKUP) $(SRCDIR_STRIPED) $(DEST) $(TAR_FILE) $(STRIPCC_ERROR_FILE) sigtrap
	@echo "------------------ Clean finished -------------------"
	@echo

# Create a tar file containing all files currently in the directory
tar:
	@echo "====================================================="
	@echo "Creating tar file."
	@echo "====================================================="
# Delete existing tar file
	@rm -f $(TAR_FILE)
	tar cfv $(TAR_FILE) ./*
	@echo "-------------- Tar creation finished ----------------"
	@echo

# Display the help file for instructions on how to make this project
help:
	@echo "=========================================================================================================="
	@echo "Makefile for building the program  'hostd'"
	@echo "=========================================================================================================="
	@echo "Targets are:"
	@echo "    hostd                create the program  'hostd'."
	@echo "    sigtrap              create the program  'sigtrap'."
	@echo "    clean                remove all object files, temporary files, target executable and tar files."
	@echo "    debug                create the debug version of 'hostd' with useful debug information."
	@echo "    tar                  create a tar file containing all files currently in the directory."
	@echo "    help                 display the help file for instructions on how to make this project."
	@echo
	@echo "Use:"
	@echo "    make                 create program 'hostd'."
	@echo "    make hostd           same as make."
	@echo "    make sigtrap         create program 'sigtrap'."
	@echo "    make debug           create program 'hostd' with capability to output useful debug information."
	@echo "    make clean           remove all object files, temporary files, target executable and tar files."
	@echo "    make clean && make tar"
	@echo "                         create a tar file containing the files required for assignment submission."
	@echo "    make sigtrap && make hostd"
	@echo "                         compile the programs 'hostd' and 'sigtrap'."
	@echo "    make help            display the help file."
	@echo "----------------------------------------------------------------------------------------------------------"
	@echo

# Create the debug version of 'hostd' with capability to output useful debug information
debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(DEST)

# Sigtrap
sigtrap: $(OBJDIR)/sigtrap.o
	@echo "====================================================="
	@echo "Linking the target $@"
	@echo "====================================================="
	$(CC) $(LDFLAGS) $^ -o $@
	@echo "------------------- Link finished -------------------"
	@echo

$(OBJDIR)/sigtrap.o: $(SRCDIR)/sigtrap.c
	@echo "====================================================="
	@echo "Compiling $<"
	@echo "====================================================="
# Create OBJDIR if it doesn't exist
	@mkdir -p $(OBJDIR)
	$(CC) -W -Wall -pedantic -c $< -o $*.o
	@echo "--------------- Compilation finished ----------------"
	@echo
