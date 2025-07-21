#include "Graphics2D.h"

#include "Common/ECS/ComponentReflector.h"
#include "Common/ECS/Scene.h"

#include "imgui_stdlib.h"

using Sprite = onyx::Graphics2D::Sprite;
using SpriteAnimator = onyx::Graphics2D::SpriteAnimator;
using ParallaxBackground = onyx::Graphics2D::ParallaxBackground;

COMPONENT_REFLECTOR( Sprite )
{
	COMPONENT_REFLECTOR_HEADER( Sprite );

	#define xproperties( f )\
		f( Sprite, std::shared_ptr< TextureAsset >, textureAsset, "Texture" )\
		f( Sprite, glm::vec2, offset, "Offset" )\
		f( Sprite, glm::vec2, extent, "Extent" )\
		f( Sprite, u32, layer, "Layer" )\

	DEFAULT_SERIALISE_COMPONENT( Sprite, xproperties );
	DEFAULT_SERIALISE_COMPONENT_EDITS( Sprite, xproperties );
	DEFAULT_DESERIALISE_COMPONENT( Sprite, xproperties );

	DO_COMPONENT_EDITOR_UI()
	{
		BEGIN_COMPONENT_EDITOR_UI( Sprite, component );
		xproperties( DEFAULT_PROPERTY_EDITOR_UI );

		if ( __any_edits && component.textureAsset )
			component.texture = component.textureAsset->GetGraphicsResource();
	}

	#undef xproperties
};

COMPONENT_REFLECTOR( SpriteAnimator )
{
	COMPONENT_REFLECTOR_HEADER( SpriteAnimator );

	#define xproperties( f )\
		f( SpriteAnimator, std::shared_ptr< TextureAnimationAsset >, animation, "Animation" )\
		f( SpriteAnimator, f32, playRate, "Play Rate" )\
		f( SpriteAnimator, bool, loop, "Loop" )\

	DEFAULT_REFLECTOR( SpriteAnimator, xproperties )

	#undef xproperties
};

COMPONENT_REFLECTOR( ParallaxBackground )
{
	COMPONENT_REFLECTOR_HEADER( ParallaxBackground );

	#define xproperties( f )\
		f( ParallaxBackground, glm::vec2, scrollRate, "ScrollRate" )\
		f( ParallaxBackground, glm::vec2, scale, "Scale" )\
	
	DEFAULT_REFLECTOR( ParallaxBackground, xproperties )
	#undef xproperties
};

DEFINE_COMPONENT_REFLECTOR( Sprite );
DEFINE_COMPONENT_REFLECTOR( SpriteAnimator );
DEFINE_COMPONENT_REFLECTOR( ParallaxBackground );

void onyx::Graphics2D::RegisterReflectors( onyx::ecs::ComponentReflectorTable& table )
{
	REGISTER_COMPONENT_REFLECTOR( table, Sprite );
	REGISTER_COMPONENT_REFLECTOR( table, SpriteAnimator );
	REGISTER_COMPONENT_REFLECTOR( table, ParallaxBackground );
}
