#include "camera.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace ddn
{

Camera::Camera(uint32_t width, uint32_t height, float fov_y_deg, float near_z, float far_z)
    : m_fov_y_rad(glm::radians(fov_y_deg))
    , m_aspect(static_cast<float>(width) / height)
    , m_near_z(near_z)
    , m_far_z(far_z)
    , m_position(0.0f)
{
}

void Camera::OnResize(uint32_t width, uint32_t height)
{
    m_aspect = static_cast<float>(width) / height;
}

void Camera::SetPosition(const glm::vec3& position)
{
    m_position = position;
}

glm::vec3 Camera::GetPosition() const
{
    return m_position;
}

glm::mat4 Camera::GetProjectionViewMatrix() const
{
    auto projection_matrix = glm::perspective(m_fov_y_rad, m_aspect, m_near_z, m_far_z);
    auto view_matrix = glm::lookAt(m_position, glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
    return projection_matrix * view_matrix;
}

}
