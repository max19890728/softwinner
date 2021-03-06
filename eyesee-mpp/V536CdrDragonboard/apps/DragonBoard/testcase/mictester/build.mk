TARGET_PATH :=$(call my-dir)

#########################################
include $(ENV_CLEAR)

include $(TARGET_TOP)/middleware/config/mpp_config.mk

TARGET_SRC := \
            mic_spk_tester.c

TARGET_INC := \
	$(TARGET_PATH)/../../../../include \
	$(TARGET_TOP)/middleware/media/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libcedarc/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/include/V4l2Camera \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/include/device \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/isp_dev \
    $(TARGET_TOP)/middleware/include/utils \
    $(TARGET_TOP)/middleware/include/media/ \
    $(TARGET_TOP)/middleware/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libISE \
    $(TARGET_TOP)/system/include \
    $(TARGET_TOP)/system/public/include \
    $(TARGET_TOP)/system/public/include/vo \
    $(TARGET_TOP)/system/public/libion/include \
    $(TARGET_TOP)/system/public/libion/kernel-headers \
    $(TARGET_TOP)/middleware/include/utils \
    $(TARGET_TOP)/middleware/include/media \
    $(TARGET_TOP)/middleware/include \
    $(TARGET_TOP)/middleware/media/include/utils \
    $(TARGET_TOP)/middleware/media/include/component \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/include/V4l2Camera \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/isp_tuning \
    $(TARGET_TOP)/middleware/media/LIBRARY/libAIE_Vda/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_stream \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_FsWriter \
    $(TARGET_TOP)/middleware/media/LIBRARY/libcedarc/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libcedarx/libcore/common/iniparser \
    $(TARGET_TOP)/middleware/sample/configfileparser \

TARGET_SHARED_LIB := \
	libsample_confparser \
    librt \
    libpthread \
    liblog \
    libion \
    libhwdisplay \
    libasound \
    libcedarxstream \
    libnormal_audio \
    libcutils \
    libcdx_common \
    libcdx_base \
    libcdx_parser \

TARGET_STATIC_LIB := \
    libaw_mpp \
    libmedia_utils \
    libcedarx_aencoder \
    libaacenc \
    libadpcmenc \
    libg711enc \
    libg726enc \
    libmp3enc \
    libadecoder \
    libcedarxdemuxer \
    libAwNosc \
    libvdecoder \
    libvideoengine \
    libawh264 \
    libawh265 \
    libawmjpegplus \
    libvencoder \
    libvenc_codec \
    libvenc_base \
    libMemAdapter \
    libVE \
    libcdc_base \
    libISP \
    libisp_dev \
    libisp_ini \
    lib_ise_mo \
    libiniparser \
    libisp_ae \
    libisp_af \
    libisp_afs \
    libisp_awb \
    libisp_base \
    libisp_gtm \
    libisp_iso \
    libisp_math \
    libisp_md \
    libisp_pltm \
    libisp_rolloff \
    libmatrix \
    libiniparser \
    libisp_ini \
    libisp_dev \
    libmuxers \
    libmp4_muxer \
    libraw_muxer \
    libmpeg2ts_muxer \
    libaac_muxer \
    libmp3_muxer \
    libffavutil \
    libFsWriter \
    libcedarxstream \
    libion \

ifeq ($(MPPCFG_USE_KFC),Y)
TARGET_STATIC_LIB += lib_hal
endif

ifeq ($(MPPCFG_EIS),Y)
TARGET_STATIC_LIB += libEIS \
    lib_eis
endif

ifeq ($(MPPCFG_ISE_MO),Y)
TARGET_STATIC_LIB += lib_ise_mo
endif

TARGET_CFLAGS += -fPIC -Wall -Wno-unused-but-set-variable -ldl
TARGET_MODULE := mictester

include $(BUILD_BIN)
#########################################

$(call copy-files-under, \
        $(TARGET_PATH)/sample_ao.conf, \
        $(TARGET_OUT)/target/usr/bin \
)

$(call copy-files-under, \
        $(TARGET_PATH)/sample_ai.conf, \
        $(TARGET_OUT)/target/usr/bin \
)

$(call copy-files-under, \
        $(TARGET_PATH)/tinyalsa/tinymix, \
        $(TARGET_OUT)/target/usr/bin \
)

$(call copy-files-under, \
        $(TARGET_PATH)/tinyalsa/tinycap, \
        $(TARGET_OUT)/target/usr/bin \
)

$(call copy-files-under, \
        $(TARGET_PATH)/tinyalsa/tinyplay, \
        $(TARGET_OUT)/target/usr/bin \
)


