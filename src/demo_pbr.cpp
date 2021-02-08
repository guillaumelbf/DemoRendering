#include "demo_pbr.hpp"

DemoPBR::DemoPBR(const DemoInputs& inputs)
{

}

DemoPBR::~DemoPBR()
{

}

void DemoPBR::UpdateAndRender(const DemoInputs& inputs)
{
    mainCamera.UpdateFreeFly(inputs.cameraInputs);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);


}