LOCAL_PATH := $(call my-dir)
MAIN_LOCAL_PATH := $(LOCAL_PATH)

# =======================
# PREBUILT LIBRARIES
# =======================

include $(CLEAR_VARS)
LOCAL_MODULE            := libdobby
LOCAL_SRC_FILES         := Tools/Dobby/libraries/$(TARGET_ARCH_ABI)/libdobby.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/Tools/Dobby/
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libcurl
LOCAL_SRC_FILES := Tools/curl/curl-android-$(TARGET_ARCH_ABI)/lib/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libssl
LOCAL_SRC_FILES := Tools/curl/openssl-android-$(TARGET_ARCH_ABI)/lib/libssl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libcrypto
LOCAL_SRC_FILES := Tools/curl/openssl-android-$(TARGET_ARCH_ABI)/lib/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

# =======================
# MAIN MODULE
# =======================

include $(CLEAR_VARS)
LOCAL_MODULE := MEOW

# -------- C FLAGS --------
LOCAL_CFLAGS := -Wno-error=format-security \
                -fvisibility=hidden \
                -ffunction-sections \
                -fdata-sections \
                -fno-rtti \
                -fno-exceptions \
                -fpermissive \
                -w

# -------- CPP FLAGS --------
# NOTE: -Werror dihapus supaya warning tidak membatalkan build
LOCAL_CPPFLAGS := -Wno-error=format-security \
                  -fvisibility=hidden \
                  -ffunction-sections \
                  -fdata-sections \
                  -s \
                  -std=c++17 \
                  -Wno-error=c++11-narrowing \
                  -fms-extensions \
                  -fno-rtti \
                  -fno-exceptions \
                  -fpermissive \
                  -w

# -------- LINKER FLAGS --------
LOCAL_LDFLAGS := -Wl,--gc-sections,--strip-all
LOCAL_LDLIBS  := -llog -landroid -lEGL -lGLESv3 -lGLESv2 -lGLESv1_CM -lz

LOCAL_ARM_MODE := arm

# -------- INCLUDE PATHS --------
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/ImGui \
    $(LOCAL_PATH)/ImGui/backends \
    $(LOCAL_PATH)/Tools/curl/openssl-android-$(TARGET_ARCH_ABI)/include \
    $(LOCAL_PATH)/Tools/curl/curl-android-$(TARGET_ARCH_ABI)/include

# -------- SOURCE FILES --------
LOCAL_SRC_FILES := \
    Main.cpp \
    ImGui/imgui.cpp \
    ImGui/imgui_draw.cpp \
    ImGui/imgui_tables.cpp \
    ImGui/imgui_widgets.cpp \
    ImGui/imgui_stdlib.cpp \
    ImGui/backends/imgui_impl_opengl3.cpp \
    ImGui/backends/imgui_impl_android.cpp \
    Substrate/hde64.c \
    Substrate/SubstrateDebug.cpp \
    Substrate/SubstrateHook.cpp \
    Substrate/SubstratePosixMemory.cpp \
    Substrate/SymbolFinder.cpp \
    KittyMemory/KittyMemory.cpp \
    KittyMemory/MemoryPatch.cpp \
    KittyMemory/MemoryBackup.cpp \
    KittyMemory/KittyUtils.cpp \
    And64InlineHook/And64InlineHook.cpp \
    Il2cpp/Il2cpp.cpp \
    Il2cpp/il2cpp-class.cpp \
    Il2cpp/xdl/xdl.c \
    Il2cpp/xdl/xdl_iterate.c \
    Il2cpp/xdl/xdl_linker.c \
    Il2cpp/xdl/xdl_lzma.c \
    Il2cpp/xdl/xdl_util.c \
    Tools/MonoString.cpp \
    Tools/Tools.cpp

# -------- STATIC LIBS --------
LOCAL_STATIC_LIBRARIES := libdobby libcurl libssl libcrypto

LOCAL_CPP_FEATURES := exceptions

include $(BUILD_SHARED_LIBRARY)

# =======================
# END
# =======================
