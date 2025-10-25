#version 450

layout( location = 0 ) in vec2 i_uv;
layout( location = 1 ) in flat uint i_textureIdx;

layout( location = 0 ) out vec4 o_colour;

layout( set = 0, binding = 1 ) uniform sampler2D u_textures[ 1000 ];

void main()
{
	o_colour = texture( u_textures[ i_textureIdx ], i_uv );
}
