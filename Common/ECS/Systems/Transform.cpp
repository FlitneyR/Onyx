#include "Transform.h"

namespace onyx
{

void UpdateTransform2DLocales( onyx::ecs::Context<> ctx, const UpdateTransform2DLocales_Transforms& parents, const UpdateTransform2DLocales_AttachedTransforms& children )
{
	for ( auto& child : children )
	{
		auto [id, child_transform, attachment] = child.Break();

		if ( auto* parent = parents.Get( attachment.localeEntity ) )
		{
			auto [id, parent_transform] = parent->Break();

			child_transform.SetLocale( parent_transform.GetMatrix() );
		}
	}
}

}
