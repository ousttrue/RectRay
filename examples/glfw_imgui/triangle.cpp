#include "triangle.h"
#include "gl_api.h"
#include <assert.h>
#include <rectray.h>

struct TriangleImpl {
  std::shared_ptr<gl::ShaderProgram> m_shader;
  std::shared_ptr<gl::Vao> m_vao;

  GLuint m_mvp = -1;
  GLuint m_vpos = -1;
  GLuint m_vcol = -1;

  TriangleImpl() {
    static const char *VS = R"(
uniform mat4 MVP;
in vec3 vCol;
in vec2 vPos;
out vec3 color;
void main()
{
    gl_Position = MVP * vec4(vPos, 0.0, 1.0);
    color = vCol;
}
)";

    static const char *FS = R"(
in vec3 color;
out vec4 FragColor;
void main()
{
    FragColor = vec4(color, 1.0);
}
)";

    m_shader = gl::ShaderProgram::FromSource(VS, FS);
    if (auto location = m_shader->UniformLocation("MVP")) {
      m_mvp = *location;
    }
    if (auto location = m_shader->AttributeLocation("vPos")) {
      m_vpos = *location;
    }
    if (auto location = m_shader->AttributeLocation("vCol")) {
      m_vcol = *location;
    }
    assert(glGetError() == GL_NO_ERROR);

    static const struct {
      float x, y;
      float r, g, b;
    } vertices[3] = {{-0.6f, -0.4f, 1.f, 0.f, 0.f},
                     {0.6f, -0.4f, 0.f, 1.f, 0.f},
                     {0.f, 0.6f, 0.f, 0.f, 1.f}};
    auto vbo = gl::Vbo::Create(vertices);
    assert(glGetError() == GL_NO_ERROR);

    static gl::Attribute attributes[] = {
        {m_vpos, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), 0},
        {m_vcol, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), 8},
    };
    m_vao = gl::Vao::Create(vbo, attributes);
    assert(glGetError() == GL_NO_ERROR);
  }

  void Render(const DirectX::XMFLOAT4X4 &viewProjection) {
    m_shader->Use();
    m_shader->SetMat4(m_mvp, &viewProjection._11);
    m_vao->Draw(GL_TRIANGLES, 3);
  }
};

Triangle::Triangle() : m_impl(new TriangleImpl) {}

Triangle::~Triangle() { delete m_impl; }

void Triangle::Render(const rectray::Camera &camera) {
  m_impl->Render(camera.ViewProjection());
}
