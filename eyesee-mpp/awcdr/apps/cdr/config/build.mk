TARGET_PATH :=$(call my-dir)
include $(ENV_CLEAR)

$(info ---------copy config file-----------)
$(call copy-one-file, \
	$(TARGET_PATH)/$(BOARD_TYPE)/MiniGUI.cfg, \
	$(TARGET_OUT)/target/etc/MiniGUI.cfg \
)

$(call copy-one-file, \
	$(TARGET_PATH)/sunxi-keyboard.kl, \
	$(TARGET_OUT)/target/etc/sunxi-keyboard.kl \
)

$(call copy-one-file, \
	$(TARGET_PATH)/10-local.rules, \
	$(TARGET_OUT)/target/etc/udev/rules.d \
)

$(call copy-one-file, \
	$(TARGET_PATH)/udhcpd.conf, \
	$(TARGET_OUT)/target/etc/udhcpd.conf \
)

# copy file as ths default configuretion
$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/data/*.lua, \
	$(TARGET_OUT)/target/usr/share/app/sdv \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/data/*, \
	$(TARGET_OUT)/target/data \
)
$(shell $(TARGET_PATH)/update_build_info.sh $(TARGET_PATH))

$(call copy-one-file, \
	$(TARGET_PATH)/build_info.lua, \
	$(TARGET_OUT)/target/usr/share/app/sdv/build_info.lua \
)

$(call copy-one-file, \
	$(TARGET_PATH)/openssl, \
	$(TARGET_OUT)/target/usr/bin/openssl \
)

$(call copy-one-file, \
	$(TARGET_PATH)/tar, \
	$(TARGET_OUT)/target/usr/bin/tar \
)

$(shell $(TARGET_PATH)/make_cfg_fex.sh $(TARGET_PATH))

$(shell $(TARGET_PATH)/make_cfg_custom_fex.sh $(TARGET_PATH))

$(shell $(TARGET_PATH)/user_config.sh $(TARGET_PATH))
