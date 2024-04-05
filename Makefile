# Default port value to use if not overridden
PORT=9090
# Compilation flags including the PORT macro and other flags for debugging and warnings
CFLAGS=-DPORT=$(PORT) -g -Wall

# Mark 'all' and 'clean' as phony targets
.PHONY: all clean battle

# The target to compile 'battle' program
all: battle

battle: battle.c
	$(CC) $(CFLAGS) battle.c -o battle

# Clean the built program
clean:
	rm -f battle