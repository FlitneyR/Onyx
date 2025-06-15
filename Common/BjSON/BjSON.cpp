#include "BjSON_impl.h"
#include "string.h"

namespace BjSON
{

Decoder::Decoder( const void* data, u32 size )
	: impl( std::make_unique< DecoderImpl >( data, size ) )
{}

Decoder::Decoder( std::ifstream& file )
	: impl( std::make_unique< DecoderImpl >( file ) )
{}

Encoder::Encoder()
	: impl( std::make_unique< EncoderImpl >() )
{}

BufferReader::BufferReader( const void* buffer, u32 size )
	: m_buffer( (byte*)buffer )
	, m_size( size )
{}

u32 BufferReader::Read( u32 offset, void* dest, u32 size )
{
	const u32 bytes_to_read = std::min( size, m_size - offset );
	memcpy( dest, m_buffer + offset, bytes_to_read );
	BjSON_ASSERT( bytes_to_read == size );
	return bytes_to_read;
}

FileReader::FileReader( std::ifstream& file )
	: m_file( file )
{}

u32 FileReader::Read( u32 offset, void* dest, u32 size )
{
	std::scoped_lock lock( m_mutex );

	return m_file
		.seekg( offset )
		.read( (char*)dest, size )
		.gcount();
}

void BufferWriter::Write( u32 offset, void* src, u32 size )
{
	if ( m_buffer.size() < offset + size )
		m_buffer.resize( offset + size );

	std::memcpy( m_buffer.data() + offset, src, size );
}

DecoderImpl::DecoderImpl( const void* buffer, u32 size )
	: m_reader( std::make_unique< BufferReader >( buffer, size ) )
	, m_rootObject( *m_reader )
{}

DecoderImpl::DecoderImpl( std::ifstream& file )
	: m_reader( std::make_unique< FileReader >( file ) )
	, m_rootObject( *m_reader )
{}

ReadOnlyObject::ReadOnlyObject( Reader& reader, u32 offset )
	: m_reader( reader )
	, m_baseAddress( offset )
{
	m_header.resize( m_reader.Read< u32 >( offset ) );
	m_reader.Read( offset, m_header.data(), m_header.size() );
}

const MemberReference* ReadOnlyObject::GetMemberReference( NameHash name ) const
{
	const MemberReference* const member = (MemberReference*)std::bsearch(
		&name,
		m_header.data(), m_header.size(), sizeof( m_header[ 0 ] ),
		[]( const void* lhs, const void* rhs )
		{
			const NameHash lhs_name = ( (MemberReference*)lhs )->name;
			const NameHash rhs_name = ( (MemberReference*)rhs )->name;
			return lhs_name < rhs_name ? -1 : lhs_name > rhs_name ? 1 : 0;
		}
	);

	return member && member->name == name ? member : nullptr;
}

u32 ReadOnlyObject::GetLiteral( NameHash name, void* dest, u32 dest_size ) const
{
	const MemberReference* const member = GetMemberReference( name );
	if ( !member )
		return 0;

	u32 offset = m_baseAddress + member->offset;
	const u32 member_size = m_reader.Read< u32 >( offset );

	if ( dest )
		m_reader.Read( offset, reinterpret_cast< byte* >( dest ), std::min( member_size, dest_size ) );

	return member_size;
}

std::shared_ptr< const IReadOnlyObject > ReadOnlyObject::GetChild( NameHash name ) const
{
	const MemberReference* const member = GetMemberReference( name );
	if ( !member )
		return nullptr;

	return std::make_shared< ReadOnlyObject >( m_reader, m_baseAddress + member->offset );
}

std::shared_ptr< const IReadOnlyObjectArray > ReadOnlyObject::GetArray( NameHash name ) const
{
	const MemberReference* const member = GetMemberReference( name );
	if ( !member )
		return nullptr;

	return std::make_shared< ReadOnlyObjectArray >( m_reader, m_baseAddress + member->offset );
}

ReadOnlyObjectArray::ReadOnlyObjectArray( Reader& reader, u32 offset )
	: m_reader( reader )
	, m_baseAddress( offset )
{
	m_header.resize( m_reader.Read< u32 >( offset ) );
	m_reader.Read( offset, m_header.data(), m_header.size() );
}

std::shared_ptr< const IReadOnlyObject > ReadOnlyObjectArray::GetChild( u32 index ) const
{
	return index >= Count() ? nullptr : std::make_shared< ReadOnlyObject >( m_reader, m_baseAddress + m_header[ index ] );
}

IReadWriteObject& ReadWriteObject::AddChild( NameHash name )
{
	return AddNamedChild< ReadWriteObject >( name );
}

IReadWriteObject* ReadWriteObject::GetChild( NameHash name ) const
{
	return GetNamedChild< IReadWriteObject >( name );
}

IReadWriteObjectArray& ReadWriteObject::AddArray( NameHash name )
{
	return AddNamedChild< ReadWriteObjectArray >( name );
}

IReadWriteObjectArray* ReadWriteObject::GetArray( NameHash name ) const
{
	return GetNamedChild< IReadWriteObjectArray >( name );
}

void ReadWriteBlob::Write( Writer& writer, u32& offset ) const
{
	writer.Write< u32 >( offset, data.size() );
	writer.Write( offset, data.data(), data.size() );
}

void ReadWriteObject::Write( Writer& writer, u32& offset ) const
{
	const u32 base_address = offset;
	writer.Write< u32 >( offset, m_children.size() );
	u32 member_table_address = offset;

	std::vector< MemberReference > member_table;
	member_table.reserve( m_children.size() );
	offset += sizeof( member_table.front() ) * m_children.size() + 1;

	for ( const auto& child : m_children )
	{
		member_table.push_back( { child.first, offset - base_address } );
		child.second->Write( writer, offset );
		offset += 1;
	}

	writer.Write( member_table_address, member_table.data(), member_table.size() );
}

void ReadWriteObjectArray::Write( Writer& writer, u32& offset ) const
{
	const u32 base_address = offset;
	writer.Write< u32 >( offset, m_children.size() );
	u32 member_table_address = offset;

	std::vector< u32 > member_table;
	member_table.reserve( m_children.size() );
	offset += sizeof( member_table.front() ) * m_children.size() + 1;

	for ( const auto& child : m_children )
	{
		member_table.push_back( offset - base_address );
		child->Write( writer, offset );
		offset += 1;
	}

	writer.Write( member_table_address, member_table.data(), member_table.size() );
}

}
