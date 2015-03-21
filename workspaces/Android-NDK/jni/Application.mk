ifeq ($(TRAVIS_LANGUAGE),clang)
    NDK_TOOLCHAIN_VERSION := $(TRAVIS_LANGUAGE)
    $(info "Use llvm Compiler")
endif

APP_ABI := armeabi-v7a

APP_STL := stlport_static
