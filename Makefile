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

SERVER_BINDIR   = ./bin/server/
SERVER_SRCDIR   = ./src/server/
SERVER_INCL_DIR = ./include/server/

SERVER_CFILES = server_manager.c\

SERVER_DEPS   = ./include/server/*

SERVER_SRC    = $(addprefix $(SERVER_SRCDIR), $(SERVER_CFILES))
SERVER_OBJ    = $(addprefix $(SERVER_BINDIR), $(SERVER_CFILES:.c=.o))

HTTP_BINDIR   = ./bin/http/
HTTP_SRCDIR   = ./src/http/
HTTP_INCL_DIR = ./include/http/

HTTP_CFILES = request.c\
			  parse_utils.c\

HTTP_DEPS   = ./include/http/*

HTTP_SRC    = $(addprefix $(HTTP_SRCDIR), $(HTTP_CFILES))
HTTP_OBJ    = $(addprefix $(HTTP_BINDIR), $(HTTP_CFILES:.c=.o))

COMMONS_BINDIR   = ./bin/commons/
COMMONS_SRCDIR   = ./src/commons/
COMMONS_INCL_DIR = ./include/commons/

COMMONS_CFILES = str_map.c

COMMONS_DEPS   = ./include/commons/*

COMMONS_SRC    = $(addprefix $(COMMONS_SRCDIR), $(COMMONS_CFILES))
COMMONS_OBJ    = $(addprefix $(COMMONS_BINDIR), $(COMMONS_CFILES:.c=.o))

TEST_TARGET    = test

TEST_BINDIR    = ./bin/test/
TEST_SRCDIR    = ./src/test/
TEST_INCL_DIR    = ./include/thread_pool/

TEST_CFILES = pool_test.c

TEST_DEPS   = ./include/thread_pool/*\
		      ./include/commons/*\
			  ./include/http/*\

TEST_SRC    = $(addprefix $(TEST_SRCDIR), $(TEST_CFILES))
TEST_OBJ    = $(addprefix $(TEST_BINDIR), $(TEST_CFILES:.c=.o))

TEST_TARGET = run_test

all: $(TEST_TARGET)

$(TEST_TARGET): $(TP_OBJ) $(TEST_OBJ) $(HTTP_OBJ) $(COMMONS_OBJ) $(SERVER_OBJ)
	$(CC) $(CFLAGS) -I $(TEST_INCL_DIR) $(TEST_OBJ) $(TP_OBJ) $(HTTP_OBJ) $(SERVER_OBJ) $(COMMONS_OBJ) -o run_test -$(LIBS)

bin/thread_pool/%.o : src/thread_pool/%.c $(TP_DEPS) 
	$(CC) -c $(CFLAGS) -I $(TP_INCL_DIR) $< -o $@

bin/http/%.o : src/http/%.c $(HTTP_DEPS) 
	$(CC) -c $(CFLAGS) -I $(HTTP_INCL_DIR) -I $(COMMONS_INCL_DIR) $< -o $@

bin/commons/%.o : src/commons/%.c $(COMMONS_DEPS) 
	$(CC) -c $(CFLAGS) -I $(COMMONS_INCL_DIR) $< -o $@

bin/server/%.o : src/server/%.c $(SERVER_DEPS) $(TP_DEPS)
	$(CC) -c $(CFLAGS) -I $(SERVER_INCL_DIR) -I $(TP_INCL_DIR) -I $(COMMONS_INCL_DIR) $< -o $@

bin/test/%.o : src/test/%.c $(TEST_DEPS) 
	$(CC) -c $(CFLAGS) -I $(TEST_INCL_DIR) -I $(COMMONS_INCL_DIR) -I $(SERVER_INCL_DIR) -I $(HTTP_INCL_DIR) $< -o $@

clean:
	rm -f $(TP_OBJ)
	rm -f $(COMMONS_OBJ)
	rm -f $(HTTP_OBJ)
	rm -f $(TEST_TARGET) $(TEST_OBJ)

