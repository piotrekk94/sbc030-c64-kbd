CC ?= gcc

SRC = c64kbd.c
OBJ = $(SRC:.c=.o)

EXE = c64kbd

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJ)
