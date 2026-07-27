#include "oovy_model_host.hpp"

#include <GLES3/gl3.h>
#include <android/log.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include <mbgl/util/geo.hpp>
#include <mbgl/util/mat4.hpp>
#include <mbgl/util/projection.hpp>

namespace {

constexpr const char* kLogTag = "OovyModelHost";
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
    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &compiled
    );

    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint length = 0;

    glGetShaderiv(
        shader,
        GL_INFO_LOG_LENGTH,
        &length
    );

    if (length > 1) {
        auto log =
            std::make_unique<GLchar[]>(
                static_cast<std::size_t>(length)
            );

        glGetShaderInfoLog(
            shader,
            length,
            nullptr,
            log.get()
        );

        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "%s shader error: %s",
            label,
            log.get()
        );
    }

    glDeleteShader(shader);

    return 0;
}

class OovyModelHost final
    : public mbgl::style::CustomLayerHost {
public:
    OovyModelHost(
        oovy::OovyModelDescriptor descriptor_,
        oovy::GlbMeshData mesh_
    )
        : descriptor(std::move(descriptor_)),
          mesh(std::move(mesh_)) {
    }

    bool is3D() const override {
        return true;
    }

    void initialize() override {
        __android_log_print(
            ANDROID_LOG_INFO,
            kLogTag,
            "initialize: assetId=%s",
            descriptor.assetId.c_str()
        );

        static constexpr const char* vertexShaderSource =
            R"(#version 300 es
        layout(location = 0) in vec3 a_position;

        uniform mat4 u_rebased_projection;
        uniform float u_pixels_per_meter;
        uniform vec2 u_model_horizontal_scale;
        uniform float u_model_yaw_radians;
        uniform float u_uniform_scale;
        uniform float u_altitude_meters;

        out vec3 v_local_position;

        void main() {
            vec3 scaledModelMeters =
                a_position * u_uniform_scale;

            vec2 footprintAdjusted = vec2(
                scaledModelMeters.x *
                    u_model_horizontal_scale.x,

                scaledModelMeters.y *
                    u_model_horizontal_scale.y
            );

            float yawCos =
                cos(u_model_yaw_radians);

            float yawSin =
                sin(u_model_yaw_radians);

            vec2 rotatedHorizontal = vec2(
                yawCos * footprintAdjusted.x -
                    yawSin * footprintAdjusted.y,

                yawSin * footprintAdjusted.x +
                    yawCos * footprintAdjusted.y
            );

            vec3 positionedMeters = vec3(
                rotatedHorizontal.x,
                rotatedHorizontal.y,
                scaledModelMeters.z +
                    u_altitude_meters
            );

            vec3 localWorldPosition = vec3(
                positionedMeters.x *
                    u_pixels_per_meter,

                positionedMeters.y *
                    u_pixels_per_meter,

                positionedMeters.z
            );

            v_local_position =
                positionedMeters;

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
                normalize(
                    vec3(-0.4, -0.6, 0.7)
                );

            float diffuse =
                0.68 +
                0.32 *
                abs(
                    dot(
                        normal,
                        lightDirection
                    )
                );

            vec3 baseColor =
                vec3(0.651, 0.486, 0.231);

            fragmentColor =
                vec4(
                    baseColor * diffuse,
                    1.0
                );
        }
        )";

        const GLuint vertexShader =
            compileShader(
                GL_VERTEX_SHADER,
                vertexShaderSource,
                "vertex"
            );

        const GLuint fragmentShader =
            compileShader(
                GL_FRAGMENT_SHADER,
                fragmentShaderSource,
                "fragment"
            );

        if (
            vertexShader == 0 ||
            fragmentShader == 0
        ) {
            if (vertexShader != 0) {
                glDeleteShader(vertexShader);
            }

            if (fragmentShader != 0) {
                glDeleteShader(fragmentShader);
            }

            return;
        }

        program = glCreateProgram();

        glAttachShader(
            program,
            vertexShader
        );

        glAttachShader(
            program,
            fragmentShader
        );

        glLinkProgram(program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = GL_FALSE;

        glGetProgramiv(
            program,
            GL_LINK_STATUS,
            &linked
        );

        if (linked != GL_TRUE) {
            logProgramError();

            glDeleteProgram(program);
            program = 0;

            return;
        }

        projectionUniform =
            glGetUniformLocation(
                program,
                "u_rebased_projection"
            );

        pixelsPerMeterUniform =
            glGetUniformLocation(
                program,
                "u_pixels_per_meter"
            );

        horizontalScaleUniform =
            glGetUniformLocation(
                program,
                "u_model_horizontal_scale"
            );

        modelYawUniform =
            glGetUniformLocation(
                program,
                "u_model_yaw_radians"
            );

        uniformScaleUniform =
            glGetUniformLocation(
                program,
                "u_uniform_scale"
            );

        altitudeUniform =
            glGetUniformLocation(
                program,
                "u_altitude_meters"
            );

        if (
            projectionUniform < 0 ||
            pixelsPerMeterUniform < 0 ||
            horizontalScaleUniform < 0 ||
            modelYawUniform < 0 ||
            uniformScaleUniform < 0 ||
            altitudeUniform < 0
        ) {
            __android_log_print(
                ANDROID_LOG_ERROR,
                kLogTag,
                "Unable to resolve shader uniforms: assetId=%s",
                descriptor.assetId.c_str()
            );

            return;
        }

        createGeometry();
    }

    void render(
        const mbgl::style::CustomLayerRenderParameters&
            parameters
    ) override {
        if (
            parameters.zoom < descriptor.minZoom ||
            parameters.zoom > descriptor.maxZoom
        ) {
            return;
        }

        if (
            program == 0 ||
            vertexArray == 0 ||
            indexBuffer == 0 ||
            drawIndexCount <= 0
        ) {
            return;
        }

        if (
            mesh.widthMeters <= 0.0f ||
            mesh.depthMeters <= 0.0f
        ) {
            return;
        }

        const double worldScale =
            std::pow(
                2.0,
                parameters.zoom
            );

        const auto anchor =
            mbgl::Projection::project(
                mbgl::LatLng{
                    descriptor.latitude,
                    descriptor.longitude,
                },
                worldScale
            );

        const double metersPerPixel =
            mbgl::Projection::
                getMetersPerPixelAtLatitude(
                    descriptor.latitude,
                    parameters.zoom
                );

        if (
            !std::isfinite(metersPerPixel) ||
            metersPerPixel <= 0.0
        ) {
            return;
        }

        const double horizontalScaleX =
            descriptor.targetFootprintWidthMeters /
            static_cast<double>(
                mesh.widthMeters
            );

        const double horizontalScaleY =
            descriptor.targetFootprintDepthMeters /
            static_cast<double>(
                mesh.depthMeters
            );

        const double maxFloat =
            static_cast<double>(
                std::numeric_limits<float>::max()
            );

        if (
            !std::isfinite(horizontalScaleX) ||
            !std::isfinite(horizontalScaleY) ||
            horizontalScaleX <= 0.0 ||
            horizontalScaleY <= 0.0 ||
            horizontalScaleX > maxFloat ||
            horizontalScaleY > maxFloat ||
            descriptor.uniformScale > maxFloat ||
            std::abs(descriptor.altitudeMeters) > maxFloat
        ) {
            return;
        }

        const float pixelsPerMeter =
            static_cast<float>(
                1.0 / metersPerPixel
            );

        const float normalizedYawDegrees =
            static_cast<float>(
                std::fmod(
                    descriptor.yawDegrees,
                    360.0
                )
            );

        const float modelYawRadians =
            normalizedYawDegrees *
            kPi /
            180.0f;

        mbgl::mat4 rebasedProjection =
            parameters.projectionMatrix;

        mbgl::matrix::translate(
            rebasedProjection,
            rebasedProjection,
            anchor.x,
            anchor.y,
            0.0
        );

        std::array<GLfloat, 16> projection{};

        std::transform(
            rebasedProjection.begin(),
            rebasedProjection.end(),
            projection.begin(),
            [](double value) {
                return static_cast<GLfloat>(value);
            }
        );

        /*
         * Conserved from the validated Eiffel POC.
         * Depth is supplied by MapLibre through depthModeFor3D().
         */
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        glUseProgram(program);
        glBindVertexArray(vertexArray);

        glUniformMatrix4fv(
            projectionUniform,
            1,
            GL_FALSE,
            projection.data()
        );

        glUniform1f(
            pixelsPerMeterUniform,
            pixelsPerMeter
        );

        glUniform2f(
            horizontalScaleUniform,
            static_cast<float>(
                horizontalScaleX
            ),
            static_cast<float>(
                horizontalScaleY
            )
        );

        glUniform1f(
            modelYawUniform,
            modelYawRadians
        );

        glUniform1f(
            uniformScaleUniform,
            static_cast<float>(
                descriptor.uniformScale
            )
        );

        glUniform1f(
            altitudeUniform,
            static_cast<float>(
                descriptor.altitudeMeters
            )
        );

        glDrawElements(
            GL_TRIANGLES,
            drawIndexCount,
            GL_UNSIGNED_INT,
            nullptr
        );

        glBindVertexArray(0);
        glUseProgram(0);

        if (!firstFrameLogged) {
            firstFrameLogged = true;

            __android_log_print(
                ANDROID_LOG_INFO,
                kLogTag,
                "first render: assetId=%s zoom=%.2f "
                "anchor=(%.2f, %.2f) altitude=%.2f "
                "yaw=%.2f scale=%.3f",
                descriptor.assetId.c_str(),
                parameters.zoom,
                anchor.x,
                anchor.y,
                descriptor.altitudeMeters,
                descriptor.yawDegrees,
                descriptor.uniformScale
            );
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
        __android_log_print(
            ANDROID_LOG_INFO,
            kLogTag,
            "deinitialize: assetId=%s",
            descriptor.assetId.c_str()
        );

        if (indexBuffer != 0) {
            glDeleteBuffers(
                1,
                &indexBuffer
            );

            indexBuffer = 0;
        }

        if (vertexBuffer != 0) {
            glDeleteBuffers(
                1,
                &vertexBuffer
            );

            vertexBuffer = 0;
        }

        if (vertexArray != 0) {
            glDeleteVertexArrays(
                1,
                &vertexArray
            );

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
            __android_log_print(
                ANDROID_LOG_ERROR,
                kLogTag,
                "No GLB geometry available: assetId=%s",
                descriptor.assetId.c_str()
            );

            return;
        }

        if (
            mesh.indices.size() >
            static_cast<std::size_t>(
                std::numeric_limits<GLsizei>::max()
            )
        ) {
            __android_log_print(
                ANDROID_LOG_ERROR,
                kLogTag,
                "GLB index count exceeds GLsizei: assetId=%s",
                descriptor.assetId.c_str()
            );

            return;
        }

        drawIndexCount =
            static_cast<GLsizei>(
                mesh.indices.size()
            );

        glGenVertexArrays(
            1,
            &vertexArray
        );

        glBindVertexArray(vertexArray);

        glGenBuffers(
            1,
            &vertexBuffer
        );

        glBindBuffer(
            GL_ARRAY_BUFFER,
            vertexBuffer
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                mesh.vertices.size() *
                sizeof(oovy::GlbVertex)
            ),
            mesh.vertices.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(oovy::GlbVertex),
            nullptr
        );

        glGenBuffers(
            1,
            &indexBuffer
        );

        glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            indexBuffer
        );

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                mesh.indices.size() *
                sizeof(std::uint32_t)
            ),
            mesh.indices.data(),
            GL_STATIC_DRAW
        );

        glBindVertexArray(0);
        glBindBuffer(
            GL_ARRAY_BUFFER,
            0
        );

        __android_log_print(
            ANDROID_LOG_INFO,
            kLogTag,
            "GPU model uploaded: assetId=%s "
            "vertices=%zu indices=%zu "
            "dimensions=(%.2f, %.2f, %.2f)m",
            descriptor.assetId.c_str(),
            mesh.vertices.size(),
            mesh.indices.size(),
            mesh.widthMeters,
            mesh.depthMeters,
            mesh.heightMeters
        );
    }

    void logProgramError() {
        GLint length = 0;

        glGetProgramiv(
            program,
            GL_INFO_LOG_LENGTH,
            &length
        );

        if (length <= 1) {
            return;
        }

        auto log =
            std::make_unique<GLchar[]>(
                static_cast<std::size_t>(length)
            );

        glGetProgramInfoLog(
            program,
            length,
            nullptr,
            log.get()
        );

        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "Program link error: assetId=%s error=%s",
            descriptor.assetId.c_str(),
            log.get()
        );
    }

    oovy::OovyModelDescriptor descriptor;
    oovy::GlbMeshData mesh;

    GLuint program = 0;
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;

    GLsizei drawIndexCount = 0;

    GLint projectionUniform = -1;
    GLint pixelsPerMeterUniform = -1;
    GLint horizontalScaleUniform = -1;
    GLint modelYawUniform = -1;
    GLint uniformScaleUniform = -1;
    GLint altitudeUniform = -1;

    bool firstFrameLogged = false;
};

} // namespace

namespace oovy {

std::unique_ptr<mbgl::style::CustomLayerHost> createModelHost(
    OovyModelDescriptor descriptor,
    GlbMeshData mesh
) {
    return std::make_unique<OovyModelHost>(
        std::move(descriptor),
        std::move(mesh)
    );
}

} // namespace oovy