CC=gcc
CFLAGS=-I.
DEPS = i2c_wrapper.h
OBJ = main.o i2c_wrapper.o

.PHONY: clean

%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

INA219: $(OBJ)
		$(CC) -Wall -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o INA219
