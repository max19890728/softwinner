# Makefile for eyesee-mpp/custom_aw/apps/ts-sdv/tina_sdvcam
CUR_PATH := .
PACKAGE_TOP := ../..
EYESEE_MPP_INCLUDE:=$(STAGING_DIR)/usr/include/eyesee-mpp
EYESEE_MPP_LIBDIR:=$(STAGING_DIR)/usr/lib/eyesee-mpp
EYESEE_MINIGUI_INCLUDE:=$(STAGING_DIR)/usr/include/eyesee-minigui
# STAGING_DIR is exported in rules.mk, so it can be used directly here.
# STAGING_DIR:=.../tina-v316/out/v316-perfnor/staging_dir/target

include $(CUR_PATH)/app_config.mk
-include $(EYESEE_MPP_INCLUDE)/middleware/config/mpp_config.mk

#set source files here.
SRC_TAGS := \
    source/common \
    source/device_model \
    source/UIKit \
    source/Device \
    source/US363 \
    source/System
EXCLUDE_DIRS := source/device_model/unused/
SRC_TAGS := $(filter-out $(EXCLUDE_DIRS), $(SRC_TAGS))
ISE_SUPPORT := 0

SRCCS := \
    $(patsubst ./%, %, $(shell find $(SRC_TAGS) \( -name "*.cpp" -o -name "*.c" -o -name "*.cc" \) -a ! -name ".*")) \
    source/main.cpp

ifneq (,$(findstring $(strip $(TARGET_PRODUCT)),v536_cdr))
    PRODUCT_DIR := V5CDRTP
else
    PRODUCT_DIR := V5CDRTP
endif
#include directories
INCLUDE_DIRS := \
    $(CUR_PATH) \
    $(CUR_PATH)/source \
    $(CUR_PATH)/source/common \
    $(CUR_PATH)/source/Framework \
    $(CUR_PATH)/source/US363 \
    $(CUR_PATH)/source/minigui-cpp/ \
	$(CUR_PATH)/source/minigui-cpp/utils/ \
    $(CUR_PATH)/source/bll_presenter/remote \
    $(CUR_PATH)/source/bll_presenter/remote/interface \
    $(CUR_PATH)/source/uilayer_view/gui/minigui \
    $(CUR_PATH)/source/uilayer_view/gui/minigui/minigui-cpp \
    $(PACKAGE_TOP)/include \
    $(PACKAGE_TOP)/include/$(PRODUCT_DIR) \
	$(PACKAGE_TOP)/include/$(PRODUCT_DIR)/minigui \
	$(PACKAGE_TOP)/include/$(PRODUCT_DIR)/curl \
	$(EYESEE_MINIGUI_INCLUDE) \
    $(EYESEE_MPP_INCLUDE)/framework/include \
    $(EYESEE_MPP_INCLUDE)/framework/include/media \
    $(EYESEE_MPP_INCLUDE)/framework/include/utils \
    $(EYESEE_MPP_INCLUDE)/framework/include/media/bdii \
    $(EYESEE_MPP_INCLUDE)/framework/include/media/camera \
    $(EYESEE_MPP_INCLUDE)/framework/include/media/recorder \
    $(EYESEE_MPP_INCLUDE)/framework/include/media/player \
    $(EYESEE_MPP_INCLUDE)/framework/include/media/ise \
    $(EYESEE_MPP_INCLUDE)/framework/include/media/eis \
    $(EYESEE_MPP_INCLUDE)/framework/include/media/motion \
    $(EYESEE_MPP_INCLUDE)/framework/include/media/thumbretriever \
    $(EYESEE_MPP_INCLUDE)/middleware/include \
    $(EYESEE_MPP_INCLUDE)/middleware/include/utils \
    $(EYESEE_MPP_INCLUDE)/middleware/include/media \
    $(EYESEE_MPP_INCLUDE)/middleware/media/include/component \
    $(EYESEE_MPP_INCLUDE)/middleware/media/include/utils \
    $(EYESEE_MPP_INCLUDE)/middleware/media/include/audio \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libisp/include \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libisp/include/V4l2Camera \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libisp/isp_tuning \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/include_ai_common \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/include_eve_common \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libaiMOD/include \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libaiBDII/include \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libVLPR/include \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libeveface/include \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/include_stream \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/include_FsWriter \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/include_muxer \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libcedarc/include \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libISE \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libVideoStabilization \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libVideoStabilization/algo \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libVideoStabilization/algo/include \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/lib_aw_ai_core/include \
    $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/lib_aw_ai_mt/include \
    $(EYESEE_MPP_INCLUDE)/system/public/bt/demo/socket_test/include \
    $(EYESEE_MPP_INCLUDE)/system/public/wifi \
    $(EYESEE_MPP_INCLUDE)/system/public/rgb_ctrl \
    $(EYESEE_MPP_INCLUDE)/system/public/smartlink \
    $(EYESEE_MPP_INCLUDE)/system/public/include \
    $(EYESEE_MPP_INCLUDE)/system/public/include/utils \
    $(EYESEE_MPP_INCLUDE)/external/SQLiteCpp/include \
    $(EYESEE_MPP_INCLUDE)/external/civetweb/include \
    $(EYESEE_MPP_INCLUDE)/external/uvoice \
	$(EYESEE_MPP_INCLUDE)/external/jsoncpp-0.8.0/include \
	$(LINUX_USER_HEADERS)/include \


