#include "scene.h"
#include "gl_api.h"
#include "triangle.h"
#include <assert.h>
#include <rectray.h>

struct SceneImpl {
  rectray::Camera m_camera;
  Triangle m_triangle;

public:
  SceneImpl() { m_camera.Transform.Translation = {0, 0, 2}; }

  void Render(int width, int height, const float clear_color[4],
              const rectray::MouseState &mouse) {
    glViewport(0, 0, width, height);
    glClearColor(clear_color[0] * clear_color[3],
                 clear_color[1] * clear_color[3],
                 clear_color[2] * clear_color[3], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    m_camera.Projection.SetSize((float)width, (float)height);
    m_camera.MouseInputTurntable(mouse);
    m_camera.Update();

    m_triangle.Render(m_camera);
  }
};

Scene::Scene() : m_impl(new SceneImpl) { assert(glGetError() == GL_NO_ERROR); }

Scene::~Scene() { delete m_impl; }

void Scene::Render(int width, int height, const float clear_color[4],
                   const rectray::MouseState &mouse) {

  m_impl->Render(width, height, clear_color, mouse);
}
