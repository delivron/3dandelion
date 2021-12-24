#pragma once

#include "cube.h"

namespace {

std::vector<ddn::VertexData> CreateCubeVertexBuffer()
{
    std::vector<ddn::VertexData> vertices(8);
    vertices[0] = { { 1.0f, -1.0f,  1.0f }, { 1.0f, 0.66f, 1.0f } };
    vertices[1] = { { 1.0f,  1.0f,  1.0f }, { 0.0f, 0.66f, 1.0f } };
    vertices[2] = { {-1.0f,  1.0f,  1.0f }, { 0.0f, 0.66f, 1.0f } };
    vertices[3] = { {-1.0f, -1.0f,  1.0f }, { 0.0f, 0.66f, 1.0f } };
    vertices[4] = { { 1.0f, -1.0f, -1.0f }, { 0.0f, 0.66f, 1.0f } };
    vertices[5] = { { 1.0f,  1.0f, -1.0f }, { 0.0f, 0.66f, 1.0f } };
    vertices[6] = { {-1.0f,  1.0f, -1.0f }, { 1.0f, 0.66f, 1.0f } };
    vertices[7] = { {-1.0f, -1.0f, -1.0f }, { 0.0f, 0.66f, 1.0f } };
    return vertices;
}

std::vector<uint16_t> CreateCubeIndexBuffer()
{
    std::vector<uint16_t> indexes = {
        0, 1, 2, 2, 3, 0, // front
        7, 6, 5, 5, 4, 7, // back
        3, 2, 6, 6, 7, 3, // left
        4, 5, 1, 1, 0, 4, // right
        6, 2, 1, 1, 5, 6, // top
        3, 7, 4, 4, 0, 3  // bottom
    };
    return indexes;
}

}

namespace ddn
{

Cube::Cube()
    : Mesh<VertexData, uint16_t>(CreateCubeVertexBuffer(), CreateCubeIndexBuffer())
{}

}  // namespace ddn
