#include <GLES3/gl3.h>
#include <android/log.h>
#include <jni.h>

#include <mbgl/style/layers/custom_layer.hpp>

#include <memory>

namespace {

constexpr const char* kLogTag = "OovyTriangle";

void logShaderError(GLuint shader, const char* label) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length <= 1) {
        return;
    }

    auto log = std::make_unique<GLchar[]>(static_cast<std::size_t>(length));
    glGetShaderInfoLog(shader, length, nullptr, log.get());

    __android_log_print(
        ANDROID_LOG_ERROR,
        kLogTag,
        "%s shader error: %s",
        label,
        log.get());
}

GLuint compileShader(GLenum type, const char* source, const char* label) {
    const GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (compiled != GL_TRUE) {
        logShaderError(shader, label);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

class OovyTriangleHost final : public mbgl::style::CustomLayerHost {
public:
    void initialize() override {
        __android_log_write(
            ANDROID_LOG_INFO,
            kLogTag,
            "initialize");

        static constexpr const char* vertexShaderSource = R"(
            #version 300 es

            layout(location = 0) in vec2 a_position;

            void main() {
                gl_Position = vec4(a_position, 0.0, 1.0);
            }
        )";

        static constexpr const char* fragmentShaderSource = R"(
            #version 300 es

            precision mediump float;

            out vec4 fragmentColor;

            void main() {
                fragmentColor = vec4(
                    0.0,
                    0.81,
                    0.91,
                    0.95
                );
            }
        )";

        const GLuint vertexShader =
            compileShader(
                GL_VERTEX_SHADER,
                vertexShaderSource,
                "vertex");

        const GLuint fragmentShader =
            compileShader(
                GL_FRAGMENT_SHADER,
                fragmentShaderSource,
                "fragment");

        if (vertexShader == 0 || fragmentShader == 0) {
            if (vertexShader != 0) {
                glDeleteShader(vertexShader);
            }

            if (fragmentShader != 0) {
                glDeleteShader(fragmentShader);
            }

            return;
        }

        program = glCreateProgram();

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);

        if (linked != GL_TRUE) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

            if (length > 1) {
                auto log =
                    std::make_unique<GLchar[]>(
                        static_cast<std::size_t>(length));

                glGetProgramInfoLog(
                    program,
                    length,
                    nullptr,
                    log.get());

                __android_log_print(
                    ANDROID_LOG_ERROR,
                    kLogTag,
                    "Program link error: %s",
                    log.get());
            }

            glDeleteProgram(program);
            program = 0;
            return;
        }

        static constexpr GLfloat vertices[] = {
             0.0f,  0.32f,
            -0.28f, -0.24f,
             0.28f, -0.24f,
        };

        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);

        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertices),
            vertices,
            GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            2 * sizeof(GLfloat),
            nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void render(
        const mbgl::style::CustomLayerRenderParameters&
    ) override {
        if (program == 0 || vertexArray == 0) {
            return;
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);

        glEnable(GL_BLEND);
        glBlendFunc(
            GL_SRC_ALPHA,
            GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(program);
        glBindVertexArray(vertexArray);

        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void contextLost() override {
        __android_log_write(
            ANDROID_LOG_WARN,
            kLogTag,
            "contextLost");

        program = 0;
        vertexArray = 0;
        vertexBuffer = 0;
    }

    void deinitialize() override {
        __android_log_write(
            ANDROID_LOG_INFO,
            kLogTag,
            "deinitialize");

        if (vertexBuffer != 0) {
            glDeleteBuffers(1, &vertexBuffer);
            vertexBuffer = 0;
        }

        if (vertexArray != 0) {
            glDeleteVertexArrays(1, &vertexArray);
            vertexArray = 0;
        }

        if (program != 0) {
            glDeleteProgram(program);
            program = 0;
        }
    }

private:
    GLuint program = 0;
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
};

} // namespace

extern "C"
JNIEXPORT jlong JNICALL
Java_com_oovy_maplibre_style_layers_OovyTriangleLayer_nativeCreateHost(
    JNIEnv*,
    jclass
) {
    auto host = std::make_unique<OovyTriangleHost>();

    return reinterpret_cast<jlong>(
        host.release());
}