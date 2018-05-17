CC        = gcc
LIBS      = lpthread
CFLAGS    = -g3 -D DEBUG

TP_BINDIR   = ./bin/thread_pool/
TP_SRCDIR   = ./src/thread_pool/
TP_INCL_DIR = ./include/thread_pool/

TP_CFILES = thread_pool.c\
			task_queue.c\

TP_DEPS   = ./include/thread_pool/*

TP_SRC    = $(addprefix $(TP_SRCDIR), $(TP_CFILES))
TP_OBJ    = $(addprefix $(TP_BINDIR), $(TP_CFILES:.c=.o))

HTTP_BINDIR   = ./bin/http/
HTTP_SRCDIR   = ./src/http/
HTTP_INCL_DIR = ./include/http/

HTTP_CFILES = request.c\
			  parse_utils.c\

HTTP_DEPS   = ./include/http/*

HTTP_SRC    = $(addprefix $(HTTP_SRCDIR), $(HTTP_CFILES))
HTTP_OBJ    = $(addprefix $(HTTP_BINDIR), $(HTTP_CFILES:.c=.o))

TEST_TARGET    = test

TEST_BINDIR    = ./bin/test/
TEST_SRCDIR    = ./src/test/
TEST_INCL_DIR    = ./include/thread_pool/

COMMONS_INCL_DIR = ./include/commons/


TEST_CFILES = pool_test.c

TEST_DEPS   = ./include/thread_pool/*\
		      ./include/commons/*\
			  ./include/http/

TEST_SRC    = $(addprefix $(TEST_SRCDIR), $(TEST_CFILES))
TEST_OBJ    = $(addprefix $(TEST_BINDIR), $(TEST_CFILES:.c=.o))

test: $(TP_OBJ) $(TEST_OBJ) $(HTTP_OBJ)
	$(CC) $(CFLAGS) -I $(TEST_INCL_DIR) $(TEST_OBJ) $(TP_OBJ) $(HTTP_OBJ) -o run_test -$(LIBS)

bin/thread_pool/%.o : src/thread_pool/%.c $(TP_DEPS) 
	$(CC) -c $(CFLAGS) -I $(TP_INCL_DIR) $< -o $@

bin/http/%.o : src/http/%.c $(HTTP_DEPS) 
	$(CC) -c $(CFLAGS) -I $(HTTP_INCL_DIR) -I $(COMMONS_INCL_DIR) $< -o $@

bin/test/%.o : src/test/%.c $(TEST_DEPS) 
	$(CC) -c $(CFLAGS) -I $(TEST_INCL_DIR) -I $(COMMONS_INCL_DIR) -I $(HTTP_INCL_DIR) $< -o $@

clean:
	rm -f $(TP_OBJ)
	rm -f run_test $(TEST_OBJ)
	rm -f $(HTTP_OBJ)

