#import <Cocoa/Cocoa.h>
#include <SDL3/SDL.h>
#include "macos.h"

MacOSTitlebarInfo macos_setup_titlebar(SDL_Window *sdl_window) {
    MacOSTitlebarInfo info = { 28.0f, 80.0f };

    SDL_PropertiesID props = SDL_GetWindowProperties(sdl_window);
    NSWindow *win = (__bridge NSWindow *)SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    if (!win) return info;

    // Measure titlebar height before setting titlebarAppearsTransparent, because
    // that property makes contentLayoutRect report the full frame height instead.
    CGFloat total   = win.frame.size.height;
    CGFloat content = win.contentLayoutRect.size.height;
    if (content > 0.0 && total > content)
        info.titlebar_height = (float)(total - content);

    win.styleMask |= NSWindowStyleMaskFullSizeContentView;
    win.titlebarAppearsTransparent = YES;
    win.titleVisibility = NSWindowTitleHidden;

    NSButton *zoom = [win standardWindowButton:NSWindowZoomButton];
    if (zoom) {
        NSRect r = zoom.frame;
        info.traffic_light_width = (float)(NSMaxX(r) + 8.0);
    }

    return info;
}