LOCAL_SHARED_LIBS := \
    libpthread \
    libdl \
    librt \
    libcdx_base \
    libcdx_parser \
    libcdx_stream \
    libcdx_common \
    libasound \
    liblog \
    libsample_confparser \
    libawmd
ifeq ($(MPPCFG_MOD),Y)
LOCAL_SHARED_LIBS += \
    libai_MOD \
    libCVEMiddleInterface
endif
ifeq ($(MPPCFG_EVEFACE),Y)
LOCAL_SHARED_LIBS += libeve_event
endif
ifeq ($(MPPCFG_VLPR),Y)
LOCAL_SHARED_LIBS += \
    libai_VLPR \
    libCVEMiddleInterface
endif

LOCAL_STATIC_LIBS := \
    libcamera \
    librecorder \
	libthumbretriever \
    libplayer \
    libcustomaw_media_utils \
    libaw_mpp \
	libcedarxrender \
    libmedia_utils \
    libcedarx_aencoder \
    libcedarx_tencoder \
    libaacenc \
    libmp3enc \
    libadecoder \
    libcedarxdemuxer \
    libvdecoder \
    libvideoengine \
    libawh264 \
    libawh265 \
    libawmjpegplus \
    libvencoder \
    libMemAdapter \
    libVE \
    libcdc_base \
    libISP \
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
    libhwdisplay \
    libwifi_sta \
    libwifi_ap \
    libwpa_ctl \
    librgb_ctrl \
    liblz4 \
    libluaconfig \
    liblua \
    libSQLiteCpp \
    libsqlite3 \
    libcutils \
    libvencoder \
    libvenc_codec \
    libvenc_base \
	libsmartlink \
	libgps \
	libcivetweb \
	libwebserver \
	libjsoncpp0.8 \
    libstdc++fs

#libAwNosc \

COMMON_CFLAGS := \
    $(CEDARX_EXT_CFLAGS) \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-sign-compare \
    -Wno-error=format-security \
    -Wno-pointer-arith \
    -fexceptions \
    -DHAS_BDROID_BUILDCFG -DLINUX_NATIVE -DANDROID_USE_LOGCAT=FALSE \
    -DDATABASE_IN_HOTPLUG_STORAGE -DSBC_FOR_EMBEDDED_LINUX -DONE_CAM \
    -DDEBUG_FIFO -DAUTO_TEST\
    -DCUT_HDMI_DISPLAY=2 \
	-DANDROID_CODE \
    # -DSHOW_DEBUG_INFO

EXCLUDE_FILES += \
    source/device_model/unused/device_qrencode.cpp \
    source/device_model/unused/version_update_manager.cpp
SRCCS := $(filter-out $(EXCLUDE_FILES), $(SRCCS))

ifeq ($(MPPCFG_USE_KFC),Y)
LOCAL_STATIC_LIBS += lib_hal
endif

ifeq ($(strip $(ENABLE_RTSP)), false)
EXCLUDE_FILES += \
    source/device_model/rtsp.cpp
SRCCS := $(filter-out $(EXCLUDE_FILES), $(SRCCS))
else
LOCAL_STATIC_LIBS += libTinyServer
# TARGET_SHARED_LIB += libTinyServer
COMMON_CFLAGS += -DENABLE_RTSP
endif

ifeq ($(strip $(GUI_SUPPORT)), false)
EXCLUDE_FILES += \
    source/device_model/display.cpp \
    source/device_model/media/video_player.cpp
SRCCS := $(filter-out $(EXCLUDE_FILES), $(SRCCS))
SRCCS += \
    source/bll_presenter/sample_ipc.cpp
