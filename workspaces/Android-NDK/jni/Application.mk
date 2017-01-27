ifeq ($(CC),clang)
    NDK_TOOLCHAIN_VERSION := $(CC)
    $(info "Use llvm Compiler")
endif

APP_ABI := armeabi-v7a

APP_STL := stlport_static
