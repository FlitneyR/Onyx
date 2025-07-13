#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "Common/ECS/Entity.h"
#include "Common/ECS/Query.h"
#include "Common/ECS/CommandBuffer.h"

#include "Common/ECS/ComponentReflector.h"

namespace onyx::Core
{

void RegisterReflectors( onyx::ecs::ComponentReflectorTable& table );

struct Name
{
	std::string name = "";
};

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
		result = glm::rotate( result, glm::radians( m_rotation ) );
		result = glm::scale( result, m_scale );

		return result;
	}

	glm::mat3 m_matrix = CalculateMatrix();
	void Refresh() { m_matrix = CalculateMatrix(); }

public:
	void SetLocale( const glm::mat3& locale ) { m_locale = locale; m_inverseLocale = glm::inverse( m_locale ); Refresh(); }
	void SetLocalPosition( const glm::vec2& position ) { m_position = position; Refresh(); }
	void SetLocalScale( const glm::vec2& scale ) { m_scale = scale; Refresh(); }
	void SetLocalRotation( f32 rotation ) { m_rotation = rotation; Refresh(); }

	const glm::mat3& GetLocale() const { return m_locale; }
	const glm::vec2& GetLocalPosition() const { return m_position; }
	const glm::vec2& GetLocalScale() const { return m_scale; }
	const f32& GetLocalRotation() const { return m_rotation; }

	glm::vec2 GetWorldPosition() const { return m_matrix[ 2 ]; }
	glm::vec2 GetWorldScale() const { return glm::vec2( glm::length( m_matrix[ 0 ] ), glm::length( m_matrix[ 1 ] ) ); }
	f32 GetWorldRotation() const { return atan2( m_matrix[ 0 ].y, m_matrix[ 0 ].x ); }

	glm::vec2 GetRelative( glm::vec3 relative ) const { return m_matrix * relative; }
	glm::vec2 LocalToWorld( glm::vec2 local ) const { return m_locale * glm::vec3( local, 1.f ); }
	glm::vec2 WorldToLocal( glm::vec2 local ) const { return m_inverseLocale * glm::vec3( local, 1.f ); }

	const glm::mat3& GetMatrix() const { return m_matrix; }

	COMPONENT_REFLECTOR_FRIEND( Transform2D );
};

struct AttachedTo
{
	onyx::ecs::EntityID localeEntity;
};

using UpdateTransform2DLocales_Transforms = onyx::ecs::Query<
	onyx::ecs::Read< Transform2D >
>;

using UpdateTransform2DLocales_AttachedTransforms = onyx::ecs::Query<
	onyx::ecs::Write< Transform2D >,
	onyx::ecs::Read< AttachedTo >
>;

void UpdateTransform2DLocales( onyx::ecs::Context<> ctx, const UpdateTransform2DLocales_Transforms& parents, const UpdateTransform2DLocales_AttachedTransforms& children );

void PostCopyUpdateRootTransforms2D( const ecs::World& world, const ecs::IDMap& id_map, const glm::mat3& transform );

}