else
SRC_TAGS := \
    source/minigui-cpp/runtime \
    source/minigui-cpp/data \
    source/minigui-cpp/debug \
    source/minigui-cpp/extra \
    source/minigui-cpp/memory \
    source/minigui-cpp/parser \
    source/minigui-cpp/resource \
    source/minigui-cpp/type \
    source/minigui-cpp/utils \
    source/minigui-cpp/widgets \
    source/minigui-cpp/window
SRCCS += source/minigui-cpp/application.cpp
SRCCS += \
    $(patsubst ./%, %, $(shell find $(SRC_TAGS) \( -name "*.cpp" -o -name "*.c" -o -name "*.cc" \) -a ! -name ".*"))

LOCAL_SHARED_LIBS += \
    libminigui_ths \
	libpng \
    libjpeg \
	libts \
    libz \

ifeq ($(strip $(DDSERVER_SUPPORT)), true)
TARGET_STATIC_LIB += \
    edog \
    libqrencode \
    libjsoncpp \
    libpaho-embed-mqtt3cc \
    libpaho-embed-mqtt3c \
    libcurl
endif
	

COMMON_CFLAGS += -DGUI_SUPPORT
endif #ifeq ($(strip $(GUI_SUPPORT)), false)

ifeq ($(strip $(BT_SUPPORT)), false)
EXCLUDE_FILES += \
    source/device_model/system/net/bluetooth_controller.cpp \
    source/bll_presenter/sample_ipc.cpp
SRCCS := $(filter-out $(EXCLUDE_FILES), $(SRCCS))
else
LOCAL_SHARED_LIBS += \
    libbtctrl \
    libbt-vendor
COMMON_CFLAGS += -DBT_SUPPORT
endif

ifeq ($(strip $(ONVIF_SUPPORT)), true)
LOCAL_SHARED_LIBS += \
   libOnvif
COMMON_CFLAGS += -DONVIF_SUPPORT -DDEBUG
endif

ifeq ($(MPPCFG_VIDEOSTABILIZATION),Y)
LOCAL_SHARED_LIBS += \
    libIRIDALABS_ViSta
COMMON_CFLAGS += -DENABLE_EIS
endif

ifeq ($(strip $(ENABLE_WATCHDOG)), true)
COMMON_CFLAGS += -DENABLE_WATCHDOG
endif

ifeq ($(strip $(BOARD_TYPE)), C26A)
COMMON_CFLAGS += -DFOX_C26A=1
endif

ifeq ($(strip $(BOARD_TYPE)), SC1663V)
COMMON_CFLAGS += -DSC1663V=1
endif

ifeq ($(strip $(SENSOR_NAME)), imx317)
COMMON_CFLAGS += -DSENSOR_IMX317=1
else ifeq ($(strip $(SENSOR_NAME)), imx278)
COMMON_CFLAGS += -DSENSOR_IMX278=1
else ifeq ($(strip $(SENSOR_NAME)), imx386)
COMMON_CFLAGS += -DSENSOR_IMX386=1
else ifeq ($(strip $(SENSOR_NAME)), imx258)
COMMON_CFLAGS += -DSENSOR_IMX258=1
else ifeq ($(strip $(SENSOR_NAME)), imx335)
COMMON_CFLAGS += -DSENSOR_IMX335=1
endif

ifeq ($(strip $(DDSERVER_SUPPORT)), true)
COMMON_CFLAGS += -DDDSERVER_SUPPORT=1
endif

ifeq ($(strip $(AES_SUPPORT)), true)
COMMON_CFLAGS += -DAES_SUPPORT=1
endif

ifeq ($(strip $(QRCODE_SUPPORT)), true)
COMMON_CFLAGS += -DQRCODE_SUPPORT=1
endif

ifeq ($(strip $(FALLOCATE_SUPPORT)), true)
COMMON_CFLAGS += -DFALLOCATE_SUPPORT=1
endif

ifeq ($(strip $(KEY_INTERACTION)), true)
COMMON_CFLAGS += -DKEY_INTERACTION=1
endif

#set dst file name: shared library, static library, execute bin.
LOCAL_TARGET_DYNAMIC :=
LOCAL_TARGET_STATIC :=
LOCAL_TARGET_BIN := sdvcam

#generate include directory flags for gcc.
inc_paths := $(foreach inc,$(filter-out -I%,$(INCLUDE_DIRS)),$(addprefix -I, $(inc))) \
                $(filter -I%, $(INCLUDE_DIRS))
