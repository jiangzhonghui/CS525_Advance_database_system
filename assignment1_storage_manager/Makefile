CC := gcc
SOURCE := dberror.c storage_mgr.c test_assign1_1.c
OBJECT := dberror.o storage_mgr.o test_assign1_1.o
assignment1: $(OBJECT)
	$(CC) -o test_assign1_1 $?
%.o: %.c
	$(CC) -g -c $<
run: assignment1
	./test_assign1_1
clean:
	rm -rf test_assign1_1 *.o
