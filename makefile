IDIR=include
CC=gcc

ifndef DEBUG
CFLAGS=-I$(IDIR) -Wall -Wextra -m64 -O2 -U_DEBUG
else
CFLAGS=-I$(IDIR) -Wall -Wextra -m64 -O0 -g -D_DEBUG
endif

ALL_H=$(wildcard $(IDIR)/*/*.h)
ALL_C=$(wildcard */*.c)
_ALL_O=$(ALL_C:.c=.o)
ALL_O=$(patsubst %,$(TGT_BUILD_DIR)/%,$(_ALL_O))

TGT_BUILD_DIR=target
TGT_NAME=persistence
INSTALL_PATH=/opt/persistence
INSTALL_CMD=cp -fp
INSTALL_SL_PATH=/usr/local/bin

all: $(ALL_O)
	$(CC) -o $(TGT_BUILD_DIR)/$(TGT_NAME) $^ $(CFLAGS)

$(TGT_BUILD_DIR)/%.o: %.c $(ALL_H)
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	if [ -d $(TGT_BUILD_DIR) ]; then rm -rf $(TGT_BUILD_DIR); fi

.PHONY: clean

install:
	mkdir -p $(INSTALL_PATH)
	$(INSTALL_CMD) $(TGT_BUILD_DIR)/$(TGT_NAME) $(INSTALL_PATH)
	ln -sf $(INSTALL_SL_PATH)/$(TGT_NAME) $(INSTALL_PATH)/$(TGT_NAME)