#Extra flags to give to the C compiler
LOCAL_CFLAGS := $(CFLAGS) $(inc_paths) -std=c++14 -fPIC -Wall $(COMMON_CFLAGS)
#Extra flags to give to the C++ compiler
LOCAL_CXXFLAGS := $(CXXFLAGS) $(inc_paths) -std=c++14 -fPIC -Wall $(COMMON_CFLAGS)
#Extra flags to give to the C preprocessor and programs that use it (the C and Fortran compilers).
LOCAL_CPPFLAGS := $(CPPFLAGS)
#target device arch: x86, arm
LOCAL_TARGET_ARCH := $(ARCH)
#Extra flags to give to compilers when they are supposed to invoke the linker,‘ld’.
LOCAL_LDFLAGS := $(LDFLAGS)

LIB_SEARCH_PATHS := \
    $(EYESEE_MPP_LIBDIR) \
    $(PACKAGE_TOP)/lib/out \
    $(PACKAGE_TOP)/apps/cdr/webserver

empty:=
space:= $(empty) $(empty)

LOCAL_BIN_LDFLAGS := $(LOCAL_LDFLAGS) \
    $(patsubst %,-L%,$(LIB_SEARCH_PATHS)) \
    -Wl,-rpath-link=$(subst $(space),:,$(strip $(LIB_SEARCH_PATHS))) \
    -Wl,-Bstatic \
    -Wl,--start-group $(foreach n, $(LOCAL_STATIC_LIBS), -l$(patsubst lib%,%,$(patsubst %.a,%,$(notdir $(n))))) -Wl,--end-group \
    -Wl,-Bdynamic \
    $(foreach y, $(LOCAL_SHARED_LIBS), -l$(patsubst lib%,%,$(patsubst %.so,%,$(notdir $(y)))))

#generate object files
OBJS := $(SRCCS:%=%.o) #OBJS=$(patsubst %,%.o,$(SRCCS))
DEPEND_LIBS := $(wildcard $(foreach p, $(patsubst %/,%,$(LIB_SEARCH_PATHS)), \
                            $(addprefix $(p)/, \
                              $(foreach y,$(LOCAL_SHARED_LIBS),$(patsubst %,%.so,$(patsubst %.so,%,$(notdir $(y))))) \
                              $(foreach n,$(LOCAL_STATIC_LIBS),$(patsubst %,%.a,$(patsubst %.a,%,$(notdir $(n))))) \
                            ) \
                          ) \
               )

#add dynamic lib name suffix and static lib name suffix.
target_dynamic := $(if $(LOCAL_TARGET_DYNAMIC),$(addsuffix .so,$(LOCAL_TARGET_DYNAMIC)),)
target_static := $(if $(LOCAL_TARGET_STATIC),$(addsuffix .a,$(LOCAL_TARGET_STATIC)),)

#generate exe file.
.PHONY: all
all: $(LOCAL_TARGET_BIN)
	@echo ===================================
	@echo build eyesee-mpp-custom_aw-apps-ts-sdv-tina_sdvcam done
	@echo ===================================

$(target_dynamic): $(OBJS)
	$(CXX) $+ $(LOCAL_DYNAMIC_LDFLAGS) -o $@
	@echo ----------------------------
	@echo "finish target: $@"
#	@echo "object files:  $+"
#	@echo "source files:  $(SRCCS)"
	@echo ----------------------------

$(target_static): $(OBJS)
	$(AR) -rcs -o $@ $+
	@echo ----------------------------
	@echo "finish target: $@"
#	@echo "object files:  $+"
#	@echo "source files:  $(SRCCS)"
	@echo ----------------------------

$(LOCAL_TARGET_BIN): $(OBJS) $(DEPEND_LIBS)
	$(CXX) $(OBJS) $(LOCAL_BIN_LDFLAGS) -o $@
	@echo ----------------------------
	@echo "finish target: $@"
#	@echo "object files:  $+"
#	@echo "source files:  $(SRCCS)"
	@echo ----------------------------

#patten rules to generate local object files
$(filter %.cpp.o %.cc.o, $(OBJS)): %.o: %
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) -MD -MP -MF $(@:%=%.d) -c -o $@ $<
$(filter %.c.o, $(OBJS)): %.o: %
	$(CC) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) -MD -MP -MF $(@:%=%.d) -c -o $@ $<

# clean all
.PHONY: clean
clean:
	-rm -f $(OBJS) $(OBJS:%=%.d) $(target_dynamic) $(target_static) $(LOCAL_TARGET_BIN)

#add *.h prerequisites
-include $(OBJS:%=%.d)
