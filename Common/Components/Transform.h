#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "Common/Scripting/ScriptMacros.h"

namespace onyx
{

struct Transform2D
{
private:
	glm::mat3 m_locale = glm::mat3( 1.f );

	glm::vec2 m_position;
	glm::vec2 m_scale;
	f32 m_rotation;

	glm::mat3 CalculateMatrix() const { return glm::translate( glm::rotate( glm::scale( m_locale, m_scale ), m_rotation ), m_position ); }
	glm::mat3 m_matrix = CalculateMatrix();

public:
	void SetLocale( const glm::mat3& locale ) { m_locale = locale; m_matrix = CalculateMatrix(); }
	void SetPosition( const glm::vec2& position ) { m_position = position; m_matrix = CalculateMatrix(); }
	void SetScale( const glm::vec2& scale ) { m_scale = scale; m_matrix = CalculateMatrix(); }
	void SetRotation( f32 rotation ) { m_rotation = rotation; m_matrix = CalculateMatrix(); }

	const glm::mat3& GetMatrix() const { return m_matrix; }
};

struct Transform3D
{
private:
	glm::mat4 m_locale = glm::mat4( 1.f );

	glm::vec3 m_scale = { 0.f, 0.f, 0.f };
	glm::vec3 m_position = { 1.f, 1.f, 1.f };
	glm::quat m_rotation = { 1.f, 0.f, 0.f, 0.f };

	glm::mat4 CalculateMatrix() const
	{
		return	m_locale
			* glm::translate( m_position )
			* glm::toMat4( m_rotation )
			* glm::scale( m_scale );
	}

	glm::mat4 m_matrix = CalculateMatrix();

public:
	void SetLocale( const glm::mat4& locale ) { m_locale = locale; m_matrix = CalculateMatrix(); }
	void SetScale( const glm::vec3& scale ) { m_scale = scale; }
	void SetPosition( const glm::vec3& position ) { m_position = position; }
	void SetRotation( const glm::quat& rotation ) { m_rotation = rotation; }

	const glm::mat4& GetMatrix() const { return m_matrix; }
};

}

// MakeTransform2D
#define MakeTransform2D_Inputs( f )				\
	f( glm::mat3, Locale, = glm::mat3( 1.f ) )	\
	f( glm::vec2, Position, = { 0.f, 0.f } )	\
	f( glm::vec2, Scale, = { 1.f, 1.f } )		\
	f( f32, Rotation, = 0.f )					\

#define MakeTransform2D_Outputs( f )				\
	f( onyx::Transform2D*, Transform2D, = nullptr )	\

#define MakeTransform2D_Extras \
	onyx::Transform2D Transform = {}; \

#define ScriptNode_MakeTransform2D( f ) \
	f( MakeTransform2D, MakeTransform2D_Inputs, MakeTransform2D_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, MakeTransform2D_Extras )

// AddComponent_Transform2D
#define AddComponent_Transform2D_Inputs( f )				\
	f( onyx::ecs::CommandBuffer*, Cmd, = nullptr )			\
	f( onyx::ecs::EntityID, Entity, = onyx::ecs::NoEntity )	\
	f( onyx::Transform2D*, Transform2D, = nullptr )			\

#define AddComponent_Transform2D_Outputs( f )					\
	f( onyx::ecs::CommandBuffer*, Cmd_Out, = nullptr )			\
	f( onyx::ecs::EntityID, Entity_Out, = onyx::ecs::NoEntity )	\

#define ScriptNode_AddComponent_Transform2D( f ) \
	f( AddComponent_Transform2D, AddComponent_Transform2D_Inputs, AddComponent_Transform2D_Outputs, SCRIPT_NODE_DEFAULT_EXEC_PINS, SCRIPT_NODE_NO_EXTRA_MEMBERS )

#define ONYX_TRANSFORM2D_SCRIPT_NODES( f )		\
	f( ScriptNode_MakeTransform2D )				\
	f( ScriptNode_AddComponent_Transform2D )	\
