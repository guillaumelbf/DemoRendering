#include "demo_skybox.hpp"


#include <cstddef>
#include <vector>
#include <imgui.h>

#include "types.hpp"
#include "calc.hpp"
#include "gl_helpers.hpp"
#include "demo_fbo.hpp"

// Vertex format
struct Vertex
{
    float3 position;
    float3 normal;
};

DemoSkybox::DemoSkybox(const DemoInputs& inputs)
{
    // Upload vertex buffer
    /// CUBE
    {
        // In memory
        Vertex* vertices = nullptr;
        int vertexCount = 0;
        {
            VertexDescriptor descriptor = {};
            descriptor.size = sizeof(Vertex);
            descriptor.positionOffset = offsetof(Vertex, position);
            descriptor.hasNormal = true;
            descriptor.normalOffset = offsetof(Vertex, normal);

            MeshBuilder meshBuilder(descriptor, (void**)&vertices, &vertexCount);

            skybox = meshBuilder.LoadObj(nullptr, "media/cube.obj", "media", 1.f);
        }

        // In VRAM
        glGenBuffers(1, &skyboxVBO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

        free(vertices);
    }

    {
        glGenVertexArrays(1, &skyboxVAO);
        glBindVertexArray(skyboxVAO);

        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, normal));
    }

    /// SPHERE
    {
        // In memory
        Vertex* vertices = nullptr;
        int vertexCount = 0;
        
        {
            VertexDescriptor descriptor = {};
            descriptor.size = sizeof(Vertex);
            descriptor.positionOffset = offsetof(Vertex, position);
            descriptor.hasNormal = true;
            descriptor.normalOffset = offsetof(Vertex, normal);

            MeshBuilder meshBuilder(descriptor, (void**)&vertices, &vertexCount);

            sphere = meshBuilder.GenIcosphere(nullptr,4);
        }

        // In VRAM
        glGenBuffers(1, &sphereVBO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

        free(vertices);
    }

    // Vertex layout
    {
        glGenVertexArrays(1,&sphereVAO);
        glBindVertexArray(sphereVAO);

        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, normal));
    }

    // Skybox program
    skyboxProgram = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        
        out vec3 textCoords;        

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            textCoords = aPosition;
            vec4 pos = projection * view * model * vec4(aPosition, 1.0);
            gl_Position = pos.xyww;
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        in vec3 textCoords;        

        out vec4 fragColor;

        uniform samplerCube skybox;

        void main()
        {
            fragColor = texture(skybox, textCoords);
        }
        )GLSL"
    );

    // Cube program
    sphereProgram = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;    
        layout(location = 1) in vec3 aNormal;

        out vec3 Normal;
        out vec3 Position;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            Normal = mat3(transpose(inverse(model))) * aNormal;
            Position = vec3(model * vec4(aPosition,1.0));
            gl_Position = projection * view * vec4(Position, 1.0);
        }
        )GLSL",

        // Fragment shader
        R"GLSL(   

        out vec4 fragColor;

        in vec3 Normal;
        in vec3 Position;

        uniform vec3 cameraPos;
        uniform samplerCube skybox;

        void main()
        {
            vec3 I = normalize(Position - cameraPos);
            vec3 R = reflect(I, normalize(Normal));
            fragColor = vec4(texture(skybox, R).rgb, 1.0);
        }
        )GLSL"
    );

    glGenTextures(1, &skyboxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    gl::UploadImageCubeMap("media/skybox/");
    //gl::SetTextureDefaultParams();
}

DemoSkybox::~DemoSkybox()
{
    // Delete OpenGL objects
    glDeleteTextures(1, &skyboxTexture);
    glDeleteProgram(skyboxProgram);
    glDeleteProgram(sphereProgram);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
}

void DemoSkybox::UpdateAndRender(const DemoInputs& inputs)
{
    // Update camera
    mainCamera.UpdateFreeFly(inputs.cameraInputs);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);
    glEnable(GL_DEPTH_TEST);

    // Draw others

    {
        mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.01f, 50.f);
        mat4 view = mainCamera.GetViewMatrix();
        mat4 model = mat4Scale(1.f);

        glUseProgram(sphereProgram);
        glUniformMatrix4fv(glGetUniformLocation(sphereProgram, "projection"), 1, GL_FALSE, projection.e);
        glUniformMatrix4fv(glGetUniformLocation(sphereProgram, "view"), 1, GL_FALSE, view.e);
        glUniformMatrix4fv(glGetUniformLocation(sphereProgram, "model"), 1, GL_FALSE, model.e);
        glUniform3f(glGetUniformLocation(sphereProgram, "cameraPos"),mainCamera.position.x,mainCamera.position.y,mainCamera.position.z);
    }

    glBindVertexArray(sphereVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glDrawArrays(GL_TRIANGLES, sphere.start, sphere.count);


    // Draw Skybox
    glDepthFunc(GL_LEQUAL);
    {
        mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.01f, 50.f);
        mat4 view = mainCamera.GetViewMatrix();
        mat4 model = mat4Scale(1.f);

        view.e[12] = 0;
        view.e[13] = 0;
        view.e[14] = 0;

        glUseProgram(skyboxProgram);
        glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, projection.e);
        glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, view.e);
        glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "model"), 1, GL_FALSE, model.e);

        //glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);
    }

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glDrawArrays(GL_TRIANGLES, skybox.start, skybox.count); // Draw triangle
    glDepthFunc(GL_LESS);
}


