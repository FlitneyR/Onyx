#pragma once

#include "Common/Assets.h"
#include "Common/External/imnodes/imnodes.h"

#include <map>

namespace onyx
{

namespace ecs
{

struct World;
struct CommandBuffer;

}

struct IScriptNodePinSet
{
	virtual const char* const* GetPinNames( u32& count ) const = 0;
	virtual const BjSON::NameHash* GetPinNameHashes( u32& count ) const = 0;
	virtual void* GetPinData( BjSON::NameHash pin_name ) = 0;
	virtual void CopyPinData( BjSON::NameHash pin_name, void* source ) = 0;
	virtual const size_t* GetPinTypeID( BjSON::NameHash pin_name ) const = 0;
	virtual const char* GetPinTypeName( BjSON::NameHash pin_name ) const = 0;
	virtual u32 GetPinIndex( BjSON::NameHash pin_name ) const = 0;
};

struct ScriptContext
{
	struct IPin
	{
		virtual size_t GetTypeID() = 0;
		virtual void* GetData() = 0;
		virtual void SetData( void* data ) = 0;
	};

	IPin* GetInput( BjSON::NameHash name );
	IPin* GetOutput( BjSON::NameHash name );

	template< typename T >
	T* GetInput( BjSON::NameHash name )
	{
		if ( IPin* pin = GetInput( name ) )
			if ( pin->GetTypeID() == typeid( T ).hash_code() )
				return reinterpret_cast< T* >( pin->GetData() );

		return nullptr;
	}

	template< typename T >
	T* GetOutput( BjSON::NameHash name )
	{
		if ( IPin* pin = GetOutput( name ) )
			if ( pin->GetTypeID() == typeid( T ).hash_code() )
				return reinterpret_cast<T*>( pin->GetData() );

		return nullptr;
	}

	std::map< BjSON::NameHash, std::shared_ptr< IPin > > m_inputs;
	std::map< BjSON::NameHash, std::shared_ptr< IPin > > m_outputs;

	template< typename T >
	struct Pin : IPin
	{
		T data;
		std::string name;

		Pin( const char* name, T value )
			: name( name )
			, data( value )
		{}

		size_t GetTypeID() { return typeid( T ).hash_code(); }
		void* GetData() { return reinterpret_cast< void* >( &data ); }
		void SetData( void* in_data ) { data = *reinterpret_cast< T* >( in_data ); }
	};

	template< typename T >
	ScriptContext& AddInput( const char* name, T value = {} )
	{
		m_inputs.insert( { BjSON::HashName( name ), std::make_shared< Pin< T > >( name, value ) } );
		return *this;
	}

	template< typename T >
	ScriptContext& AddOutput( const char* name, T default_value = {} )
	{
		m_outputs.insert( { BjSON::HashName( name ), std::make_shared< Pin< T > >( name, default_value ) } );
		return *this;
	}
};

struct IScriptNode
{
	virtual const char* GetScriptNodeTypeName() const = 0;
	virtual IScriptNodePinSet& GetInputs() = 0;
	virtual IScriptNodePinSet& GetOutputs() = 0;
	virtual u32 Exec( ScriptContext& ctx ) = 0;
	virtual const char* const* GetExecPinNames( u32& count ) const = 0;
	virtual u32 GetExecPinIndex( BjSON::NameHash pin_name ) const = 0;
	virtual void DoCustomUI() {}
	virtual void DoCustomLoad( const BjSON::IReadOnlyObject& reader ) {}
	virtual void DoCustomSave( BjSON::IReadWriteObject& writer ) {}
};

void ScriptNodes_DoCustomUI( IScriptNode& node );
void ScriptNodes_DoCustomLoad( IScriptNode& node, const BjSON::IReadOnlyObject& reader );
void ScriptNodes_DoCustomSave( IScriptNode& node, BjSON::IReadWriteObject& writer );

struct Script final : IAsset
{
	struct NodeWrapper;

	struct DataLink
	{
		std::weak_ptr< NodeWrapper > m_sourceNode = {};
		BjSON::NameHash m_sourceNodeNameHash = 0;
		BjSON::NameHash m_sourcePin = 0;
	};

	struct ExecLink
	{
		std::weak_ptr< NodeWrapper > m_nextNode = {};
		BjSON::NameHash m_nextNodeNameHash = 0;
	};

	struct NodeWrapper
	{
		std::unique_ptr< IScriptNode > m_node;
		std::vector< DataLink > m_inputLinks; // links to where the data comes from for each input pin
		std::vector< ExecLink > m_execLinks; // links to which nodes to execute next for each exec pin
		const std::string m_name;
		const BjSON::NameHash m_nameHash;
		ImVec2 m_position = { 0, 0 };
		bool m_needsToSetPosition = true;

		NodeWrapper( const char* name )
			: m_name( name )
			, m_nameHash( BjSON::HashName( name ) )
		{}
	};

	std::vector< std::shared_ptr< NodeWrapper > > m_nodes;
	std::map< std::string, size_t > m_inputs;
	std::map< std::string, size_t > m_outputs;
	BjSON::NameHash m_entryPoint = 0;

	void Load( LoadType type ) override;
	void Save( BjSON::IReadWriteObject& writer, SaveType type ) override;
	void DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context ) override;
	std::vector< std::shared_ptr< NodeWrapper > >::iterator GetNode( BjSON::NameHash name_hash );
};

struct ScriptRunner
{
	ScriptRunner( std::shared_ptr< Script > script, ScriptContext& ctx );
	bool Run();
	bool IsRunning() const { m_currentNode; }

private:
	ScriptContext& m_context;
	std::shared_ptr< Script > m_script;
	std::shared_ptr< Script::NodeWrapper > m_currentNode;
};

struct ScriptEditor : editor::IWindow
{
	ImNodesContext* m_nodeEditorContext;

	std::unique_ptr< IScriptNode > m_newScriptNode;
	char m_newNodeName[ 256 ] = "";

	std::shared_ptr< Script > m_script = nullptr;
	std::string m_scriptPath = "";

	bool m_needsToSetAllNodePositions = true;
	bool m_selectEntryNode = false;
	bool m_showMiniMap = true;

	std::string m_newInputName = "";
	size_t m_newInputType = 0;
	i32 m_newInputDestPinID = 0;

	std::unordered_map< i32, std::pair< BjSON::NameHash, BjSON::NameHash > > m_uidToPin;

	ScriptEditor();
	~ScriptEditor();

	inline static const char* const s_name = "Script Editor";
	const char* GetName() const override { return s_name; }
	void Run( IFrameContext& frame_context ) override;
	std::string GetWindowTitle() const override;

	i32 GetPinID( BjSON::NameHash node_name, BjSON::NameHash pin_name );
	const std::pair< BjSON::NameHash, BjSON::NameHash >* GetPinFromID( i32 uid );
};

namespace ScriptNodes
{

// implement in your editor's ScriptNodes.cpp
extern std::unique_ptr< IScriptNode > DoCreateScriptNodeCombo();
extern std::unique_ptr< IScriptNode > ConstructFromTypeHash( u32 type_name_hash );

}

}
