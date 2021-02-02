#include "glad/glad.h"

#include "mesh_builder.hpp"

#include "demo.hpp"

class DemoPBR : public Demo
{
public:
    DemoPBR(const DemoInputs& inputs);
    ~DemoPBR();

    void UpdateAndRender(const DemoInputs& inputs) final;
    const char* Name() const final { return "PBR"; }

private:

    Camera mainCamera = {};


};