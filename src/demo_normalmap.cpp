
#include <vector>
#include <imgui.h>

#include "calc.hpp"
#include "gl_helpers.hpp"

#include "demo_normalmap.hpp"

// Vertex format
struct Vertex
{
    float3 position;
    float2 uv;
    float3 normal;
    // TODO: Add tangent
};

DemoNormalMap::DemoNormalMap(const DemoInputs& inputs)
{
    camera.position = { 0.f, 0.f, 2.f };
    lightPosition = { 0.2f, 0.4f, 0.2f };

    // Upload vertex buffer
    {
        // In memory
        int vertexCount = 0;
        Vertex* vertices = (Vertex*)calloc(6, sizeof(Vertex));

        // Create quad
        {
            Vertex quadVertices[6] = 
            {
                // Quad (6 vertices)
                // TODO: Add tangent for each vertex
                { { 0.5f,-0.5f, 0.f }, { 1.f, 0.f }, { 0.f, 0.f, 1.f } },
                { {-0.5f,-0.5f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f, 1.f } },
                { { 0.5f, 0.5f, 0.f }, { 1.f, 1.f }, { 0.f, 0.f, 1.f } },

                { { 0.5f, 0.5f, 0.f }, { 1.f, 1.f }, { 0.f, 0.f, 1.f } },
                { {-0.5f, 0.5f, 0.f }, { 0.f, 1.f }, { 0.f, 0.f, 1.f } },
                { {-0.5f,-0.5f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f, 1.f } },
            };
            memcpy(vertices, quadVertices, 6 * sizeof(Vertex));
            quad.start = 0;
            quad.count = 6;
            vertexCount += 6;
        }

        // Create sphere
        {
            VertexDescriptor descriptor = {};
            descriptor.size             = sizeof(Vertex);
            descriptor.positionOffset   = offsetof(Vertex, position);
            descriptor.hasUV            = true;
            descriptor.uvOffset         = offsetof(Vertex, uv);
            descriptor.hasNormal        = true;
            descriptor.normalOffset     = offsetof(Vertex, normal);
            // TODO: Add tangent property

            MeshBuilder builder(descriptor, (void**)&vertices, &vertexCount);
            sphere = builder.GenUVSphere(nullptr, 48, 64);
        }

        glGenBuffers(1, &vertexBuffer);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
        free(vertices);
    }

    // Vertex layout
    {
        glGenVertexArrays(1, &vertexArrayObject);
        glBindVertexArray(vertexArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, normal));
        // TODO: Add tangent attribute
    }

    program = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec3 aNormal;
        // TODO: Add and use tangent attribute

        out vec2 vUV;
        out vec3 vWorldPosition;
        out vec3 vWorldNormal;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            gl_Position = projection * view * model * vec4(aPosition, 1.0);
            vWorldNormal = mat3(model) * aNormal; // Assume uniform scale
            vWorldPosition = (model * vec4(aPosition, 1.0)).xyz;
            vUV = aUV;
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        in vec2 vUV;
        in vec3 vWorldPosition;
        in vec3 vWorldNormal;

        layout(location = 0) out vec4 fragColor;

        uniform sampler2D albedoTexture;
        uniform sampler2D normalTexture;

        uniform vec3 lightWorldPosition;

        uniform bool debugDisableLight;
        uniform bool debugDisableNormalMap;
        uniform bool debugShowGeometryNormals;
        uniform bool debugShowNormals;
        uniform bool debugShowNormalMap;

        void main()
        {
            vec3 worldNormal = normalize(vWorldNormal);
            vec3 L = normalize(lightWorldPosition - vWorldPosition);

            vec3 albedo = texture(albedoTexture, vUV).rgb;

            float diffuse = max(dot(worldNormal, L), 0.0);

            fragColor = vec4(diffuse * albedo, 1.0);
            
            if (debugShowNormalMap)
                fragColor = texture(normalTexture, vUV);

            if (debugShowGeometryNormals)
                fragColor = vec4(normalize(vWorldNormal), 1.0);
            
            // TODO: Show calculated normal
            //if (debugShowNormals)
            //    

            if (debugDisableLight)
                fragColor = vec4(albedo, 1.0);
        }
        )GLSL"
    );

    {
        glGenTextures(1, &albedoTexture);
        glBindTexture(GL_TEXTURE_2D, albedoTexture);
        gl::UploadImage("media/scpgdgca_2K_Albedo.jpg");
        gl::SetTextureDefaultParams();
    }

    {
        glGenTextures(1, &normalTexture);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
        gl::UploadImage("media/scpgdgca_2K_Normal.jpg");
        gl::SetTextureDefaultParams();
    }

    {
        glGenTextures(1, &whiteTexture);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        gl::UploadColoredTexture(1.f, 1.f, 1.f, 1.f);
        gl::SetTextureDefaultParams();
    }

    {
        glGenTextures(1, &purpleTexture);
        glBindTexture(GL_TEXTURE_2D, purpleTexture);
        gl::UploadColoredTexture(0.5f, 0.5f, 1.f, 1.f);
        gl::SetTextureDefaultParams();
    }
}

