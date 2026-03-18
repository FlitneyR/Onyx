#include "WindowManager.h"

#ifdef WIN32
#include "Windows.h"
#endif

namespace onyx
{

std::string IWindowManager::DoOpenFileDialog() const
{
	#ifdef WIN32
	char buffer[ 512 ] = "";

	OPENFILENAMEA ofn {};
	ofn.lStructSize = sizeof( ofn );
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = COUNTOF( buffer );
	ofn.Flags = OFN_FILEMUSTEXIST;

	if ( !GetOpenFileNameA( &ofn ) )
		return "";

	return buffer;

	#else	
	STRONG_ASSERT(false, "Not implemented!");
	return "";
	#endif
}

std::string IWindowManager::DoSaveFileDialog() const
{
	#ifdef WIN32
	char buffer[ 512 ] = "";

	OPENFILENAMEA ofn {};
	ofn.lStructSize = sizeof( ofn );
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = COUNTOF( buffer );

	if ( !GetSaveFileNameA( &ofn ) )
		return "";

	return buffer;

	#else
	STRONG_ASSERT(false, "Not implemented!");
	return "";
	#endif
}

}
