
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
    float3 tangent;
    float3 bitangent;
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

            // positions
            float3 pos1(-1.0f, 1.0f, 0.0f);
            float3 pos2(-1.0f, -1.0f, 0.0f);
            float3 pos3(1.0f, -1.0f, 0.0f);
            float3 pos4(1.0f, 1.0f, 0.0f);
            // texture coordinates
            float2 uv1(0.0f, 1.0f);
            float2 uv2(0.0f, 0.0f);
            float2 uv3(1.0f, 0.0f);
            float2 uv4(1.0f, 1.0f);
            // normal vector
            float3 nm(0.0f, 0.0f, 1.0f);

            // calculate tangent/bitangent vectors of both triangles
            float3 tangent1, bitangent1;
            float3 tangent2, bitangent2;
            // triangle 1
            // ----------
            float3 edge1 = pos2 - pos1;
            float3 edge2 = pos3 - pos1;
            float2 deltaUV1 = uv2 - uv1;
            float2 deltaUV2 = uv3 - uv1;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

            // triangle 2
            // ----------
            edge1 = pos3 - pos1;
            edge2 = pos4 - pos1;
            deltaUV1 = uv3 - uv1;
            deltaUV2 = uv4 - uv1;

            f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


            bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


            Vertex quadVertices[6] = {
                // positions                // texcoords    // normal           // tangent                            // bitangent
                { {pos1.x, pos1.y, pos1.z}, {uv1.x, uv1.y}, {nm.x, nm.y, nm.z}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z} },
                { {pos2.x, pos2.y, pos2.z}, {uv2.x, uv2.y}, {nm.x, nm.y, nm.z}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z} },
                { {pos3.x, pos3.y, pos3.z}, {uv3.x, uv3.y}, {nm.x, nm.y, nm.z}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z} },

                { {pos1.x, pos1.y, pos1.z}, {uv1.x, uv1.y}, {nm.x, nm.y, nm.z}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z} },
                { {pos3.x, pos3.y, pos3.z}, {uv3.x, uv3.y}, {nm.x, nm.y, nm.z}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z} },
                { {pos4.x, pos4.y, pos4.z}, {uv4.x, uv4.y}, {nm.x, nm.y, nm.z}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z} }
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
            descriptor.hasTangent       = true;
            descriptor.tangentOffset    = offsetof(Vertex, tangent);
            descriptor.hasBitangent     = true;
            descriptor.bitangentOffset  = offsetof(Vertex, bitangent);

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
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, bitangent));
    }

    program = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec3 aNormal;
        layout(location = 3) in vec3 aTangent;
        layout(location = 4) in vec3 aBitangent;
        
        out vec3 vFragPos;
        out vec2 vUV;
        out vec3 vTangentLightPos;
        out vec3 vTangentViewPos;
        out vec3 vTangentFragPos;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        uniform vec3 lightPos;
        uniform vec3 viewPos;

        void main()
        {
            vFragPos = vec3(model * vec4(aPosition, 1.0));
            vUV      = aUV;

            mat3 normalMatrix = transpose(inverse(mat3(model)));
            vec3 T = normalize(normalMatrix * aTangent);
            vec3 N = normalize(normalMatrix * aNormal);
            T = normalize(T - dot(T, N) * N);
            vec3 B = cross(N, T);
            
            mat3 TBN = transpose(mat3(T, B, N));    
            vTangentLightPos = TBN * lightPos;
            vTangentViewPos  = TBN * viewPos;
            vTangentFragPos  = TBN * vFragPos;

            gl_Position = projection * view * model * vec4(aPosition, 1.0);
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        layout(location = 0) out vec4 fragColor;

        in vec3 vFragPos;
        in vec2 vUV;
        in vec3 vTangentLightPos;
        in vec3 vTangentViewPos;
        in vec3 vTangentFragPos;

        uniform sampler2D albedoTexture;
        uniform sampler2D normalTexture;

        uniform vec3 lightPos;
        uniform vec3 viewPos;

        uniform bool debugDisableLight;
        uniform bool debugDisableNormalMap;
        uniform bool debugShowGeometryNormals;
        uniform bool debugShowNormals;
        uniform bool debugShowNormalMap;

        void main()
        {
            vec3 normal = texture(normalTexture, vUV).rgb;
            normal = normalize(normal * 2.0 - 1.0);

            vec3 albedo = texture(albedoTexture, vUV).rgb;

            vec3 ambient = 0.1 * albedo;

            vec3 lightDir = normalize(vTangentLightPos - vTangentFragPos);
            float diff = max(dot(lightDir, normal), 0.0);
            vec3 diffuse = diff * albedo;           

            vec3 viewDir = normalize(vTangentViewPos - vTangentFragPos);
            vec3 reflectDir = reflect(-lightDir, normal);
            vec3 halfwayDir = normalize(lightDir + viewDir);  
            float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
            vec3 specular = vec3(0.2) * spec;            

            fragColor = vec4(ambient + diffuse + specular, 1.0);            

            if (debugShowNormalMap)
                fragColor = texture(normalTexture, vUV);

            if (debugShowGeometryNormals)
                fragColor = vec4(normalize(normal), 1.0);
            
            if (debugShowNormals)  
                fragColor = vec4(ambient + diffuse + specular, 1.0);

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
    glUniform3fv(glGetUniformLocation(program, "lightPos"), 1, lightPosition.e);
    glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, camera.position.e);
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
