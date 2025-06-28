#version 450

vec2 verts[4] = vec2[4](
	vec2( -0.5, -0.5 ),
	vec2(  0.5, -0.5 ),
	vec2(  0.5,  0.5 ),
	vec2( -0.5,  0.5 )
);

layout( location = 0 ) out vec2 o_uv;
layout( location = 1 ) out flat uint o_textureIdx;

layout( push_constant ) uniform Camera
{
	mat3 transform;
} pc_camera;

struct InstanceData
{
	mat3 transform;
	vec2 offset;
	vec2 extent;
	uint textureIdx;
};

layout( set = 0, binding = 0 ) readonly buffer Instances
{
	InstanceData u_instances[];
};

void main()
{
	const InstanceData instance = u_instances[ gl_InstanceIndex ];

	const vec2 local_vert = verts[ gl_VertexIndex ];
	const vec3 world_vert = instance.transform * vec3( local_vert, 1.0 );
	const vec3 camera_vert = pc_camera.transform * world_vert;
	
	gl_Position = vec4( camera_vert.xy, 0.0, 1.0 );
	o_uv = instance.offset + instance.extent * ( local_vert + 0.5 );
    o_textureIdx = instance.textureIdx;
}
