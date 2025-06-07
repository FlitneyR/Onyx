#pragma once

#include "Common/AssetManager.h"

namespace onyx
{

struct IScriptContext
{
};

struct IScriptNodePinSet
{
	virtual const char* const* GetPinNames( u32& count ) const = 0;
	virtual void* GetPinData( BjSON::NameHash pin_name ) = 0;
	virtual u32 GetPinSize( BjSON::NameHash pin_name ) const = 0;
	virtual const char* GetPinTypeName( BjSON::NameHash pin_name ) const = 0;
};

struct IScriptNode
{
	virtual IScriptNodePinSet& GetInputs() = 0;
	virtual IScriptNodePinSet& GetOutputs() = 0;
	virtual u32 Exec( IScriptContext& ctx ) = 0;
};

struct Script : IAsset
{
	struct DataLink
	{
		BjSON::NameHash m_sourceNode;
		BjSON::NameHash m_sourcePin;
		u32 m_sourceNodeIndexCache;
	};

	struct ExecLink
	{
		BjSON::NameHash m_nodeName;
		u32 m_nodeIndexCache;
	};

	struct NodeWrapper
	{
		std::unique_ptr< IScriptNode > m_node;
		std::vector< DataLink > m_inputLinks;
		std::vector< ExecLink > m_execLinks;
		std::string m_name;
		BjSON::NameHash m_nameHash;
	};

	std::vector< NodeWrapper > m_nodes;
	BjSON::NameHash m_entryPoint;

	void Deserialise( BjSON::IReadOnlyObject& source );
	void Serialise( BjSON::IReadWriteObject& dest );
};

}
