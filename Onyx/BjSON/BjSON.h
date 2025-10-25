#pragma once

//#if !defined( BjSON_ASSERT )
//#include <assert.h>
//#define BjSON_ASSERT(...) assert(__VA_ARGS__)
//#endif

#include "Onyx/Asserts.h"
#define BjSON_ASSERT( ... ) WEAK_ASSERT( __VA_ARGS__ )

#include <memory>
#include <fstream>
#include <vector>

/*

BjSON (pronounced Bison) is a structured binary file format reflecting the structure of JSON
An object in a BjSON file consists of a list of members, identified by name hashes
A member is either:
	1) A literal - a sized array of bytes
	2) A child - another BjSON object
	3) An array - a sized array of children

BjSON makes no attempt to record the data types of anything in the file, it is up to the user to
instruct BjSON how to interpret the members data types, and to ensure that literals are interpreted correctly

*/

namespace BjSON
{

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned int	u32;
typedef unsigned char	byte;

typedef u32 NameHash;

constexpr NameHash HashName( const char* name )
{
	return !*name ? 5381 : ( *name + 33 * HashName( name + 1 ) );
}

struct IReadOnlyObject;
struct IReadOnlyObjectArray;
struct IReadWriteObject;
struct IReadWriteObjectArray;

struct IReadOnlyObject
{
	// copy the requested literal to the destination buffer
	// if the literal does not exist, returns 0, otherwise returns the size of the literal
	// will not write to dest if it is nullptr, and will not write beyond dest_size
	virtual u32 GetLiteral( NameHash name, void* dest = nullptr, u32 dest_size = 0 ) const = 0;
	virtual std::shared_ptr< const IReadOnlyObject > GetChild( NameHash name ) const = 0;
	virtual std::shared_ptr< const IReadOnlyObjectArray > GetArray( NameHash name ) const = 0;
	virtual u32 GetMemberCount() const = 0;
	virtual NameHash GetMemberName( u32 index ) const = 0;
	virtual bool HasMember( NameHash name ) const = 0;

	template< typename T >
	u32 GetLiteral( NameHash name, T* vals_out, u32 val_count )
	{
		return GetLiteral( name, (void*)vals_out, val_count * sizeof( T ) );
	}

	template< typename T >
	bool GetLiteral( NameHash name, T& val_out ) const
	{
		if ( !HasMember( name ) )
			return false;

		T temp;
		if ( !WEAK_ASSERT( GetLiteral( name, &temp, sizeof( T ) ) == sizeof( T ) ) )
			return false;
		
		val_out = temp;
		return true;
	}

	template< typename T >
	T GetLiteral( NameHash name ) const
	{
		T result {};
		BjSON_ASSERT( GetLiteral( name, result ) );
		return result;
	}

	template<>
	bool GetLiteral< std::string >( NameHash name, std::string& result ) const
	{
		if ( !HasMember( name ) ) return false;
		const u32 size = GetLiteral( name );
		result.clear();
		result.resize( size );
		GetLiteral( name, (char*)result.c_str(), size );
		result.resize( strlen( result.c_str() ) );
		return true;
	}

	template<>
	std::string GetLiteral< std::string >( NameHash name ) const
	{
		std::string result;
		GetLiteral( name, result );
		return result;
	}
};

struct IReadOnlyObjectArray
{
	virtual u32 Count() const = 0;
	virtual std::shared_ptr< const IReadOnlyObject > GetChild( u32 index ) const = 0;
};

struct IReadWriteObject
{
	template< typename T >
	IReadWriteObject& SetLiteral( NameHash name, const T& literal )
	{
		SetLiteral( name, &literal, sizeof( T ) );
		return *this;
	}

	template<>
	IReadWriteObject& SetLiteral< std::string >( NameHash name, const std::string& literal )
	{
		SetLiteral( name, literal.c_str(), (u32)literal.size() );
		return *this;
	}

	template< typename T >
	T* GetLiteral( NameHash name, u32* out_count = nullptr ) const
	{
		u32 size = 0;
		void* ptr = GetLiteral( name );
		
		BjSON_ASSERT( size % sizeof( name ) == 0 );

		if ( out_count )
			*out_count = size / sizeof( name );

		return ptr;
	}

	virtual IReadWriteObject& SetLiteral( NameHash name, const void* data, u32 size ) = 0;
	virtual const void* GetLiteral( NameHash name, u32* out_size = nullptr ) const = 0;

	virtual IReadWriteObject& AddChild( NameHash name ) = 0;
	virtual IReadWriteObject* GetChild( NameHash name ) const = 0;

	virtual IReadWriteObjectArray& AddArray( NameHash name ) = 0;
	virtual IReadWriteObjectArray* GetArray( NameHash name ) const = 0;
};

struct IReadWriteObjectArray
{
	virtual u32 Count() const = 0;
	virtual IReadWriteObject& AddChild() = 0;
	virtual IReadWriteObject* GetChild( u32 index ) const = 0;
};

struct IDecoder
{
	virtual const IReadOnlyObject& GetRootObject() const = 0;
};

struct IEncoder
{
	virtual IReadWriteObject& GetRootObject() = 0;
	virtual void WriteTo( std::vector< byte >& dest ) const = 0;
	virtual void WriteTo( std::ofstream& file ) const = 0;
};

struct Decoder : IDecoder
{
	Decoder( const void* data, u32 size );
	Decoder( const std::vector< byte >& bytes ) : Decoder( bytes.data(), (u32)bytes.size() ) {}
	Decoder( std::ifstream& file );

	const IReadOnlyObject& GetRootObject() const override { return impl->GetRootObject(); }

private:
	std::unique_ptr< IDecoder > impl;
};

struct Encoder : IEncoder
{
	Encoder();

	IReadWriteObject& GetRootObject() override { return impl->GetRootObject(); }
	void WriteTo( std::vector< byte >& dest ) const override { impl->WriteTo( dest ); }
	void WriteTo( std::ofstream& file ) const override { impl->WriteTo( file ); }

private:
	std::unique_ptr< IEncoder > impl;
};

}

constexpr BjSON::NameHash operator ""_name( const char* name, size_t len ) { return BjSON::HashName( name ); }
