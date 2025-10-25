#include "World.h"

#include "Modules/Core.h"

namespace onyx::ecs
{

void World::RemoveEntity( EntityID entity, bool and_children )
{
	for ( auto& [hash, table] : m_componentTables )
		table->RemoveComponent( entity );

	if ( and_children )
	{
		if ( ComponentTable< onyx::Core::AttachedTo >* attached_to_table = GetOptionalComponentTable< onyx::Core::AttachedTo >() )
		{
			std::vector< EntityID > children;

			for ( auto attached_to_iter = attached_to_table->Iter(); attached_to_iter; ++attached_to_iter )
				if ( attached_to_iter.Component().localeEntity == entity )
					children.push_back( attached_to_iter.ID() );

			for ( EntityID child : children )
				RemoveEntity( child, and_children );
		}
	}
}

}
