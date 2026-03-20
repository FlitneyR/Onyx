#include "CocoaWrappers.h"
#include "AppKit/NSOpenPanel.h"
#include "AppKit/NSSavePanel.h"

const char* Cocoa_DoOpenFileDialog()
{
    NSOpenPanel* const panel = [NSOpenPanel openPanel];

    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseDirectories: YES];

    if ([panel runModal] == NSModalResponseOK) {
        NSURL* const url = [[panel URLs] objectAtIndex:0];
        NSString* const path_nsstring = [url absoluteString];
        char* out_filename = (char*)malloc( [path_nsstring length] );
        strcpy( out_filename, [path_nsstring UTF8String] );
        return out_filename;
    }

    return NULL;
}

const char* Cocoa_DoSaveFileDialog()
{
    NSSavePanel* const panel = [NSSavePanel savePanel];

    if ( [panel runModal] == NSModalResponseOK ) {
        NSURL* const url = [panel URL];
        NSString* const path_nsstring = [url absoluteString];
        char* out_filename = (char*)malloc( [path_nsstring length] );
        strcpy( out_filename, [path_nsstring UTF8String] );
        return out_filename;
    }

    return NULL;
}
