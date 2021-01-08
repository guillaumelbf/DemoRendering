
#include <cstdio>
#include <vector>

#include <stb_perlin.h>
#include <stb_image.h>

#include "types.hpp"
#include "gl_helpers.hpp"

GLuint gl::CreateShader(GLenum type, int sourceCount, const char** sources)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, sourceCount, sources, nullptr);
    glCompileShader(shader);
    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, ARRAYSIZE(infoLog), nullptr, infoLog);
        printf("Shader compilation error: %s", infoLog);
    }

    return shader;
}

GLuint gl::CreateProgram(int vsStrsCount, const char** vsStrs, int fsStrsCount, const char** fsStrs)
{
    GLuint program = glCreateProgram();

    const char* shaderHeader = "#version 330\n";

    std::vector<const char*> vertexShaderSources;
    vertexShaderSources.push_back(shaderHeader);
    for (int i = 0; i < vsStrsCount; ++i)
        vertexShaderSources.push_back(vsStrs[i]);

    std::vector<const char*> pixelShaderSources;
    pixelShaderSources.push_back(shaderHeader);
    for (int i = 0; i < fsStrsCount; ++i)
        pixelShaderSources.push_back(fsStrs[i]);

    GLuint vertexShader = gl::CreateShader(GL_VERTEX_SHADER,   (int)vertexShaderSources.size(), vertexShaderSources.data());
    GLuint pixelShader  = gl::CreateShader(GL_FRAGMENT_SHADER, (int)pixelShaderSources.size(),  pixelShaderSources.data());

    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);
    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, ARRAYSIZE(infoLog), nullptr, infoLog);
        printf("Program link error: %s", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(pixelShader);

    return program;
}

GLuint gl::CreateBasicProgram(const char* vsStr, const char* fsStr)
{
    return CreateProgram(1, &vsStr, 1, &fsStr);
}

void gl::UploadPerlinNoise(int width, int height, float z, float lacunarity, float gain, float offset, int octaves)
{
    std::vector<float> pixels(width * height);

#if 0
    for (float& pixel : pixels)
        pixel = std::rand() / (float)RAND_MAX;
#else
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            pixels[x + y * width] = stb_perlin_ridge_noise3(x / (float)width, y / (float)height, z, lacunarity, gain, offset, octaves);
        }
    }
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, pixels.data());
}

void gl::UploadImage(const char* file, bool linear)
{
    int width    = 0;
    int height   = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(1);
    void* colors;

    if (linear)
        colors = stbi_loadf(file, &width, &height, &channels, 0);
    else
        colors = stbi_load(file, &width, &height, &channels, 0);

    if (colors == nullptr)
        fprintf(stderr, "Failed to load image '%s'\n", file);
    else
        printf("Load image '%s' (%dx%d %d channels)\n", file, width, height, channels);

    GLenum format;
    switch (channels)
    {
    case 1:          format = GL_RED;  break;
    case 2:          format = GL_RG;   break;
    case 3:          format = GL_RGB;  break;
    case 4: default: format = GL_RGBA; break;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, linear ? GL_FLOAT : GL_UNSIGNED_BYTE, colors);

    stbi_image_free(colors);
}

void gl::UploadImageCubeMap(const std::string& folderPath)
{
    const std::string faces[6] =
    {
        "right.jpg",
        "left.jpg",
        "top.jpg",
        "bottom.jpg",
        "back.jpg",
        "front.jpg"
    };

    int width = 0;
    int height = 0;
    int channels = 0;

    for (size_t i = 0; i < 6; i++)
    {
        std::string curFile = (folderPath + faces[i]);
        stbi_set_flip_vertically_on_load(false);
        unsigned char* colors = stbi_load(curFile.c_str(), &width, &height, &channels, 0);
        if (colors == nullptr)
            fprintf(stderr, "Failed to load image '%s'\n", curFile.c_str());
        else
            printf("Load image '%s' (%dx%d %d channels)\n", curFile.c_str(), width, height, channels);

        GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, colors);

        stbi_image_free(colors);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void gl::SetTextureDefaultParams(bool genMipmap)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (genMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    if (GLAD_GL_EXT_texture_filter_anisotropic)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.f);
}