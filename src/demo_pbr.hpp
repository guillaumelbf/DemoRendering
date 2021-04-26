#pragma once

#include "glad/glad.h"

#include "mesh_builder.hpp"

#include "demo.hpp"

struct Program
{
    GLuint id = 0;
};

struct Object
{
    GLuint VAO = 0;
    GLuint VBO = 0;

    MeshSlice mesh {};

    GLuint albedo    = 0;
    GLuint normal    = 0;
    GLuint metallic  = 0;
    GLuint roughness = 0;
    GLuint ao        = 0;
};

struct Light
{
    float3 position;
    float3 color;
};

class DemoPBR : public Demo
{
public:
    DemoPBR(const DemoInputs& inputs);
    ~DemoPBR();

    void UpdateAndRender(const DemoInputs& inputs) final;
    const char* Name() const final { return "PBR"; }

private:

    Camera mainCamera = {};

    Object pbrSphere;

    Program basicPBR;
    Program texturedPBR;

    Program usedProgram;

    Light lights = 
    {
        {0.f,0.f,10.f},{150.f,150.f,150.f},
    };

};