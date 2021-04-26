#pragma once

#include "glad/glad.h"

#include "mesh_builder.hpp"

#include "demo.hpp"

#include "demo_pbr.hpp"

class DemoIBL : public Demo
{
public:
    DemoIBL(const DemoInputs& inputs);
    ~DemoIBL();

    void UpdateAndRender(const DemoInputs& inputs) final;
    const char* Name() const final { return "IBL"; }

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