#include "scene.h"
#include "gl_api.h"
#include "plane.h"
#include "triangle.h"
#include <assert.h>
#include <rectray.h>

struct SceneImpl {
  rectray::Camera m_camera;
  Triangle m_triangle;
  Plane m_plane;

public:
  SceneImpl() {
    m_camera.Transform.Translation = {0, 0, 2};
    m_camera.Projection.NearZ = 0.01f;
    m_camera.Projection.FarZ = 10.0f;
  }

  void Render(float width, float height, const float clear_color[4],
              const rectray::MouseState &mouse) {
    glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_ALWAYS);
    glDepthFunc(GL_LEQUAL);
    glViewport(0, 0, (int)width, (int)height);
    glClearColor(clear_color[0] * clear_color[3],
                 clear_color[1] * clear_color[3],
                 clear_color[2] * clear_color[3], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_camera.Projection.SetSize(width, height);
    m_camera.MouseInputTurntable(mouse);
    m_camera.Update();

    m_triangle.Render(m_camera);
    m_plane.Render(m_camera);
  }
};

Scene::Scene() : m_impl(new SceneImpl) { assert(glGetError() == GL_NO_ERROR); }

Scene::~Scene() { delete m_impl; }

void Scene::Render(float width, float height, const float clear_color[4],
                   const rectray::MouseState &mouse) {

  m_impl->Render(width, height, clear_color, mouse);
}
