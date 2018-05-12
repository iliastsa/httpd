CC        = gcc
LIBS      = lpthread
CFLAGS    = -g3

TP_BINDIR   = ./bin/thread_pool/
TP_SRCDIR   = ./src/thread_pool/
TP_INCL_DIR = ./include/thread_pool/

TP_CFILES = thread_pool.c\
			task_queue.c\

TP_DEPS   = ./include/thread_pool/*

TP_SRC    = $(addprefix $(TP_SRCDIR), $(TP_CFILES))
TP_OBJ    = $(addprefix $(TP_BINDIR), $(TP_CFILES:.c=.o))

TEST_TARGET    = test

TEST_BINDIR    = ./bin/test/
TEST_SRCDIR    = ./src/test/
TEST_INCL_DIR    = ./include/thread_pool/

TEST_CFILES = pool_test.c

TEST_DEPS   = ./include/thread_pool/*

TEST_SRC    = $(addprefix $(TEST_SRCDIR), $(TEST_CFILES))
TEST_OBJ    = $(addprefix $(TEST_BINDIR), $(TEST_CFILES:.c=.o))

test: $(TP_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) -I $(TEST_INCL_DIR) $(TEST_OBJ) $(TP_OBJ) -o test -$(LIBS)

bin/thread_pool/%.o : src/thread_pool/%.c $(TP_DEPS) 
	$(CC) -c $(CFLAGS) -I $(TP_INCL_DIR) $< -o $@

bin/test/%.o : src/test/%.c $(TEST_DEPS) 
	$(CC) -c $(CFLAGS) -I $(TEST_INCL_DIR) $< -o $@

clean:
	rm -f $(TP_OBJ)
	rm -f test $(TEST_OBJ)

