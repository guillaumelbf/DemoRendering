// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <tiny_obj_loader.h>

#include "calc.hpp"

#include "mesh_builder.hpp"

#define OBJ_CACHE_VERSION 1

struct FullVertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float4 color;
    float4 tangent;
};

static void ConvertVertices(void* dst, const FullVertex* src, int count, const VertexDescriptor& descriptor)
{
    unsigned char* dstBuffer = (unsigned char*)dst;
    for (int i = 0; i < count; ++i)
    {
        const FullVertex* srcVertex = src + i;
        unsigned char* vertexStart = dstBuffer + (i * descriptor.size);

        float3* dstPos = (float3*)(vertexStart + descriptor.positionOffset);
        *dstPos = srcVertex->position;

        if (descriptor.hasNormal)
        {
            float3* dstNormal = (float3*)(vertexStart + descriptor.normalOffset);
            *dstNormal = srcVertex->normal;
        }

        if (descriptor.hasUV)
        {
            float2* dstUV = (float2*)(vertexStart + descriptor.uvOffset);
            *dstUV = srcVertex->uv;
        }

        if (descriptor.hasColor)
        {
            float4* dstColor = (float4*)(vertexStart + descriptor.colorOffset);
            *dstColor = srcVertex->color;
        }

        if (descriptor.hasTangent)
        {
            float4* dstTangent = (float4*)(vertexStart + descriptor.tangentOffset);
            *dstTangent = srcVertex->tangent;
        }
    }
}

MeshBuilder::MeshBuilder(const VertexDescriptor& descriptor, void** verticesPtr, int* vertexCount)
    : descriptor(descriptor)
    , verticesPtr(verticesPtr)
    , vertexCount(vertexCount)
{
}

void* MeshBuilder::GetDst(int* startIndex, int count)
{
    void* dst;

    // TODO: Check space is sufficient if startIndex is set
    if (startIndex)
        dst = (unsigned char*)(*verticesPtr) + *startIndex * descriptor.size;
    else
        dst = Grow(count);

    return dst;
}

void* MeshBuilder::Grow(int count)
{
    int oldCount = *vertexCount;
    *vertexCount += count;
    void* newPtr = realloc(*verticesPtr, *vertexCount * descriptor.size);
    *verticesPtr = newPtr;

    return (unsigned char*)*verticesPtr + (oldCount * descriptor.size);
}

MeshSlice MeshBuilder::GenTriangle(int* startIndex)
{
    FullVertex vertices[] =
    {
        // position             normal             uv              color
        { { 0.5f,-0.5f, 0.f }, { 0.f, 0.f, 1.f }, { 1.0f, 0.0f }, { 1.f, 0.f, 0.f, 1.f } },
        { {-0.5f,-0.5f, 0.f }, { 0.f, 0.f, 1.f }, { 0.0f, 0.0f }, { 0.f, 1.f, 0.f, 1.f } },
        { { 0.0f, 0.5f, 0.f }, { 0.f, 0.f, 1.f }, { 0.5f, 1.0f }, { 0.f, 0.f, 1.f, 1.f } },
    };

    int count = ARRAYSIZE(vertices);

    ConvertVertices(GetDst(startIndex, count), vertices, count, descriptor);

    return { *vertexCount - count, count };
}

