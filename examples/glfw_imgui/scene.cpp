#include "scene.h"
#include "gl_api.h"
#include "triangle.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

struct SceneImpl {
  Triangle m_triangle;

public:
  SceneImpl() {}

  void Render(int width, int height, const float clear_color[4]) {
    glViewport(0, 0, width, height);
    glClearColor(clear_color[0] * clear_color[3],
                 clear_color[1] * clear_color[3],
                 clear_color[2] * clear_color[3], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    m_triangle.Render();
  }
};

Scene::Scene() : m_impl(new SceneImpl) { assert(glGetError() == GL_NO_ERROR); }

Scene::~Scene() { delete m_impl; }

void Scene::Render(int width, int height, const float clear_color[4]) {

  m_impl->Render(width, height, clear_color);
}
