CC = gcc
SRC = vm_test.c vm.c
OF = vm

build: $(SRC)
	$(CC) -o $(OF) $(SRC)

clean:
	rm -f *.o $(OF)

rebuild: clean build
