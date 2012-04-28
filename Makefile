CFLAGS := -O2 -Wall -Wextra -lz

OBJ := pdftotxt.o

all: test_pdftotxt

test_pdftotxt: ${OBJ}

clean:
	rm -f ${OBJ} test_pdftotxt
