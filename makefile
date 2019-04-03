IDIR=include
CC=gcc

ifndef DEBUG
CFLAGS=-I$(IDIR) -Wall -Wextra -m64 -O2 -U_DEBUG
else
CFLAGS=-I$(IDIR) -Wall -Wextra -m64 -O0 -g -D_DEBUG
endif

TGT_BUILD_DIR=target
TGT_NAME=persistence
TGT_CLIENT_NAME=psql
INSTALL_PATH=/opt/persistence
INSTALL_CMD=cp -fp
INSTALL_SL_PATH=/usr/local/bin
INSTALL_CLIENT_PATH=/usr/local/bin

ALL_H=$(wildcard $(IDIR)/*/*.h)
ALL_C=$(wildcard */*.c)
ALL_SERVER_C=$(filter-out $(wildcard client/*.c),$(ALL_C))
ALL_CLIENT_C=$(filter-out $(wildcard session/*.c),$(ALL_C))
ALL_SERVER_O=$(patsubst %,$(TGT_BUILD_DIR)/%,$(ALL_SERVER_C:.c=.o))
ALL_CLIENT_O=$(patsubst %,$(TGT_BUILD_DIR)/%,$(ALL_CLIENT_C:.c=.o))



all: build_server build_client config

build_server: $(ALL_SERVER_O)
	$(CC) -o $(TGT_BUILD_DIR)/$(TGT_NAME) $^ $(CFLAGS)

build_client: $(ALL_CLIENT_O)
	$(CC) -o $(TGT_BUILD_DIR)/$(TGT_CLIENT_NAME) $^ $(CFLAGS)

$(TGT_BUILD_DIR)/%.o: %.c $(ALL_H)
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)

config: persistence.cfg
	cp $< $(TGT_BUILD_DIR)



clean:
	if [ -d $(TGT_BUILD_DIR) ]; then rm -rf $(TGT_BUILD_DIR); fi

.PHONY: clean



install:
	mkdir -p $(INSTALL_PATH)
	$(INSTALL_CMD) $(TGT_BUILD_DIR)/$(TGT_NAME) $(INSTALL_PATH)
	ln -sf $(INSTALL_SL_PATH)/$(TGT_NAME) $(INSTALL_PATH)/$(TGT_NAME)
	$(INSTALL_CMD) $(TGT_BUILD_DIR)/$(TGT_CLIENT_NAME) $(INSTALL_CLIENT_PATH)

