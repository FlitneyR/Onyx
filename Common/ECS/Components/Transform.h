#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "Common/ECS/Entity.h"

namespace onyx
{

struct Transform2D
{
private:
	glm::mat3 m_locale = glm::mat3( 1.f );
	glm::mat3 m_inverseLocale = glm::mat3( 1.f );

	glm::vec2 m_position {};
	glm::vec2 m_scale { 1.f, 1.f };
	f32 m_rotation {};

	glm::mat3 CalculateMatrix() const
	{
		glm::mat3 result = m_locale;

		result = glm::translate( result, m_position );
		result = glm::rotate( result, m_rotation );
		result = glm::scale( result, m_scale );

		return result;
	}

	glm::mat3 m_matrix = CalculateMatrix();

public:
	void SetLocale( const glm::mat3& locale ) { m_locale = locale; m_inverseLocale = glm::inverse( locale ); m_matrix = CalculateMatrix(); }
	void SetLocalPosition( const glm::vec2& position ) { m_position = position; m_matrix = CalculateMatrix(); }
	void SetLocalScale( const glm::vec2& scale ) { m_scale = scale; m_matrix = CalculateMatrix(); }
	void SetLocalRotation( f32 rotation ) { m_rotation = rotation; m_matrix = CalculateMatrix(); }

	const glm::mat3& GetLocale() const { return m_locale; }
	const glm::vec2& GetLocalPosition() const { return m_position; }
	const glm::vec2& GetLocalScale() const { return m_scale; }
	const f32& GetLocalRotation() const { return m_rotation; }

	glm::vec2 GetWorldPosition() const { return LocalToWorld( {} ); }

	glm::vec2 GetRelative( glm::vec2 relative ) const { return m_matrix * glm::vec3( relative, 0.f ); }
	glm::vec2 LocalToWorld( glm::vec2 local ) const { return m_locale * glm::vec3( local, 1.f ); }
	glm::vec2 WorldToLocal( glm::vec2 local ) const { return m_inverseLocale * glm::vec3( local, 1.f ); }

	const glm::mat3& GetMatrix() const { return m_matrix; }
};

struct AttachedTo
{
	onyx::ecs::EntityID localeEntity;
};

}