MeshSlice MeshBuilder::GenQuad(int* startIndex, float halfWidth, float halfHeight)
{
    FullVertex vertices[] =
    {
        // position                        normal             uv            color
        { { halfWidth,-halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
        { {-halfWidth,-halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
        { { halfWidth, halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },

        { { halfWidth, halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },
        { {-halfWidth, halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },
        { {-halfWidth,-halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
    };
    int count = ARRAYSIZE(vertices);

    ConvertVertices(GetDst(startIndex, count), vertices, count, descriptor);

    return { *vertexCount - count, count };
}

MeshSlice MeshBuilder::GenIcosphereFace(int* index, float3 a, float3 b, float3 c, int depth)
{
    int startIndex = *index;
    if (depth == 0)
    {
        a = v3Normalize(a);
        b = v3Normalize(b);
        c = v3Normalize(c);

        FullVertex vertices[] =
        {
            { a * 0.5f, a, { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
            { b * 0.5f, b, { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
            { c * 0.5f, c, { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
        };
        int count = ARRAYSIZE(vertices);
        ConvertVertices(GetDst(index, count), vertices, count, descriptor);
        *index += count;
    }
    else
    {
        float3 mab = a + (b-a) * 0.5f;
        float3 mbc = b + (c-b) * 0.5f;
        float3 mca = c + (a-c) * 0.5f;

        GenIcosphereFace(index, a, mab, mca, depth-1);
        GenIcosphereFace(index, b, mbc, mab, depth-1);
        GenIcosphereFace(index, c, mca, mbc, depth-1);
        GenIcosphereFace(index, mab, mbc, mca, depth-1);
    }

    return { startIndex, *index - startIndex };
}

MeshSlice MeshBuilder::GenIcosphere(int* startIndex, int depth)
{
    // Create iscosahedron positions (radius = 1)
    float t = 1.f + sqrtf(5.f) / 2.f; // Golden ratio
    float h = t;
    float w = 1.f;

    float3 positions[]
    {
        {-w, h, 0.f },
        { w, h, 0.f },
        {-w,-h, 0.f },
        { w,-h, 0.f },

        { 0.f,-w, h },
        { 0.f, w, h },
        { 0.f,-w,-h },
        { 0.f, w,-h },

        { h, 0.f,-w },
        { h, 0.f, w },
        {-h, 0.f,-w },
        {-h, 0.f, w },
    };

    // Triangles
    int indices[] =
    {
        0, 11,  5,
        0,  5,  1,
        0,  1,  7,
        0,  7, 10,
        0, 10, 11,

        1,  5,  9,
        5, 11,  4,
       11, 10,  2,
       10,  7,  6,
        7,  1,  8,

        3,  9,  4,
        3,  4,  2,
        3,  2,  6,
        3,  6,  8,
        3,  8,  9,

        4,  9,  5,
        2,  4, 11,
        6,  2, 10,
        8,  6,  7,
        9,  8,  1,
    };

    int count = (int)(ARRAYSIZE(indices) * calc::Pow(4, (float)depth));
    GetDst(startIndex, count);
    int index = *vertexCount - count;
    int indexTmp = index;

    for (int i = 0; i < ARRAYSIZE(indices); i += 3)
        GenIcosphereFace(&indexTmp, positions[indices[i+0]], positions[indices[i+1]], positions[indices[i+2]], depth);

    return { index, count };
}

MeshSlice MeshBuilder::GenUVSphere(int* startIndex, int lat, int lon)
{
    int count = lon * lat * 6;
    GetDst(startIndex, count);
    int index = *vertexCount - count;
    MeshSlice slice = { index, count };

    for (int i = 0; i < lat; ++i)
    {
        // Theta varies from 0 to 180
        float theta        = calc::TAU / 2.f * (float)(i+0) / lat;
        float thetaCos     = calc::Cos(theta);
        float thetaSin     = calc::Sin(theta);

        float thetaNext    = calc::TAU / 2.f * (float)(i+1) / lat;
        float thetaNextCos = calc::Cos(thetaNext);
        float thetaNextSin = calc::Sin(thetaNext);

        for (int j = 0; j < lon; ++j)
        {
            // Phi varies from 0 to 360
            float phi        = calc::TAU * (float)(j+0) / lon;
            float phiCos     = calc::Cos(phi);
            float phiSin     = calc::Sin(phi);

            float phiNext    = calc::TAU * (float)(j+1) / lon;
            float phiNextCos = calc::Cos(phiNext);
            float phiNextSin = calc::Sin(phiNext);

            // Compute positions
            float3 p0 = {     thetaSin * phiCos,         thetaCos,     thetaSin * phiSin     };
            float3 p1 = {     thetaSin * phiNextCos,     thetaCos,     thetaSin * phiNextSin };
            float3 p2 = { thetaNextSin * phiCos,     thetaNextCos, thetaNextSin * phiSin     };
            float3 p3 = { thetaNextSin * phiNextCos, thetaNextCos, thetaNextSin * phiNextSin };
            float3 t0 = {                phiSin,              0.f,                phiCos     };
            float3 t1 = {            phiNextSin,              0.f,                phiNextCos };
            float3 t2 = {                phiSin,              0.f,                phiCos     };
            float3 t3 = {            phiNextSin,              0.f,                phiNextCos };

            FullVertex quad[6];
            quad[0].position = quad[0].normal = p0;
            quad[1].position = quad[1].normal = p1;
            quad[2].position = quad[2].normal = p2;
            quad[3].position = quad[3].normal = p2;
            quad[4].position = quad[4].normal = p1;
            quad[5].position = quad[5].normal = p3;

            for (int i = 0; i < 6; ++i)
                quad[i].position *= 0.5f; // pos between -0.5 and 0.5 (unit sphere)
            
            quad[0].tangent = float4(t0, 1.f);
            quad[1].tangent = float4(t1, 1.f);
            quad[2].tangent = float4(t2, 1.f);
            quad[3].tangent = float4(t2, 1.f);
            quad[4].tangent = float4(t1, 1.f);
            quad[5].tangent = float4(t3, 1.f);

            // Set uv
            float u0 = 1.f - (j + 0.f) / (float)lon;
            float u1 = 1.f - (j + 1.f) / (float)lon;
            float v0 = 1.f - (i + 0.f) / (float)lat;
            float v1 = 1.f - (i + 1.f) / (float)lat;

            quad[0].uv = { u0, v0 };
            quad[1].uv = { u1, v0 };
            quad[2].uv = { u0, v1 };
            quad[3].uv = quad[2].uv;
            quad[4].uv = quad[1].uv;
            quad[5].uv = { u1, v1 };

            ConvertVertices(GetDst(&index, 6), quad, 6, descriptor);
            index += 6;
        }
    }

    return slice;
}

// Implement dumb caching to avoid parsing .obj again and again
static bool LoadObjFromCache(std::vector<FullVertex>& mesh, const char* filename)
{
    std::string cachedFile = filename;
    cachedFile += ".cache";

    FILE* file = fopen(cachedFile.c_str(), "rb");
    if (file == nullptr)
        return false;

    size_t version;
    fread(&version, sizeof(size_t), 1, file);
    if (version != OBJ_CACHE_VERSION)
    {
        printf("Cached version mismatch for %s, reload...\n", filename);
        return false;
    }

    size_t vertexCount = 0;
    fread(&vertexCount, sizeof(size_t), 1, file);
    mesh.resize(vertexCount);
    fread(mesh.data(), sizeof(FullVertex), vertexCount, file);
    fclose(file);

    printf("Model loaded from cache: %s (%d vertices)\n", filename, (int)vertexCount);

    return true;
}

static void SaveObjToCache(const std::vector<FullVertex>& mesh, const char* filename)
{
    std::string cachedFile = filename;
    cachedFile += ".cache";

    FILE* file = fopen(cachedFile.c_str(), "wb");
    size_t version = OBJ_CACHE_VERSION;
    fwrite(&version, sizeof(size_t), 1, file);
    size_t vertexCount = mesh.size();
    fwrite(&vertexCount, sizeof(size_t), 1, file);
    fwrite(&mesh[0], sizeof(FullVertex), vertexCount, file);
    fclose(file);

    printf("Model saved to cache: %s (%d vertices)\n", filename, (int)vertexCount);
}

MeshSlice MeshBuilder::LoadObj(int* startIndex, const char* objFile, const char* mtlDir, float scale)
{
    std::vector<FullVertex> vertices;
    if (!LoadObjFromCache(vertices, objFile))
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warning;
        std::string error;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, objFile, mtlDir, true);
        if (!warning.empty())
            printf("%s\n", warning.c_str());

        if (!error.empty())
            fprintf(stderr, "%s\n", error.c_str());

        if (!ret)
            return { 0, 0 };

        // TODO: Precompute total vertex count to prealloc

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++)
        {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            {
                int fv = shapes[s].mesh.num_face_vertices[f];

                // Loop over vertices in the face.
                for (int v = 0; v < fv; v++)
                {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    FullVertex vert = {};

                    vert.position.x = attrib.vertices[3 * idx.vertex_index + 0];
                    vert.position.y = attrib.vertices[3 * idx.vertex_index + 1];
                    vert.position.z = attrib.vertices[3 * idx.vertex_index + 2];

                    if (!attrib.normals.empty())
                    {
                        vert.normal.x = attrib.normals[3 * idx.normal_index + 0];
                        vert.normal.y = attrib.normals[3 * idx.normal_index + 1];
                        vert.normal.z = attrib.normals[3 * idx.normal_index + 2];
                    }

                    if (!attrib.texcoords.empty())
                    {
                        vert.uv.u = attrib.texcoords[2 * idx.texcoord_index + 0];
                        vert.uv.v = attrib.texcoords[2 * idx.texcoord_index + 1];
                    }

                    // Optional: vertex colors
                    if (!attrib.colors.empty())
                    {
                        vert.color.r = attrib.colors[3 * idx.vertex_index + 0];
                        vert.color.g = attrib.colors[3 * idx.vertex_index + 1];
                        vert.color.b = attrib.colors[3 * idx.vertex_index + 2];
                    }

                    vertices.push_back(vert);
                }
                index_offset += fv;
            }
        }

        SaveObjToCache(vertices, objFile);
    }

    for (FullVertex& vertex : vertices)
        vertex.position *= scale;

    int count = (int)vertices.size();
    ConvertVertices(GetDst(startIndex, count), vertices.data(), count, descriptor);

    return { *vertexCount - count, count };
}
