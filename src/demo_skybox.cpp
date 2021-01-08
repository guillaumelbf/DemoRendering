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
};

DemoSkybox::DemoSkybox(const DemoInputs& inputs)
{
    // Upload vertex buffer
    {
        // In memory
        Vertex* vertices = nullptr;
        int vertexCount = 0;

        {
            VertexDescriptor descriptor = {};
            descriptor.size = sizeof(Vertex);
            descriptor.positionOffset = offsetof(Vertex, position);

            MeshBuilder meshBuilder(descriptor, (void**)&vertices, &vertexCount);

            fullscreenQuad = meshBuilder.GenQuad(nullptr, 1.0f, 1.0f);
            obj = meshBuilder.LoadObj(nullptr, "media/cube.obj", "media", 1.f);
        }

        // In VRAM
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

        free(vertices);
    }

    // Vertex layout
    {
        glGenVertexArrays(1, &skyboxVAO);
        glBindVertexArray(skyboxVAO);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
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
    cubeProgram = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;       

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            gl_Position = projection * view * model * vec4(aPosition, 1.0);
        }
        )GLSL",

        // Fragment shader
        R"GLSL(   

        out vec4 fragColor;

        void main()
        {
            fragColor = vec4(1,1,1,1);
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
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &vertexBuffer);
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

        glUseProgram(cubeProgram);
        glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "projection"), 1, GL_FALSE, projection.e);
        glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "view"), 1, GL_FALSE, view.e);
        glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "model"), 1, GL_FALSE, model.e);
    }

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, obj.start, obj.count);


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

        glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);
    }

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

    glDrawArrays(GL_TRIANGLES, obj.start, obj.count); // Draw triangle
    glDepthFunc(GL_LESS);
}


