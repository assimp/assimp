#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "tinyusdz.hh"
#include "io-util.hh"

#include "render-ctx.hh"

#ifndef TINYUSDZ_ANDROID_LOAD_FROM_ASSETS
#error "This demo requires to load .usd file from Android Assets"
#else
#endif



namespace {

// https://stackoverflow.com/questions/41820039/jstringjni-to-stdstringc-with-utf8-characters
std::string jstring2string(JNIEnv *env, jstring jStr) {
    if (!jStr)
        return "";

    const jclass stringClass = env->GetObjectClass(jStr);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    std::string ret = std::string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
}

}

extern "C" {


JNIEXPORT jint JNICALL
Java_com_example_hellotinyusdz_MainActivity_touchMove(JNIEnv *env, jobject obj, jfloat dx, jfloat dy) {
    const float scale = 0.2f;

    example::g_gui_ctx.yaw += scale * dy;
    example::g_gui_ctx.roll -= scale * dx;

    return 0;
}

    JNIEXPORT jint JNICALL
    Java_com_example_hellotinyusdz_MainActivity_grabImage(JNIEnv *env, jobject obj, jintArray _intarray, jint width, jint height) {

      __android_log_print(ANDROID_LOG_INFO, "tinyusdz", "grabImage");

        jint *ptr = env->GetIntArrayElements(_intarray, NULL);
        int length = env->GetArrayLength(_intarray);
        if (length != (width * height)) {
            __android_log_print(ANDROID_LOG_ERROR, "tinyusdz", "Buffer size mismatch");
            return -1;
        }

        if (length != (example::g_gui_ctx.aov.width * example::g_gui_ctx.aov.height)) {
            __android_log_print(ANDROID_LOG_ERROR, "tinyusdz", "AOV size mismatch");
            return -1;
        }

        uint32_t *dest_ptr = reinterpret_cast<uint32_t *>(ptr);

        std::vector<uint32_t> src;
        example::GetRenderedImage(example::g_gui_ctx, &src);
        
        if (src.size() != length) {
          __android_log_print(ANDROID_LOG_ERROR, "tinyusdz", "GetRenderedImage failed.");
          return -1;
        }

        memcpy(dest_ptr, src.data(), sizeof(uint32_t) * length);

        return 1;
    }

    JNIEXPORT jint JNICALL
    Java_com_example_hellotinyusdz_MainActivity_renderImage(
            JNIEnv *env,
            jobject obj,
            jint width, jint height)
    {
      example::g_gui_ctx.render_width = width;
      example::g_gui_ctx.render_height = height;

      __android_log_print(ANDROID_LOG_INFO, "tinyusdz", "renderImage");
      __android_log_print(ANDROID_LOG_INFO, "tinyusdz", "draw_meshes %d", (int)example::g_gui_ctx.render_scene.draw_meshes.size());
      bool ret = example::RenderScene(example::g_gui_ctx);

      if (!ret) {
        __android_log_print(ANDROID_LOG_ERROR, "tinyusdz", "RenderScene failed.");
      }

      return 1;
    }


    /* 
     * Returns:  0 - success
     *          -1 - failed
     */
    JNIEXPORT jint JNICALL
    Java_com_example_hellotinyusdz_MainActivity_initScene(
            JNIEnv *env,
            jobject obj,
            jobject assetManager,
            jstring _filename) {

        std::string filename = jstring2string(env, _filename);
        tinyusdz::io::asset_manager = AAssetManager_fromJava(env, assetManager);

        tinyusdz::USDLoadOptions options;

        // load from Android asset folder
        example::g_gui_ctx.stage = tinyusdz::Stage(); // reset

        std::string warn, err;
        bool ret = LoadUSDCFromFile(filename, &example::g_gui_ctx.stage, &warn, &err, options);

        if (warn.size()) {
            __android_log_print(ANDROID_LOG_WARN, "tinyusdz", "USD load warning: %s", warn.c_str());
        }

        if (!ret) {
            if (err.size()) {
                __android_log_print(ANDROID_LOG_ERROR, "tinyusdz", "USD load error: %s", err.c_str());
            }
        }

        if (ret) {
            // TODO
            //__android_log_print(ANDROID_LOG_INFO, "tinyusdz", "USD loaded. #of geom_meshes: %d", int(example::g_gui_ctx.stage.geom_meshes.size()));
        }

        ret = example::SetupScene(example::g_gui_ctx);
        if (!ret) {
            __android_log_print(ANDROID_LOG_ERROR, "tinyusdz", "SetupScene failed");
            return -1;
        }


        // TODO
        //return int(example::g_gui_ctx.stage.geom_meshes.size()); // OK
        return 0;
    }
}
