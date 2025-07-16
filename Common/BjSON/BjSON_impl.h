#pragma once
#include "BjSON.h"

#include <map>
#include <mutex>

namespace BjSON
{

struct Reader
{
	template< typename T >
	u32 Read( u32& offset, T* dest, u32 count = 1 )
	{
		const u32 bytes = Read( offset, (void*)dest, count * sizeof( T ) );
		offset += bytes;
		return bytes;
	}

	template< typename T >
	T Read( u32& offset )
	{
		T result;
		BjSON_ASSERT( Read( offset, &result ) == sizeof( T ) );
		return result;
	}

protected:
	virtual u32 Read( u32 offset, void* dest, u32 size ) = 0;
};

struct Writer
{
	template< typename T >
	void Write( u32& offset, T* src, u32 count = 1 )
	{
		Write( offset, (void*)src, count * sizeof( T ) );
		offset += count * sizeof( T );
	}

	template< typename T >
	void Write( u32& offset, const T& val )
	{
		Write( offset, (void*)&val, sizeof( T ) );
		offset += sizeof( T );
	}

protected:
	virtual void Write( u32 offset, void* src, u32 size ) = 0;
};

struct BufferReader : Reader
{
	const byte* m_buffer;
	u32 m_size;

	BufferReader( const void* buffer, u32 size );
	u32 Read( u32 offset, void* dest, u32 size ) override;
};

struct FileReader : Reader
{
	std::ifstream& m_file;
	std::mutex m_mutex;

	FileReader( std::ifstream& file );
	u32 Read( u32 offset, void* dest, u32 size ) override;
};

struct BufferWriter : Writer
{
	std::vector< byte >& m_buffer;

	BufferWriter( std::vector< byte >& dest ) : m_buffer( dest ) { m_buffer.clear(); }

	void Write( u32 offset, void* src, u32 size ) override;
};

struct MemberReference
{
	NameHash name;
	u32 offset;
};

struct ReadOnlyObject : IReadOnlyObject
{
	Reader& m_reader;
	u32 m_baseAddress;
	std::vector< MemberReference > m_header;

	ReadOnlyObject( Reader& reader, u32 base_address = 0 );

	const MemberReference* GetMemberReference( NameHash name ) const;

	u32 GetLiteral( NameHash name, void* dest, u32 dest_size ) const override;
	std::shared_ptr< const IReadOnlyObject > GetChild( NameHash name ) const override;
	std::shared_ptr< const IReadOnlyObjectArray > GetArray( NameHash name ) const override;
	u32 GetMemberCount() const override { return (u32)m_header.size(); }
	NameHash GetMemberName( u32 index ) const override { return m_header[ index ].name; }
	bool HasMember( NameHash name ) const override { return GetMemberReference( name ) != nullptr; }
};

struct ReadOnlyObjectArray : IReadOnlyObjectArray
{
	Reader& m_reader;
	u32 m_baseAddress;
	std::vector< u32 > m_header;

	ReadOnlyObjectArray( Reader& reader, u32 base_address = 0 );

	u32 Count() const override { return (u32)m_header.size(); }
	std::shared_ptr< const IReadOnlyObject > GetChild( u32 index ) const override;
};

struct DecoderImpl : IDecoder
{
	std::unique_ptr< Reader > m_reader;
	ReadOnlyObject m_rootObject;

	DecoderImpl( const void* buffer, u32 size );
	DecoderImpl( std::ifstream& file );

	const IReadOnlyObject& GetRootObject() const override { return m_rootObject; }
};

struct IEncoderNode
{
	// writes this node into the writer at the requested address
	// returns how many bytes were written
	virtual void Write( Writer& writer, u32& offset ) const = 0;
};

struct ReadWriteBlob;
struct ReadWriteObject;
struct ReadWriteObjectArray;

struct ReadWriteBlob : IEncoderNode
{
	std::vector< byte > data;

	// IEncoderNode
	void Write( Writer& writer, u32& offset ) const override;
};

struct ReadWriteObject : IReadWriteObject, IEncoderNode
{
	std::map< NameHash, std::unique_ptr< IEncoderNode > > m_children;

	template< typename T >
	T* GetNamedChild( NameHash name ) const
	{
		const auto child = m_children.find( name );
		return child == m_children.end() ? nullptr : dynamic_cast< T* >( child->second.get() );
	}

	template< typename T, typename ... Args >
	T& AddNamedChild( NameHash name, Args ... args )
	{
		BjSON_ASSERT( m_children.find( name ) == m_children.end() );

		std::unique_ptr< T > new_child = std::make_unique< T >( args ... );
		T& result = *new_child;
		m_children.insert( { name, std::move( new_child ) } );

		return result;
	}

	// IReadWriteObject
	IReadWriteObject& SetLiteral( NameHash name, const void* data, u32 size ) override
	{
		ReadWriteBlob* blob = GetNamedChild< ReadWriteBlob >( name );

		if ( !blob )
			blob = &AddNamedChild< ReadWriteBlob >( name );

		blob->data.clear();
		blob->data.resize( size );
		std::memcpy( blob->data.data(), data, size );

		return *this;
	}

	const void* GetLiteral( NameHash name, u32* out_size = nullptr ) const override
	{
		ReadWriteBlob* blob = GetNamedChild< ReadWriteBlob >( name );
		return !blob ? nullptr : blob->data.data();
	}

	IReadWriteObject& AddChild( NameHash name ) override;
	IReadWriteObject* GetChild( NameHash name ) const override;

	IReadWriteObjectArray& AddArray( NameHash name ) override;
	IReadWriteObjectArray* GetArray( NameHash name ) const override;

	// IEncoderNode
	void Write( Writer& writer, u32& offset ) const override;
};

struct ReadWriteObjectArray : IReadWriteObjectArray, IEncoderNode
{
	std::vector< std::unique_ptr< ReadWriteObject > > m_children;

	// IReadWriteObjectArray
	u32 Count() const override { return (u32)m_children.size(); }
	IReadWriteObject& AddChild() override { m_children.push_back( std::make_unique< ReadWriteObject >() ); return *m_children.back(); }
	IReadWriteObject* GetChild( u32 index ) const override { return index >= Count() ? nullptr : m_children[ index ].get(); }

	// IEncoderNode
	void Write( Writer& writer, u32& offset ) const override;
};

struct EncoderImpl : IEncoder
{
	// either a data blob, or a list of named children, or an array of unnamed children, cannot be any combination
	// root is always a list of named children
	ReadWriteObject m_rootObject;

	EncoderImpl() = default;

	// IEncoder
	IReadWriteObject& GetRootObject() override { return m_rootObject; }
	void WriteTo( std::vector< byte >& dest ) const override
	{
		BufferWriter writer( dest );
		u32 offset = 0;
		m_rootObject.Write( writer, offset );
	}

	void WriteTo( std::ofstream& file ) const override
	{
		std::vector< byte > buffer;
		BufferWriter writer( buffer );

		u32 offset = 0;
		file.seekp( offset );

		m_rootObject.Write( writer, offset );
		file.write( (char*)buffer.data(), buffer.size() );
	}
};

}
