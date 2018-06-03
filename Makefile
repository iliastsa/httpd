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
				request_manager.c\
				command_manager.c\
				main.c\

SERVER_DEPS   = ./include/server/*

SERVER_SRC    = $(addprefix $(SERVER_SRCDIR), $(SERVER_CFILES))
SERVER_OBJ    = $(addprefix $(SERVER_BINDIR), $(SERVER_CFILES:.c=.o))

SERVER_TARGET = myhttpd

HTTP_BINDIR   = ./bin/http/
HTTP_SRCDIR   = ./src/http/
HTTP_INCL_DIR = ./include/http/

HTTP_CFILES = request.c\
			  parse_utils.c\
			  response_messages.c\

HTTP_DEPS   = ./include/http/*

HTTP_SRC    = $(addprefix $(HTTP_SRCDIR), $(HTTP_CFILES))
HTTP_OBJ    = $(addprefix $(HTTP_BINDIR), $(HTTP_CFILES:.c=.o))

COMMONS_BINDIR   = ./bin/commons/
COMMONS_SRCDIR   = ./src/commons/
COMMONS_INCL_DIR = ./include/commons/

COMMONS_CFILES = str_map.c\
				 utils.c\
				 network_io.c\

COMMONS_DEPS   = ./include/commons/*

COMMONS_SRC    = $(addprefix $(COMMONS_SRCDIR), $(COMMONS_CFILES))
COMMONS_OBJ    = $(addprefix $(COMMONS_BINDIR), $(COMMONS_CFILES:.c=.o))

all: $(SERVER_TARGET)

$(SERVER_TARGET) : $(TP_OBJ) $(HTTP_OBJ) $(COMMONS_OBJ) $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(TP_OBJ) $(HTTP_OBJ) $(SERVER_OBJ) $(COMMONS_OBJ) -o $(SERVER_TARGET) -$(LIBS)

bin/thread_pool/%.o : src/thread_pool/%.c $(TP_DEPS) 
	$(CC) -c $(CFLAGS) -I $(TP_INCL_DIR) $< -o $@

bin/http/%.o : src/http/%.c $(HTTP_DEPS) 
	$(CC) -c $(CFLAGS) -I $(HTTP_INCL_DIR) -I $(COMMONS_INCL_DIR) $< -o $@

bin/commons/%.o : src/commons/%.c $(COMMONS_DEPS) 
	$(CC) -c $(CFLAGS) -I $(COMMONS_INCL_DIR) $< -o $@

bin/server/%.o : src/server/%.c $(SERVER_DEPS) $(TP_DEPS)
	$(CC) -c $(CFLAGS) -I $(HTTP_INCL_DIR) -I $(SERVER_INCL_DIR) -I $(TP_INCL_DIR) -I $(COMMONS_INCL_DIR) $< -o $@

clean:
	rm -f $(TP_OBJ) $(COMMONS_OBJ) $(HTTP_OBJ) $(SERVER_OBJ) $(SERVER_TARGET)
