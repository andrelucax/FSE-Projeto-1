CC = gcc
LDFLAGS = -lncurses -lpthread -lbcm2835 -lwiringPi
BLDDIR = .
INCDIR = $(BLDDIR)/inc2
SRCDIR = $(BLDDIR)/src2
OBJDIR = $(BLDDIR)/obj
CFLAGS = -c -Wall -I$(INCDIR) -lwiringPi
SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))
EXE = bin/bin

all: clean $(EXE) 
    
$(EXE): $(OBJ) 
	$(CC) $(OBJDIR)/*.o -o $@ $(LDFLAGS)

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f $(OBJDIR)/*.o $(EXE)
