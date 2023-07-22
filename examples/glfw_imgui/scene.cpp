#include "scene.h"
#include <stdio.h>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <imgui_impl_opengl3.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
static const char *vertex_shader_text = R"(#version 300 es
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

static const char *fragment_shader_text = R"(#version 300 es
precision highp float;
in vec3 color;
out vec4 FragColor;
void main()
{
    FragColor = vec4(color, 1.0);
}
)";

#else
#include <GL/glew.h>
static const char *vertex_shader_text =
    R"(#version 110
      uniform mat4 MVP;
      attribute vec3 vCol;
      attribute vec2 vPos;
      varying vec3 color;
      void main()
      {
          gl_Position = MVP * vec4(vPos, 0.0, 1.0);
          color = vCol;
      }
      )";

static const char *fragment_shader_text =
    R"(#version 110
      varying vec3 color;
      void main()
      {
          gl_FragColor = vec4(color, 1.0);
      }
      )";
#endif

void print_compile_error(const char *prefix, GLuint shader) {
  GLint compileSuccess = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);
  if (compileSuccess == GL_FALSE) {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
      std::vector<char> buffer(infoLen);
      glGetShaderInfoLog(shader, infoLen, NULL, buffer.data());
      printf("%s: %s", prefix, buffer.data());
    }
  }
}

void print_link_error(GLuint program) {
  GLint linkSuccess = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);

  if (linkSuccess == GL_FALSE) {
    GLint infoLen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
      std::vector<char> buffer(infoLen);
      glGetProgramInfoLog(program, infoLen, NULL, buffer.data());
      printf("%s", buffer.data());
    }
  }
}

struct SceneImpl {
  GLuint vertex_buffer, vertex_shader, fragment_shader, program;
  GLint mvp_location, vpos_location, vcol_location;

public:
  SceneImpl() {
    static const struct {
      float x, y;
      float r, g, b;
    } vertices[3] = {{-0.6f, -0.4f, 1.f, 0.f, 0.f},
                     {0.6f, -0.4f, 0.f, 1.f, 0.f},
                     {0.f, 0.6f, 0.f, 0.f, 1.f}};

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    print_compile_error("vs", vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    print_compile_error("fs", fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    print_link_error(program);

    mvp_location = glGetUniformLocation(program, "MVP");
    vpos_location = glGetAttribLocation(program, "vPos");
    vcol_location = glGetAttribLocation(program, "vCol");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void *)0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void *)(sizeof(float) * 2));
  }

  void Render(int width, int height, const float clear_color[4]) {
    glViewport(0, 0, width, height);
    glClearColor(clear_color[0] * clear_color[3],
                 clear_color[1] * clear_color[3],
                 clear_color[2] * clear_color[3], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    {
      float mat4[16] = {
          1, 0, 0, 0, //
          0, 1, 0, 0, //
          0, 0, 1, 0, //
          0, 0, 0, 1, //
      };
      glUseProgram(program);
      glUniformMatrix4fv(mvp_location, 1, GL_FALSE, mat4);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }
};

Scene::Scene() : m_impl(new SceneImpl) {}

Scene::~Scene() { delete m_impl; }

void Scene::Render(int width, int height, const float clear_color[4]) {

  m_impl->Render(width, height, clear_color);
}
