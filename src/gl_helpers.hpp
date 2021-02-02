#pragma once


#include <glad/glad.h>
#include <string>


namespace gl
{
    GLuint CreateShader(GLenum type, int sourceCount, const char** sources);
    GLuint CreateBasicProgram(const char* vsStr, const char* fsStr);
    GLuint CreateProgram(int vsStrsCount, const char** vsStrs, int fsStrsCount, const char** fsStrs);
    void UploadPerlinNoise(int width, int height, float z, float lacunarity = 2.f, float gain = 0.5f, float offset = 1.f, int octaves = 6);
    void UploadImageCubeMap(const std::string& folderPath);
    void UploadImage(const char* file, bool linear = false);
    void UploadColoredTexture(float r, float g, float b, float a);
    void UploadCubemap(const char* filename);
    void SetTextureDefaultParams(bool genMipmap = true);
}