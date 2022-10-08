# This Makefile is intended for Windows platform!
# To create a corresponding one for other OS, just check the batch commands

OBJDIR	:= obj
SRCDIR	:= .
CC 		:= gcc
CFLAGS 	:= -g -ansi		#must add -Wall later
LDFLAGS := 				#for libraries
OBJ 	:= $(addprefix $(OBJDIR)/, \
		   globals.o \
		   error.o \
		   fileman.o \
		   regalloc.o \
		   recog.o \
		   bypass.o \
		   codegen.o \
		   dfg.o \
		   ddg.o \
		   cfg.o \
		   cdg.o \
		   jani.o )
TARGET 	:= jani

all: $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	del /s /q *.o
