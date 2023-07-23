#include "platform.h"
#include "scene.h"
#include <imgui.h>
#include <rectray.h>

// This example can also compile and run with Emscripten! See
// 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "examples/libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Main code
int main(int, char **) {
  Platform platform;

  if (!platform.CreateWindow()) {
    return 1;
  }

  Scene scene;
  auto &io = ImGui::GetIO();

  // Main loop
#ifdef __EMSCRIPTEN__
  // For an Emscripten build we are disabling file-system access, so let's not
  // attempt to do a fopen() of the imgui.ini file. You may manually call
  // LoadIniSettingsFromMemory() to load settings from your own storage.
  io.IniFilename = NULL;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (platform.BeginFrame())
#endif
  {
    platform.UpdateGui();

    rectray::MouseState mouse{
        .X = io.MousePos.x,
        .Y = io.MousePos.y,
    };
    if (!io.WantCaptureMouse) {
      mouse.DeltaX = io.MouseDelta.x;
      mouse.DeltaY = io.MouseDelta.y;
      mouse.LeftDown = io.MouseDown[0];
      mouse.RightDown = io.MouseDown[1];
      mouse.MiddleDown = io.MouseDown[2];
      mouse.Wheel = io.MouseWheel;
    }
    scene.Render(io.DisplaySize.x, io.DisplaySize.y, platform.clear_color,
                 mouse);

    platform.EndFrame();
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  return 0;
}
