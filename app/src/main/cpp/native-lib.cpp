#include <jni.h>
#include <string>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>
#include "ft2build.h"
#include FT_FREETYPE_H

#define GET_STR(x) #x
static const char *textVertexShader = GET_STR(
        attribute vec4 aPosition;
        attribute
        vec2 aTextCoord;//输入的纹理坐标，会在程序指定将数据输入到该字段
        varying
        vec2 vTextCoord;
        void main() {
            vTextCoord =  vec2( aTextCoord.x, 1.0-aTextCoord.y);;
            gl_Position = aPosition;
        }
);


static const char *textFragmentShader = GET_STR(
        varying
        vec2 vTextCoord;
        uniform
        sampler2D yTexture;
        void main() {
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture2D(yTexture, vTextCoord).a);
            gl_FragColor =  vec4(1.0,0.0,0.0,1.0)*sampled;
        }
);


static unsigned int CompileShader(const unsigned int type,const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id,1,&src,nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id,GL_COMPILE_STATUS,&result);
    if(result == GL_FALSE){
        int length;
        glGetShaderiv(id,GL_INFO_LOG_LENGTH,&length);
        char* messge = (char * )alloca(length * sizeof(char));
        glGetShaderInfoLog(id,length,&length,messge);
        printf( "fild to compile %s", (type == GL_VERTEX_SHADER?"vertex":"fragment"));
        printf("%s", messge);
    }

    return id;
}

static int CreateShader(const std::string& vertexShader,const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER,vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER,fragmentShader);

    glAttachShader(program,vs);
    glAttachShader(program,fs);

    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

static FT_Face g_face;

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int shader = CreateShader(textVertexShader,textFragmentShader);
    glUseProgram(shader);

    //draw call
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    float positions[] = {
            -0.5f,-0.5f,0.0f,
            -0.5f,0.5f,0.0f,
            0.5f,0.5f,0.0f,
            -0.5f,-0.5f,0.0f,
            0.5f,0.5f,0.0f,
            0.5f,-0.5f,0.0f,
    };

    GLuint apos = static_cast<GLuint>(glGetAttribLocation(shader, "aPosition"));
    glEnableVertexAttribArray(apos);
    glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 0, positions);

    static float textCoord[] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 1.0f,
            1.0f, 0.0f
    };

    GLuint aTex = static_cast<GLuint>(glGetAttribLocation(shader, "aTextCoord"));
    glEnableVertexAttribArray(aTex);
    glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 0, textCoord);


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLint tTexture = glGetUniformLocation(shader,"yTexture");

    unsigned int texture;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    FT_Set_Pixel_Sizes(g_face, 0, 48);
    char * chinese_str = "aaa";
    FT_Load_Char(g_face, chinese_str[0], FT_LOAD_RENDER);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_ALPHA,
        g_face->glyph->bitmap.width,
        g_face->glyph->bitmap.rows,
        0,
        GL_ALPHA,
        GL_UNSIGNED_BYTE,
        g_face->glyph->bitmap.buffer
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(tTexture, 0);


    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
    eglSwapBuffers(mDisplay, mWinSurface);

}

extern "C"
JNIEXPORT void JNICALL
Java_top_niap_openglfreetypedemo_MainActivity_initAsserts(JNIEnv *env, jobject thiz,
                                                          jobject assert_manager) {

    AAssetManager *g_pAssetManager = AAssetManager_fromJava(env, assert_manager);
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        printf("ERROR::FREETYPE: Could not init FreeType Library");


    if(g_pAssetManager){
        AAsset* fontAsset = AAssetManager_open(g_pAssetManager, "arial.ttf", AASSET_MODE_UNKNOWN);
        if (fontAsset)
        {
            size_t assetLength = AAsset_getLength(fontAsset);
            char* buffer = (char*) malloc(assetLength);
            AAsset_read(fontAsset, buffer, assetLength);
            AAsset_close(fontAsset);
            FT_New_Memory_Face(ft,(const FT_Byte *)buffer,assetLength,0,&g_face);
        }

    }
}