#pragma once

#include <glad/glad.h>

#include "demo.hpp"
#include "mesh_builder.hpp"

class DemoNormalMap : public Demo
{
public:
    DemoNormalMap(const DemoInputs& inputs);
    ~DemoNormalMap() override;
    void UpdateAndRender(const DemoInputs& inputs) override;
    const char* Name() const override { return "Normal map"; }

private:
    Camera camera = {};

    GLuint vertexBuffer = 0;
    GLuint vertexArrayObject = 0;
    GLuint program = 0;

    GLuint albedoTexture = 0;
    GLuint normalTexture = 0;
    GLuint whiteTexture = 0;
    GLuint purpleTexture = 0;

    float3 lightPosition = {};

    enum class DebugMode : int
    {
        SHADE,
        SHOW_NORMALS,
        SHOW_GEO_NORMALS,
        SHOW_NORMAL_MAP
    };

    MeshSlice quad = {};
    MeshSlice sphere = {};
    bool disableNormalMap = false;
    DebugMode debugMode = DebugMode::SHADE;
};