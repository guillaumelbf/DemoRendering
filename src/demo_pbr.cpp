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


}