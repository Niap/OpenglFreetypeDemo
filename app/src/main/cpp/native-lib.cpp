#include <jni.h>
#include <string>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window_jni.h>


extern "C" JNIEXPORT void JNICALL
Java_top_niap_openglfreetypedemo_MainActivity_nativeSetView(JNIEnv* env, jobject obj, jobject surface) {

    // init window & context
    ANativeWindow *mWindow = ANativeWindow_fromSurface(env, surface);

    EGLDisplay mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) {
        printf("egl display failed");
    }
    if (EGL_TRUE != eglInitialize(mDisplay, 0, 0)) {
        printf("eglInitialize failed");
    }

    EGLConfig eglConfig;
    EGLint configNum;
    EGLint configSpec[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };

    if (EGL_TRUE != eglChooseConfig(mDisplay, configSpec, &eglConfig, 1, &configNum)) {
        printf("eglChooseConfig failed");
    }

    EGLSurface mWinSurface = eglCreateWindowSurface(mDisplay, eglConfig, mWindow, 0);
    if (mWinSurface == EGL_NO_SURFACE) {
        printf("eglCreateWindowSurface failed");
    }

    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };
    EGLContext context = eglCreateContext(mDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT) {
        printf("eglCreateContext failed");
    }
    if (EGL_TRUE != eglMakeCurrent(mDisplay, mWinSurface, mWinSurface, context)) {
        printf("eglMakeCurrent failed");
    }

    //draw call
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    eglSwapBuffers(mDisplay, mWinSurface);

}