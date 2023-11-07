# Program / executable name.
PROGN := gameoflife

# Flags for the build target.
#	-g Compiler flags for debugging.
CFLAGS := -g
SRCS_HEADERS := -lm

build:
	clang -o $(PROGN) $(CFLAGS) main.c $(SRCS_HEADERS)

clean:
	rm -f $(PROGN) *.o

test: CFLAGS += -DVERBOSE_FLAG
test: build
	-(./$(PROGN))

