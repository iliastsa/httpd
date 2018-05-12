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

all: $(TP_OBJ)

bin/thread_pool/%.o : src/thread_pool/%.c $(TP_DEPS) 
	$(CC) -c $(CFLAGS) -I $(TP_INCL_DIR) $< -o $@
