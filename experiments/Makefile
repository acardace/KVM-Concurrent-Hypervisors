CC=gcc
CFLAGS=-Wall -std=c11 -I./ -O2
SRC=hyperrun.c
HDR=$(SRC:.c=.h)
EXE=hyperrun

#colors
GREEN=\033[0;32m

all: $(EXE)

hyperrun: $(SRC) $(HDR)
	@$(CC) $(CFLAGS) -o $(EXE) $(SRC)
	@echo -e "$(GREEN)[C] $(EXE)"


.PHONY: clean

clean:
	@rm $(EXE)
