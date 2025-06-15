#include "WindowManager.h"

#include "Windows.h"

namespace onyx
{

std::string IWindowManager::DoOpenFileDialog() const
{
	char buffer[ 512 ] = "";

	OPENFILENAMEA ofn {};
	ofn.lStructSize = sizeof( ofn );
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = _countof( buffer );
	ofn.Flags = OFN_FILEMUSTEXIST;

	if ( !GetOpenFileNameA( &ofn ) )
		return "";

	return buffer;
}

std::string IWindowManager::DoSaveFileDialog() const
{
	char buffer[ 512 ] = "";

	OPENFILENAMEA ofn {};
	ofn.lStructSize = sizeof( ofn );
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = _countof( buffer );

	if ( !GetSaveFileNameA( &ofn ) )
		return "";

	return buffer;
}

}