DemoNormalMap::~DemoNormalMap()
{
    glDeleteTextures(1, &purpleTexture);
    glDeleteTextures(1, &whiteTexture);
    glDeleteTextures(1, &normalTexture);
    glDeleteTextures(1, &albedoTexture);
    glDeleteProgram(program);

}

void DemoNormalMap::UpdateAndRender(const DemoInputs& inputs)
{
    camera.UpdateFreeFly(inputs.cameraInputs);

    mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.1f, 400.f);
    mat4 view       = camera.GetViewMatrix();

    ImGui::Checkbox("disable normal map", &disableNormalMap);

    const char* debugModeTypeStr[] =
    {
        "SHADE",
        "SHOW_NORMALS",
        "SHOW_GEO_NORMALS",
        "SHOW_NORMAL_MAP"
    };

    ImGui::Combo("debug mode", (int*)&debugMode, debugModeTypeStr, ARRAYSIZE(debugModeTypeStr));

    ImGui::DragFloat3("light pos", lightPosition.e, 0.05f);

    glUseProgram(program);

    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection.e);
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view.e);
    glUniform3fv(glGetUniformLocation(program, "lightWorldPosition"), 1, lightPosition.e);
    glUniform1i(glGetUniformLocation(program, "albedoTexture"), 0);
    glUniform1i(glGetUniformLocation(program, "normalTexture"), 1);
    glUniform1i(glGetUniformLocation(program, "debugDisableLight"), 0);
    glUniform1i(glGetUniformLocation(program, "debugDisableNormalMap"), disableNormalMap);
    glUniform1i(glGetUniformLocation(program, "debugShowGeometryNormals"), debugMode == DebugMode::SHOW_GEO_NORMALS);
    glUniform1i(glGetUniformLocation(program, "debugShowNormals"), debugMode == DebugMode::SHOW_NORMALS);
    glUniform1i(glGetUniformLocation(program, "debugShowNormalMap"), debugMode == DebugMode::SHOW_NORMAL_MAP);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, disableNormalMap ? purpleTexture : normalTexture);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(vertexArrayObject);

    // Draw textured objects
    {
        // Draw quad
        {
            mat4 model = mat4Translate({ -0.5f, 0.f, 0.f });
            glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model.e);
            glDrawArrays(GL_TRIANGLES, quad.start, quad.count);
        }
        // Draw sphere
        {
            mat4 model = mat4Translate({ 0.5f, 0.f, 0.f }) * mat4Scale(0.5f);
            glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model.e);
            glDrawArrays(GL_TRIANGLES, sphere.start, sphere.count);
        }
    }

    // Draw light position
    {
        glUniform1i(glGetUniformLocation(program, "debugDisableLight"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        mat4 model = mat4Translate(lightPosition) * mat4Scale(0.05f);
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model.e);
        glDrawArrays(GL_TRIANGLES, sphere.start, sphere.count);
    }
}
