CC=gcc

TARGET = linux-scalability

MYFLAGS = -m32 -g -O0 -Wall -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free

#uncomment this line to compile the $(TARGET) with standatd memory allocator
#MYFLAGS = -g -O0 -Wall

# MYLIBS = libmtmm.a
MYLIBS = libmtmmSSol.a


all: $(TARGET) $(MYLIBS) bct


libmtmm.a: core_memory_allocator.c cpu_heap.c memory_allocator.c size_class.c 
	$(CC) $(MYFLAGS) -c core_memory_allocator.c cpu_heap.c memory_allocator.c size_class.c 
	ar rcu libmtmm.a core_memory_allocator.o cpu_heap.o memory_allocator.o size_class.o 
	ranlib libmtmm.a


$(TARGET): $(TARGET).c $(MYLIBS) libmtmm.a
	$(CC) $(CCFLAGS) $(MYFLAGS) $(TARGET).c $(MYLIBS) -o $(TARGET) -lpthread -lm


bct: $(MYLIBS)
	gcc -g -O0 -Wall -m32 -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free big-chanks.c libmtmm.a -o big-chanks -lm 

clean:
	rm -f $(TARGET)  *.o  libmtmm.a a.out
