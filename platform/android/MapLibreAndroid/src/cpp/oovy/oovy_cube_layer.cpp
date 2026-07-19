#include <GLES3/gl3.h>
#include <android/log.h>
#include <jni.h>
#include <cstdint>
#include <limits>
#include <utility>

#include <mbgl/style/layers/custom_layer.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/projection.hpp>
#include <mbgl/util/mat4.hpp>

#include "oovy_glb_inspector.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

namespace {

constexpr const char* kLogTag = "OovyCube";

constexpr double kLatitude = 48.8582599;
constexpr double kLongitude = 2.2945006;

constexpr float kTargetModelHeightMeters = 330.0f;
constexpr float kTargetModelFootprintMeters = 124.9f;

// Rotation horaire dans le repère local X=est, Y=sud.
// 45° constitue le point de départ pour aligner les piliers avec l'empreinte.
constexpr float kModelYawDegrees = 43.9f;

constexpr float kPi = 3.14159265358979323846f;

GLuint compileShader(
    GLenum type,
    const char* source,
    const char* label
) {
    const GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 1) {
        auto log =
            std::make_unique<GLchar[]>(
                static_cast<std::size_t>(length));

        glGetShaderInfoLog(
            shader,
            length,
            nullptr,
            log.get());

        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "%s shader error: %s",
            label,
            log.get());
    }

    glDeleteShader(shader);
    return 0;
}

