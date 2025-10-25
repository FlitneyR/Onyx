#pragma once

#include "Onyx/Assets.h"
#include "Onyx/ECS/World.h"
#include "Onyx/BjSON/BjSON.h"

#include "Onyx/ECS/ComponentReflector.h"

namespace onyx::ecs
{

struct Scene : IAsset
{
	// copy the entities in this scene to the destination world, and return a map from entity IDs in this scene, to the ids those entities have in the world
	void CopyToWorld( World& world, IDMap& entity_map ) const;

	// IAsset
	void Load( LoadType type ) override;
	void Save( BjSON::IReadWriteObject& writer, SaveType type ) override;
	void DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context ) override;

	World m_world;
};

struct SceneEditor : editor::IWindow
{
	struct IPreviewer
	{
		virtual ~IPreviewer() = default;
		virtual void Tick( onyx::IFrameContext& frame_context, std::shared_ptr< onyx::IRenderTarget >& render_target ) = 0;

		struct IFactory
		{
			virtual std::unique_ptr< IPreviewer > MakePreviewer( World& world ) = 0;

			inline static IFactory* s_singleton = nullptr;
		};
	};

	std::shared_ptr< Scene > m_scene;
	std::unique_ptr< IPreviewer > m_previewer;
	std::shared_ptr< onyx::IRenderTarget > m_renderTarget;

	inline static const char* const s_name = "Scene Editor";
	const char* GetName() const override { return s_name; }
	void Run( IFrameContext& frame_context ) override;
	std::string GetWindowTitle() const override;

private:

	EntityID m_selectedEntity = NoEntity;
	void DoRecursiveEntityHierarchy( const std::unordered_map< EntityID, std::vector< EntityID > >& hierarchy, EntityID entity );
};

// scene instance component
// added to each entity after copying a scene to an ecs world to track where it came from and what has been edited
struct SceneInstance
{
	std::shared_ptr< Scene > m_scene = nullptr;
	EntityID m_rootEntity = NoEntity;
	EntityID m_sceneEntityId = NoEntity;

	// What has been edited?
	//		  Component names			Property names
	std::map< BjSON::NameHash, std::set< BjSON::NameHash > > m_edits;
};

}

/*

// Structure of a serialised scene
{
	> Entities = [
		...
		{ // a normal entity, just a collection of components to be read
			> Transform = ...
			> Sprite = ...
		}
		...
		{ // an instance of another scene
			> SceneInstance = {
				> Scene = "/prefabs/enemy" // specify the other scene to load
				> Overrides = [ // and any modifications to be made after loading it
					{
						> __localEntityID = 0 // the id of the relevant entity in "/prefabs/enemy"
						> Transform = ...
						> Sprite = ...
					}
				]
			}
		}
		...
	]
	> NextEntityID = ... // make sure we keep track of the entity IDs we've already used, so we don't reuse one
}

// structure of a scene deserialised into a world
{
	> Entities = [
		...
		{ ... // a normal entity
			> Transform = ...
			> Sprite = ...
		}
		...
		{ X // an entity from an scene instance
			> SceneInstance = {
				> Scene = "/prefabs/enemy"
				> Root Entity = X // this is entity X, the root of this scene instance
				> Local ID = ... // the id of the entity in "/prefabs/enemy" that this entity maps to
			}
			... // some other components
		}
		{ ... // another entity from the same scene instance
			> SceneInstance = {
				> Scene = "/prefabs/enemy"
				> Root Entity = X // this is not the root of this scene instance
				> Local ID = ...
			}
			... // some other components
		}
		...
		{ ... // an entity from another instance of the same scene
			> SceneInstance = {
				> Scene = "/prefabs/enemy"
				> Root Entity = Y
				> Local ID = ...
			}
			... // some other components
		}
		...
		{ Z // an entity from an instance of another scene
			> SceneInstance = {
				> Scene = "/prefabs/wall"
				> Root Entity = Z
				> Local ID = ...
			}
			... // some other components
		}
		...
	]
	> Next Entity ID = ...
}

When loading a scene:
	- Iterate through the entities in the serialisation of the scene. For each entity
		- If it is not a scene instance
			- For each component in the serialisation
				- Look up a deserialisation function for that component type from the component function table
				- Add a default initialised component of that type to the entity
				- Call the deserialisation function passing in a reference to the component you just added
		- If it is a scene instance ( we should probably check that this scene isn't one we're already nested in, which would cause an infinite loop )
			- Load the scene instance by path from the same asset manager as this scene
			- Copy the entities in that scene's deserialised world to this world
				- If we are loading in editor mode, add a scene instance component and set the root entity id and local entity id of each copied entity
					( this is necessary for saving an edited scene, but not necessary in gameplay )
			- For each override
				- Map the entity id in the override, to the entity in the world
				- Warn and move on if the entity doesn't exist
				- For each component
					- Add the component if it doesn't exist
					- Look up a deserialisation function for that component type from the component function table
					- Call the deserialisation function passing in a reference to the already existant component

When saving a scene:
	- Iterate through the entities in the world. For each entity
		- If it is not part of a scene instance
			- Add an entity to the entity list in the serialisation
			- For each component present in the entity
				- Add a component to the entity's serialisation
				- Look up a serialisation function from the component function table
				- Serialise the component
		- If it is the root of a scene instance
			- Add a scene instance to the serialisation, setting the scene path
			- For each entity in the world that is part of this scene instance ( they should be immediately subsequent to this entity in the world )
				- For each component that is part of this entity
					- Look up a diff-serialisation function for that component type from the component function table
						- This function should only serialise values that are different between the two instances of the components
					- Use the diff-serialisation function to serialise an override for the same component of the entity in the scene that this entity maps to ( via the local entity ID )

When editing a scene:
	- Iterate through all the entities to build and display a scene hierarchy
	- Find the component iterators for the currently selected entity. For each component
		- Find the editor UI function from the component function table
		- Call the editor UI function, passing in a const pointer to the source entity if this is entity is part of a scene instance
			- The editor UI function should highlight any differences between this component and the source component, and offer options to revert those changes individually
	- If the currently selected entity is not part of a scene instance
		- Allow the user to Add or Remove any component
	- If the currently selected entity is part of a scene instance
		- Allow the user to Add or Remove any component that isn't part of the entity in the source scene

Data types:
	- Component function table
		- Needs to store functions for
			- Deserialising a component
			- Serialising a component
			- Displaying editor ui for a component
			- Serialising a diff of two components
		- Need to be able to look up these functions by
			- BjSON::NameHash and typeid( ... ).hash_code()

*/
