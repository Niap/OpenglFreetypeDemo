#include <jni.h>
#include <string>
#include <map>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>
#include "ft2build.h"
#include FT_FREETYPE_H
#include "glm.hpp"
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <unistd.h>

#define GET_STR(x) #x
static const char *textVertexShader = GET_STR(
        attribute vec4 aPosition;
        varying
        vec2 vTextCoord;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(aPosition.xy, 0.0, 1.0);
            vTextCoord = aPosition.zw;
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


struct size{
    unsigned int x;
    unsigned int y;
};

struct Character {
    GLuint     TextureID;  // 字形纹理的ID
    glm::ivec2 Size;       // 字形大小
    glm::ivec2 Bearing;    // 从基准线到字形左部/顶部的偏移值
    GLuint     Advance;    // 原点距下一个字形原点的距离
};


std::map<GLchar, Character> g_characters;
static FT_Face g_face;
static int g_width;
static int g_height;
ANativeWindow * g_window;

void RenderText(unsigned int shader,std::string text, GLfloat x, GLfloat y, GLfloat scale)
{

    // 遍历文本中所有的字符
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = g_characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // 对每个字符更新VBO
        GLfloat vertices[6][4] = {
                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos,     ypos,       0.0, 1.0 },
                { xpos + w, ypos,       1.0, 1.0 },

                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos + w, ypos,       1.0, 1.0 },
                { xpos + w, ypos + h,   1.0, 0.0 }
        };

        GLuint apos = static_cast<GLuint>(glGetAttribLocation(shader, "aPosition"));
        glEnableVertexAttribArray(apos);
        glVertexAttribPointer(apos, 4, GL_FLOAT, GL_FALSE, 0, vertices);

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // 绘制四边形
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // 更新位置到下一个字形的原点，注意单位是1/64像素
        x += (ch.Advance >> 6) * scale; // 位偏移6个单位来获取单位为像素的值 (2^6 = 64)
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void loadCharTexture(){
    FT_Set_Pixel_Sizes(g_face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //禁用字节对齐限制
    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(g_face, c, FT_LOAD_RENDER)) {
            printf("ERROR::FREETYTPE: Failed to load Glyph");
        }
        // 生成纹理
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
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
        // 设置纹理选项
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // 储存字符供之后使用
        Character character = {
                texture,
                glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows),
                glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top),
                (GLuint) g_face->glyph->advance.x
        };
        g_characters.insert(std::pair<GLchar, Character>(c, character));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_top_niap_openglfreetypedemo_MainActivity_nativeSetView(JNIEnv* env, jobject obj, jobject surface) {

    // init window & context
    g_window = ANativeWindow_fromSurface(env, surface);


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
        AAsset* fontAsset = AAssetManager_open(g_pAssetManager, "Arialn.ttf", AASSET_MODE_UNKNOWN);
        if (fontAsset)
        {
            size_t assetLength = AAsset_getLength(fontAsset);
            char* buffer = (char*) malloc(assetLength);
            AAsset_read(fontAsset, buffer, assetLength);
            AAsset_close(fontAsset);
            FT_New_Memory_Face(ft,(const FT_Byte *)buffer,assetLength,0,&g_face);
        }

    }
}extern "C"
JNIEXPORT void JNICALL
Java_top_niap_openglfreetypedemo_MainActivity_nativeSetViewSize(JNIEnv *env, jobject thiz,
                                                                jint width, jint height) {
    g_height = height;
    g_width = width;


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

    EGLSurface mWinSurface = eglCreateWindowSurface(mDisplay, eglConfig, g_window, 0);
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

    loadCharTexture();

    //draw call
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(g_width), 0.0f, static_cast<GLfloat>(g_height));
    GLuint projectionId = static_cast<GLuint>(glGetUniformLocation(shader, "projection"));
    glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projection));

    RenderText(shader,"Hello World",g_width/2,g_height/2,1.0);

    eglSwapBuffers(mDisplay, mWinSurface);
}