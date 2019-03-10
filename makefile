TGT_BUILD_DIR=target				# build files directory
TGT_NAME=persistence				# target name
INSTALL_PATH=/opt/persistence		# installaton path
INSTALL_CMD=cp -fp
INSTALL_SL_PATH=/usr/local/bin

all:


clean:
	rm -rf $(TGT_BUILD_DIR)

.PHONY: clean

install:
	@mkdir -p $(INSTALL_PATH)
	$(INSTALL_CMD) $(TGT_BUILD_DIR)/$(TGT_NAME) $(INSTALL_PATH)
	@ln -sf $(INSTALL_SL_PATH)/$(TGT_NAME) $(INSTALL_PATH)/$(TGT_NAME)

