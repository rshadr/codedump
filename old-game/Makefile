
all: build/client build/server

include config.mk

COMMON_FILES =\
	debug

CLIENT_FILES =\
	$(COMMON_FILES)\
	gfx\
	client_main

SERVER_FILES =\
	$(COMMON_FILES)\
	server_main

CLIENT_OBJS = $(patsubst %,build/client_objs/%.o,$(CLIENT_FILES))
SERVER_OBJS = $(patsubst %,build/server_objs/%.o,$(SERVER_FILES))
CLIENT_DEPS = $(CLIENT_OBJS:.o=.d)
SERVER_DEPS = $(SERVER_OBJS:.o=.d)

-include $(CLIENT_DEPS)
-include $(SERVER_DEPS)

build/client: $(CLIENT_OBJS)
	@mkdir -p $(@D)
	$(CC) -o $@ $(CLIENT_OBJS) $(LDFLAGS_CLIENT)
	@printf "\033[1m==> client built\n\033[0m"

build/server: $(SERVER_OBJS)
	@mkdir -p $(@D)
	$(CC) -o $@ $(SERVER_OBJS) $(LDFLAGS_SERVER)
	@printf "\033[1m==> server built\n\033[0m"

build/client_objs/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -o $@ -c -MMD $(CFLAGS_CLIENT) $<

build/server_objs/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -o $@ -c -MMD $(CFLAGS_SERVER) $<

clean:
	rm -rf build

.PHONY: all clean
