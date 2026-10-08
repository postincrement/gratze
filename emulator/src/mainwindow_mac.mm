#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

#include <SDL.h>
#include <SDL_syswm.h>

// macOS 14 and later ignore SDL_RaiseWindow for a process started from a
// terminal. Mark this process as a foreground application and order its
// window in front.
extern "C" bool MacUsableDisplaySize(int * width, int * height)
{
  if ((width == nullptr) || (height == nullptr))
    return false;

  NSScreen * screen = [NSScreen mainScreen];
  if (screen == nil)
    return false;

  NSRect visible = [screen visibleFrame];
  *width = (int)visible.size.width;
  *height = (int)visible.size.height;
  return (*width > 0) && (*height > 0);
}

void RaiseMacWindow(SDL_Window * window)
{
  NSApplication * app = [NSApplication sharedApplication];
  [app setActivationPolicy:NSApplicationActivationPolicyRegular];

  ProcessSerialNumber psn = { 0, kCurrentProcess };
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  TransformProcessType(&psn, kProcessTransformToForegroundApplication);
  SetFrontProcessWithOptions(&psn, kSetFrontProcessCausedByUser);
#pragma clang diagnostic pop

  if (@available(macOS 14.0, *))
    [app activate];
  else
    [app activateIgnoringOtherApps:YES];

  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  if (SDL_GetWindowWMInfo(window, &info) && (info.subsystem == SDL_SYSWM_COCOA))
    [info.info.cocoa.window makeKeyAndOrderFront:nil];
}