class OovyCubeHost final
    : public mbgl::style::CustomLayerHost {
public:
    explicit OovyCubeHost(
        oovy::GlbMeshData meshData
    )
        : mesh(std::move(meshData)) {
    }
    void initialize() override {
        __android_log_write(
            ANDROID_LOG_INFO,
            kLogTag,
            "initialize");

        static constexpr const char* vertexShaderSource =
            R"(#version 300 es
        layout(location = 0) in vec3 a_position;

        uniform mat4 u_rebased_projection;
        uniform float u_pixels_per_meter;
        uniform vec2 u_model_horizontal_scale;
        uniform float u_model_yaw_radians;

        out vec3 v_local_position;

        void main() {
            vec3 localMeters = a_position;

            vec2 scaledHorizontal = vec2(
                localMeters.x * u_model_horizontal_scale.x,
                localMeters.y * u_model_horizontal_scale.y
            );

            float yawCos = cos(u_model_yaw_radians);
            float yawSin = sin(u_model_yaw_radians);

            vec2 rotatedHorizontal = vec2(
                yawCos * scaledHorizontal.x -
                    yawSin * scaledHorizontal.y,

                yawSin * scaledHorizontal.x +
                    yawCos * scaledHorizontal.y
            );

            vec3 positionedMeters = vec3(
                rotatedHorizontal.x,
                rotatedHorizontal.y,
                localMeters.z
            );

            vec3 localWorldPosition = vec3(
                positionedMeters.x * u_pixels_per_meter,
                positionedMeters.y * u_pixels_per_meter,
                positionedMeters.z
            );

            v_local_position = positionedMeters;

            gl_Position =
                u_rebased_projection *
                vec4(localWorldPosition, 1.0);
        }
        )";

        static constexpr const char* fragmentShaderSource =
            R"(#version 300 es
        precision highp float;

        in vec3 v_local_position;

        out vec4 fragmentColor;

        void main() {
            vec3 normal = normalize(
                cross(
                    dFdx(v_local_position),
                    dFdy(v_local_position)
                )
            );

            vec3 lightDirection =
                normalize(vec3(-0.4, -0.6, 0.7));

            float diffuse =
                0.35 +
                0.65 *
                abs(dot(normal, lightDirection));

            vec3 baseColor =
                vec3(0.0, 0.72, 0.84);

            fragmentColor =
                vec4(baseColor * diffuse, 1.0);
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
            logProgramError();
            glDeleteProgram(program);
            program = 0;
            return;
        }

        projectionUniform =
            glGetUniformLocation(
                program,
                "u_rebased_projection");

        pixelsPerMeterUniform =
            glGetUniformLocation(
                program,
                "u_pixels_per_meter");
        
        horizontalScaleUniform =
            glGetUniformLocation(
                program,
                "u_model_horizontal_scale");

        modelYawUniform =
            glGetUniformLocation(
                program,
                "u_model_yaw_radians");

        createGeometry();
    }

    void render(
        const mbgl::style::CustomLayerRenderParameters&
            parameters
    ) override {
        if (
            program == 0 ||
            vertexArray == 0 ||
            indexBuffer == 0 ||
            drawIndexCount <= 0
        ) {
            return;
        }

        const double scale =
            std::pow(2.0, parameters.zoom);

        const auto anchor =
            mbgl::Projection::project(
                mbgl::LatLng{
                    kLatitude,
                    kLongitude,
                },
                scale);

        const double metersPerPixel =
            mbgl::Projection::
                getMetersPerPixelAtLatitude(
                    kLatitude,
                    parameters.zoom);

        if (metersPerPixel <= 0.0) {
            return;
        }

        const float pixelsPerMeter =
            static_cast<float>(
                1.0 / metersPerPixel);

        /*
        * La translation est appliquée en double avant la conversion en float.
        * Le shader peut ensuite travailler autour de l'origine locale (0, 0, 0).
        */
        mbgl::mat4 rebasedProjection =
            parameters.projectionMatrix;

        mbgl::matrix::translate(
            rebasedProjection,
            rebasedProjection,
            anchor.x,
            anchor.y,
            0.0);

        std::array<GLfloat, 16> projection{};

        std::transform(
            rebasedProjection.begin(),
            rebasedProjection.end(),
            projection.begin(),
            [](double value) {
                return static_cast<GLfloat>(value);
            });

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);

        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        glUseProgram(program);
        glBindVertexArray(vertexArray);

        glUniformMatrix4fv(
            projectionUniform,
            1,
            GL_FALSE,
            projection.data());        

        glUniform1f(
            pixelsPerMeterUniform,
            pixelsPerMeter);
        
        if (
            mesh.widthMeters <= 0.0f ||
            mesh.depthMeters <= 0.0f
        ) {
            return;
        }

        const float horizontalScaleX =
            kTargetModelFootprintMeters /
            mesh.widthMeters;

        const float horizontalScaleY =
            kTargetModelFootprintMeters /
            mesh.depthMeters;

        const float modelYawRadians =
            kModelYawDegrees *
            kPi /
            180.0f;

        glUniform2f(
            horizontalScaleUniform,
            horizontalScaleX,
            horizontalScaleY);

        glUniform1f(
            modelYawUniform,
            modelYawRadians);

        glDrawElements(
            GL_TRIANGLES,
            drawIndexCount,
            GL_UNSIGNED_INT,
            nullptr);

        glBindVertexArray(0);
        glUseProgram(0);

        if (!firstFrameLogged) {
            firstFrameLogged = true;

            __android_log_print(
                ANDROID_LOG_INFO,
                kLogTag,
                "first render: zoom=%.2f anchor=(%.2f, %.2f)",
                parameters.zoom,
                anchor.x,
                anchor.y);
        }
    }

    void contextLost() override {
        program = 0;
        vertexArray = 0;
        vertexBuffer = 0;
        indexBuffer = 0;
        drawIndexCount = 0;
    }

    void deinitialize() override {
        __android_log_write(
            ANDROID_LOG_INFO,
            kLogTag,
            "deinitialize");

        if (indexBuffer != 0) {
            glDeleteBuffers(1, &indexBuffer);
            indexBuffer = 0;
        }

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
    void createGeometry() {
        if (
            mesh.vertices.empty() ||
            mesh.indices.empty()
        ) {
            __android_log_write(
                ANDROID_LOG_ERROR,
                kLogTag,
                "No GLB geometry available for GPU upload");

            return;
        }

        if (
            mesh.indices.size() >
            static_cast<std::size_t>(
                std::numeric_limits<GLsizei>::max())
        ) {
            __android_log_write(
                ANDROID_LOG_ERROR,
                kLogTag,
                "GLB index count exceeds GLsizei range");

            return;
        }

        drawIndexCount =
            static_cast<GLsizei>(
                mesh.indices.size());

        glGenVertexArrays(
            1,
            &vertexArray);

        glBindVertexArray(
            vertexArray);

        glGenBuffers(
            1,
            &vertexBuffer);

        glBindBuffer(
            GL_ARRAY_BUFFER,
            vertexBuffer);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                mesh.vertices.size() *
                sizeof(oovy::GlbVertex)),
            mesh.vertices.data(),
            GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(oovy::GlbVertex),
            nullptr);

        glGenBuffers(
            1,
            &indexBuffer);

        glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            indexBuffer);

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                mesh.indices.size() *
                sizeof(std::uint32_t)),
            mesh.indices.data(),
            GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        __android_log_print(
            ANDROID_LOG_INFO,
            kLogTag,
            "GPU model uploaded: vertices=%zu indices=%zu "
            "dimensions=(%.2f, %.2f, %.2f)m",
            mesh.vertices.size(),
            mesh.indices.size(),
            mesh.widthMeters,
            mesh.depthMeters,
            mesh.heightMeters);
    }

    oovy::GlbMeshData mesh;
    GLsizei drawIndexCount = 0;

    void logProgramError() {
        GLint length = 0;
        glGetProgramiv(
            program,
            GL_INFO_LOG_LENGTH,
            &length);

        if (length <= 1) {
            return;
        }

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

    GLuint program = 0;
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;

    GLint projectionUniform = -1;    
    GLint pixelsPerMeterUniform = -1; 
    GLint horizontalScaleUniform = -1;
    GLint modelYawUniform = -1;

    bool firstFrameLogged = false;
};

} // namespace

