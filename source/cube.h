#pragma once

#include "mesh.h"

#include <glm/vec3.hpp>

namespace ddn
{

struct VertexData
{
    glm::vec3 position;
    glm::vec3 color;
};

class Cube : public Mesh<VertexData, uint16_t>
{
public:
    Cube();
};

}  // namespace ddn
