#include "glad/glad.h"

#include "mesh_builder.hpp"

#include "demo.hpp"

class DemoSkybox : public Demo
{
public:
	DemoSkybox(const DemoInputs& inputs);
	~DemoSkybox();

	void UpdateAndRender(const DemoInputs& inputs) final;
	const char* Name() const final { return "Skybox & Reflection"; }

private:

    Camera mainCamera = {};

    GLuint skyboxVBO = 0;
    GLuint skyboxVAO = 0;

    GLuint sphereVBO = 0;
    GLuint sphereVAO = 0;

    GLuint skyboxTexture = 0;

    GLuint skyboxProgram = 0;
    GLuint sphereProgram = 0;

    MeshSlice skybox = {};
    MeshSlice sphere = {};
};

