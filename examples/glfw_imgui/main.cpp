#include "gl_api.h"
#include "platform.h"
#include "scene.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
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

  auto main = std::make_shared<gl::RenderTarget>();

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

    if (ImGui::Begin("main")) {
      auto pos = ImGui::GetCursorScreenPos();
      auto size = ImGui::GetContentRegionAvail();

      // image button. capture mouse event
      if (auto texture = main->Begin(size.x, size.y, platform.clear_color)) {
        ImGui::ImageButton((void *)texture, size, {0, 1}, {1, 0}, 0,
                           {1, 1, 1, 1}, {1, 1, 1, 1});
        ImGui::ButtonBehavior(ImGui::GetCurrentContext()->LastItemData.Rect,
                              ImGui::GetCurrentContext()->LastItemData.ID,
                              nullptr, nullptr,
                              ImGuiButtonFlags_MouseButtonMiddle |
                                  ImGuiButtonFlags_MouseButtonRight);
        auto active = ImGui::IsItemActive();
        auto hover = ImGui::IsItemHovered();

        main->End();
      }
    }
    ImGui::End();

    if (ImGui::Begin("debug")) {
      auto pos = ImGui::GetCursorScreenPos();
      auto size = ImGui::GetContentRegionAvail();
      ImGui::TextUnformatted("debug");
    }
    ImGui::End();

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
