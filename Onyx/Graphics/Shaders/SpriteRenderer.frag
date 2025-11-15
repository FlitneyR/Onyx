#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout( location = 0 ) in vec2 i_uv;
layout( location = 1 ) in flat uint i_textureIdx;

layout( location = 0 ) out vec4 o_colour;

layout( set = 0, binding = 1 ) uniform sampler2D u_textures[];

void main()
{
	o_colour = texture( u_textures[ nonuniformEXT( i_textureIdx ) ], i_uv );
}
