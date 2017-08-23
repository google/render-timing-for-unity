LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := RenderTimingPlugin
LOCAL_SRC_FILES := ../../RenderTimingPlugin.cpp
LOCAL_LDLIBS := -llog -lGLESv3
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -DUNITY_ANDROID

include $(BUILD_SHARED_LIBRARY)
