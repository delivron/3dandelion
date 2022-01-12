#pragma once

#include "window.h"

#include <glm/mat4x4.hpp>

namespace ddn
{

class Camera
    : public IWindowListener
{
public:
    Camera(uint32_t width, uint32_t height, float fov_y_deg, float near_z, float far_z);

    void OnResize(uint32_t width, uint32_t height) override;

    void SetPosition(const glm::vec3& position);
    glm::vec3 GetPosition() const;

    glm::mat4 GetProjectionViewMatrix() const;

private:
    float m_fov_y_rad;
    float m_aspect;
    float m_near_z;
    float m_far_z;
    glm::vec3 m_position;
};

}  // namespace ddn