extern "C"
JNIEXPORT jlong JNICALL
Java_com_oovy_maplibre_style_layers_OovyCubeLayer_nativeCreateHost(
    JNIEnv* env,
    jclass,
    jbyteArray glbData
) {
    auto createEmptyHost = []() -> jlong {
        return reinterpret_cast<jlong>(
            std::make_unique<OovyCubeHost>(
                oovy::GlbMeshData{}
            ).release());
    };

    if (glbData == nullptr) {
        __android_log_write(
            ANDROID_LOG_ERROR,
            kLogTag,
            "GLB Java byte array is null");

        return createEmptyHost();
    }

    const jsize byteCount =
        env->GetArrayLength(glbData);

    if (byteCount <= 0) {
        __android_log_write(
            ANDROID_LOG_ERROR,
            kLogTag,
            "GLB Java byte array is empty");

        return createEmptyHost();
    }

    std::vector<std::uint8_t> bytes(
        static_cast<std::size_t>(byteCount));

    env->GetByteArrayRegion(
        glbData,
        0,
        byteCount,
        reinterpret_cast<jbyte*>(
            bytes.data()));

    if (env->ExceptionCheck()) {
        __android_log_write(
            ANDROID_LOG_ERROR,
            kLogTag,
            "Unable to copy GLB bytes from Java");

        return createEmptyHost();
    }

    oovy::GlbMeshData mesh;
    oovy::GlbSummary summary;
    std::string error;

    if (
        !oovy::loadGlbMesh(
            bytes.data(),
            bytes.size(),
            kTargetModelHeightMeters,
            mesh,
            summary,
            error)
    ) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "GLB loading failed: %s",
            error.c_str());

        return createEmptyHost();
    }

    __android_log_print(
        ANDROID_LOG_INFO,
        kLogTag,
        "GLB loaded: scenes=%zu nodes=%zu meshes=%zu "
        "primitives=%zu vertices=%zu indices=%zu "
        "dimensions=(%.2f, %.2f, %.2f)m",
        summary.sceneCount,
        summary.nodeCount,
        summary.meshCount,
        summary.primitiveCount,
        mesh.vertices.size(),
        mesh.indices.size(),
        mesh.widthMeters,
        mesh.depthMeters,
        mesh.heightMeters);

    return reinterpret_cast<jlong>(
        std::make_unique<OovyCubeHost>(
            std::move(mesh)
        ).release());
}