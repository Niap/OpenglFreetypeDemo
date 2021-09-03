#include <jni.h>
#include <string>
#include <android/native_window_jni.h>

extern "C" JNIEXPORT jstring JNICALL
Java_top_niap_openglfreetypedemo_MainActivity_nativeSetViewI(JNIEnv* env, jobject obj, jobject surface) {

    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);

}