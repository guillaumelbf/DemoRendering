
#include "calc.hpp"
#include "gl_helpers.hpp"

#include "demo_cubemap.hpp"

// Vertex format
struct Vertex
{
    float3 position;
    float3 normal;
};

DemoCubemap::DemoCubemap(const DemoInputs& inputs)
{
    camera.position = { 0.f, 0.f, 2.f };

    // Upload vertex buffer
    {
        // In memory
        Vertex* vertices = nullptr;
        int vertexCount = 0;

        {
            VertexDescriptor descriptor = {};
            descriptor.positionOffset = offsetof(Vertex, position);
            descriptor.size = sizeof(Vertex);
            descriptor.hasNormal = true;
            descriptor.normalOffset = offsetof(Vertex, normal);

            MeshBuilder builder(descriptor, (void**)&vertices, &vertexCount);
            icosphere = builder.GenIcosphere(nullptr);
        }

        glGenBuffers(1, &vertexBuffer);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    }

    // Vertex layout
    {
        glGenVertexArrays(1, &vertexArrayObject);
        glBindVertexArray(vertexArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, normal));
    }

    // Create program
    program = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;

        out vec3 vWorldNormal;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            gl_Position = projection * view * model * vec4(aPosition, 1.0);
            vWorldNormal = mat3(model) * aNormal;
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        in vec3 vWorldNormal;

        out vec4 fragColor;

        uniform samplerCube cubemap;

        void main()
        {
            fragColor = vec4(1.0);

            // Debug show normals
            //fragColor = vec4(normalize(vWorldNormal), 1.0);
        }
        )GLSL"
    );

    // TODO:

    // Charger une cubemap
    // - https://hdrihaven.com/hdris/
    // -> .hdr -> .dds (format cubemap)
    // avec ibl baker à télécharger sur le drive partagé ou https://github.com/derkreature/IBLBaker

    // gen texture
    //glBindTexture(GL_TEXTURE_CUBE_MAP);
    //gl::UploadCubemap("cubemap.dds");

    // Cas d'utilisation : 
    // - Skybox
    // - Environement mapping reflections (en utilisants les normales)
    // - Générer une cubemap (avancé)
    // - Sauvegarder dans un .dds (accès aux données de textures OpenGL)
}

DemoCubemap::~DemoCubemap()
{
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(1, &vertexBuffer);
}

void DemoCubemap::UpdateAndRender(const DemoInputs& inputs)
{
    camera.UpdateFreeFly(inputs.cameraInputs);

    static float time = 0.f;
    time += 1.f / 60.f;

    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);

    glEnable(GL_DEPTH_TEST);

    glUseProgram(program);
    mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.01f, 50.f);
    mat4 view       = camera.GetViewMatrix();
    mat4 model      = mat4Identity();

    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection.e);
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view.e);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model.e);

    glBindVertexArray(vertexArrayObject);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, icosphere.start, icosphere.count);
}