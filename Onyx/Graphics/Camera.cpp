#include "Camera.h"

#include "glm/gtx/matrix_transform_2d.hpp"

namespace onyx
{

glm::mat3 Camera2D::GetMatrix() const
{
	glm::mat3 transform = glm::mat3( 1.f );

	transform = glm::translate( transform, position );
	transform = glm::rotate( transform, rotation );
	transform = glm::scale( transform, fov * aspectRatio );

	return glm::inverse( transform );
}

}
