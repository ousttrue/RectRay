#include "gl_api.h"
#include "platform.h"
#include "renderer.h"
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

struct CameraState {
  std::string Name;
  rectray::Camera Camera;
  rectray::Interface Interface;
  float ClearColor[4];
  Renderer Renderer;
  void Show() {
    ImGui::Begin(Name.c_str());
    {

      ImGui::ColorEdit3("clear color", ClearColor);

      // camera
      ImGui::Separator();
      ImGui::TextUnformatted("[camera]");
      ImGui::DragFloat("camera near", &Camera.Projection.NearZ);
      Camera.Projection.NearZ = std::max(
          0.01f, std::min(Camera.Projection.NearZ, Camera.Projection.FarZ - 1));
      ImGui::DragFloat("camera far", &Camera.Projection.FarZ);
      ImGui::InputFloat3("camera pos", &Camera.Transform.Translation.x);
      Camera.Projection.FarZ =
          std::max(Camera.Projection.FarZ, Camera.Projection.NearZ + 1);

      if (auto ray = Interface.m_context.Ray) {
        ImGui::BeginDisabled(false);
        ImGui::InputFloat3("ray dir", &ray->Direction.x);
      } else {
        float zero[3]{0, 0, 0};
        ImGui::InputFloat3("ray dir", zero);
        ImGui::BeginDisabled(true);
      }

      ImGui::EndDisabled();
    }
    ImGui::End();
  }
  // ImGui::GetWindowDrawList()
  //&mainCamera.Screen
  void Render(const rectray::ScreenState &state, ImDrawList *imDrawList,
              Scene *scene, const rectray::Interface *other) {
    Renderer.Render(Interface, Camera, state, imDrawList, scene, other);
  }
};

// Main code
int main(int, char **) {
  Platform platform;

  if (!platform.CreateWindow()) {
    return 1;
  }
  auto &io = ImGui::GetIO();

  Scene scene;
  {
    scene.Objects.push_back(std::make_shared<Object>());
    scene.Objects.back()->Transform.Translation = {};

    scene.Objects.push_back(std::make_shared<Object>());
    scene.Objects.back()->Transform.Translation = {2, 0, 0};

    scene.Objects.push_back(std::make_shared<Object>());
    scene.Objects.back()->Transform.Translation = {0, 2, 0};
  }

  CameraState mainCamera{
      .Name = "main camera",
      .Camera{
          .Projection{
              .NearZ = 1,
              .FarZ = 100,
          },
          .Transform{
              .Translation{0, 1, 10},
          },
          .GazeDistance = 10,
      },
      .ClearColor{0.1f, 0.2f, 0.1f, 1.0f},
  };
  CameraState debugCamera{
      .Name = "debug camera",
      .Camera{
          .Projection{
              .NearZ = 1,
              .FarZ = 100,
          },
          .Transform{
              .Translation{0, 1, 20},
          },
          .GazeDistance = 20,
      },
      .ClearColor{0.3f, 0.3f, 0.3f, 1.0f},
  };
  auto renderTarget = std::make_shared<gl::RenderTarget>();

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

    static ImVec2 lastMouse = io.MousePos;

    mainCamera.Show();
    debugCamera.Show();

    if (ImGui::Begin("scene")) {
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      // scene
      ImGui::Separator();
      ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
      if (ImGui::TreeNode("[objects]")) {
        int clicked = 0;
        for (int i = 0; i < scene.Objects.size(); ++i) {
          auto &o = scene.Objects[i];
          char buf[256];
          snprintf(buf, std::size(buf), "%d", i);
          if (ImGui::Selectable(buf, o == scene.Selected)) {
            ++clicked;
            scene.Selected = o;
          }
        }
        if (ImGui::IsMouseClicked(0) && clicked == 0) {
          scene.Selected = {};
        }
        ImGui::TreePop();
      }
    }
    ImGui::End();

    //
    // render to renderTarget
    //
    if (ImGui::Begin("debug")) {
      auto pos = ImGui::GetCursorScreenPos();
      auto size = ImGui::GetContentRegionAvail();

      if (auto texture =
              renderTarget->Begin(size.x, size.y, debugCamera.ClearColor)) {
        ImGui::ImageButton((void *)(int64_t)texture, size, {0, 1}, {1, 0}, 0,
                           {1, 1, 1, 1}, {1, 1, 1, 1});
        ImGui::ButtonBehavior(ImGui::GetCurrentContext()->LastItemData.Rect,
                              ImGui::GetCurrentContext()->LastItemData.ID,
                              nullptr, nullptr,
                              ImGuiButtonFlags_MouseButtonMiddle |
                                  ImGuiButtonFlags_MouseButtonRight);

        auto focus = rectray::ScreenFocus::None;
        if (ImGui::IsItemActive()) {
          focus = rectray::ScreenFocus::Active;
        } else if (ImGui::IsItemHovered()) {
          focus = rectray::ScreenFocus::Hover;
        }

        rectray::ScreenState state{
            .Focus = focus,
            .ViewportX = pos.x,
            .ViewportY = pos.y,
            .ViewportWidth = size.x,
            .ViewportHeight = size.y,
            .MouseX = io.MousePos.x,
            .MouseY = io.MousePos.y,
            .MouseDeltaX = io.MouseDelta.x,
            .MouseDeltaY = io.MouseDelta.y,
            .MouseLeftDown = io.MouseDown[0],
            .MouseRightDown = io.MouseDown[1],
            .MouseMiddleDown = io.MouseDown[2],
            .MouseWheel = io.MouseWheel,
        };

        debugCamera.Render(state, ImGui::GetWindowDrawList(), &scene,
                           &mainCamera.Interface);

        renderTarget->End();
      }
    }
    ImGui::End();

    //
    // render to background
    //
    {
      auto focus = rectray::ScreenFocus::None;
      if (!io.WantCaptureMouse) {
        focus = rectray::ScreenFocus::Active;
      }

      rectray::ScreenState state{
          .Focus = focus,
          .ViewportX = 0,
          .ViewportY = 0,
          .ViewportWidth = io.DisplaySize.x,
          .ViewportHeight = io.DisplaySize.y,
          .MouseX = io.MousePos.x,
          .MouseY = io.MousePos.y,
          .MouseDeltaX = io.MouseDelta.x,
          .MouseDeltaY = io.MouseDelta.y,
          .MouseLeftDown = io.MouseDown[0],
          .MouseRightDown = io.MouseDown[1],
          .MouseMiddleDown = io.MouseDown[2],
          .MouseWheel = io.MouseWheel,
      };

      glEnable(GL_DEPTH_TEST);
      // glDepthFunc(GL_ALWAYS);
      glDepthFunc(GL_LEQUAL);
      glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
      glClearColor(mainCamera.ClearColor[0] * mainCamera.ClearColor[3],
                   mainCamera.ClearColor[1] * mainCamera.ClearColor[3],
                   mainCamera.ClearColor[2] * mainCamera.ClearColor[3],
                   mainCamera.ClearColor[3]);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      mainCamera.Render(state, ImGui::GetBackgroundDrawList(), &scene,
                        &debugCamera.Interface);
    }

    lastMouse = io.MousePos;
    platform.EndFrame();
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  return 0;
}
