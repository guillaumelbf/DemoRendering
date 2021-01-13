#pragma once

#include <glad/glad.h>

#include "mesh_builder.hpp"
#include "demo.hpp"

class DemoCubemap : public Demo
{
public:
    DemoCubemap(const DemoInputs& inputs);
    ~DemoCubemap() override;

    void UpdateAndRender(const DemoInputs& inputs) override;
    const char* Name() const override { return "Cubemap"; }

private:
    GLuint vertexBuffer = 0;
    GLuint vertexArrayObject = 0;
    GLuint program = 0;
    GLuint cubemap = 0;

    MeshSlice icosphere = {};

    Camera camera = {};
};