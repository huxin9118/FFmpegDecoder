LOCAL_PATH := $(call my-dir)

# FFmpeg library
include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg
LOCAL_SRC_FILES := libffmpeg.so
include $(PREBUILT_SHARED_LIBRARY)

###########################
#
# mediacodec ndk shared library
#
###########################

include $(CLEAR_VARS)
LOCAL_MODULE:= libnative_codec16
LOCAL_SRC_FILES:= libnative_codec16.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libnative_codec17
LOCAL_SRC_FILES:= libnative_codec17.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libnative_codec18
LOCAL_SRC_FILES:= libnative_codec18.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libnative_codec19
LOCAL_SRC_FILES:= libnative_codec19.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= navite_mediacodec
LOCAL_SRC_FILES:= libnavite_mediacodec.so
include $(PREBUILT_SHARED_LIBRARY)

# Program
include $(CLEAR_VARS)
LOCAL_MODULE := ffmpegdecoder
LOCAL_SRC_FILES := simplest_ffmpeg_decoder.c 
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_LDLIBS := -llog -lz
LOCAL_SHARED_LIBRARIES := ffmpeg navite_mediacodec
include $(BUILD_SHARED_LIBRARY)