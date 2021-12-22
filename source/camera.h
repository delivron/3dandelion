#pragma once

#include <glm/mat4x4.hpp>

namespace ddn
{

class Camera
{
public:
    Camera(float fov_y_deg, float aspect, float near_z, float far_z);

    void SetAspect(float aspect); 
    float GetAspect() const;

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
