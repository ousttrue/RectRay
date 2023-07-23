#pragma once
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <imgui_impl_opengl3.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>
#define glBindVertexArray glBindVertexArrayOES
#define glGenVertexArrays glGenVertexArraysOES
#define glDeleteVertexArrays glDeleteVertexArraysOES
inline const char *SHADER_HEADER = "#version 300 es\nprecision highp float;\n";

#else
#include <GL/glew.h>
inline const char *SHADER_HEADER = "#version 150\n";
#endif

namespace gl {

inline std::string GetCompileError(GLuint shader) {
  GLint compileSuccess = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);
  if (compileSuccess != GL_FALSE) {
    return {};
  }

  GLint infoLen;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
  if (infoLen > 1) {
    std::string buffer;
    buffer.resize(infoLen);
    glGetShaderInfoLog(shader, infoLen, nullptr, (GLchar *)buffer.data());
    return buffer;
  } else {
    return "unknown error";
  }
}

inline std::string GetLinkError(GLuint program) {
  GLint linkSuccess = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
  if (linkSuccess != GL_FALSE) {
    return {};
  }

  GLint infoLen;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
  if (infoLen > 1) {
    std::string buffer;
    buffer.resize(infoLen);
    glGetProgramInfoLog(program, infoLen, nullptr, (GLchar *)buffer.data());
    return buffer;
  } else {
    return "unknown error";
  }
}

class ShaderSource {
  GLuint m_shader;

public:
  ShaderSource(GLuint type) { m_shader = glCreateShader(type); }
  ~ShaderSource() { glDeleteShader(m_shader); }
  GLuint Handle() const { return m_shader; }

  std::expected<std::monostate, std::string>
  Compile(std::span<const char *> snippets) {
    glShaderSource(m_shader, snippets.size(), snippets.data(), nullptr);
    glCompileShader(m_shader);
    auto error = GetCompileError(m_shader);
    if (!error.empty()) {
      return std::unexpected{error};
    }
    return std::monostate{};
  }

  static std::shared_ptr<ShaderSource> CreateShader(GLuint type,
                                                    const char *source) {
    auto ptr = std::make_shared<ShaderSource>(type);

    const char *snippets[] = {
        SHADER_HEADER,
        source,
    };
    auto success = ptr->Compile(snippets);
    if (!success) {
      printf("%s", success.error().c_str());
      return {};
    }

    return ptr;
  }
};

class ShaderProgram {
  GLuint m_program;

public:
  ShaderProgram() { m_program = glCreateProgram(); }
  ~ShaderProgram() { glDeleteProgram(m_program); }

  std::expected<std::monostate, std::string> Link(GLuint vs, GLuint fs) {
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);
    auto error = GetLinkError(m_program);
    if (!error.empty()) {
      return std::unexpected{error};
    }
    return std::monostate{};
  }

  static std::shared_ptr<ShaderProgram> FromSource(const char *vsSource,
                                                   const char *fsSource) {

    auto vs = ShaderSource::CreateShader(GL_VERTEX_SHADER, vsSource);
    if (!vs) {
      return nullptr;
    }

    auto fs = ShaderSource::CreateShader(GL_FRAGMENT_SHADER, fsSource);
    if (!fs) {
      return nullptr;
    }

    auto ptr = std::make_shared<ShaderProgram>();
    auto success = ptr->Link(vs->Handle(), fs->Handle());
    if (!success) {
      printf("link: %s", success.error().c_str());
      return nullptr;
    }

    return ptr;
  }

  void Use() { glUseProgram(m_program); }

  std::optional<GLuint> UniformLocation(const char *name) {
    auto location = glGetUniformLocation(m_program, name);
    if (location < 0) {
      return std::nullopt;
    }
    return (GLuint)location;
  }

  void SetMat4(GLuint location, const float *m) {
    glUniformMatrix4fv(location, 1, GL_FALSE, m);
  }

  std::optional<GLuint> AttributeLocation(const char *name) {
    auto location = glGetAttribLocation(m_program, name);
    if (location < 0) {
      return std::nullopt;
    }
    return (GLuint)location;
  }

  std::optional<uint32_t> UboBlockIndex(const char *name) {
    auto blockIndex = glGetUniformBlockIndex(m_program, name);
    if (blockIndex < 0) {
      return std::nullopt;
    }
    return blockIndex;
  }

  void UboBind(uint32_t blockIndex, uint32_t binding_point) {
    glUniformBlockBinding(m_program, blockIndex, binding_point);
  }
};

class Vbo {
  GLuint m_vbo;

public:
  Vbo() { glGenBuffers(1, &m_vbo); }
  ~Vbo() { glDeleteBuffers(1, &m_vbo); }

  template <typename T, size_t N>
  static std::shared_ptr<Vbo> Create(const T (&vertices)[N]) {
    auto ptr = std::make_shared<Vbo>();
    ptr->Upload((const char *)vertices, sizeof(T) * N);
    return ptr;
  }

  void Bind() { glBindBuffer(GL_ARRAY_BUFFER, m_vbo); }

  void Unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

  void Upload(const char *p, uint32_t size) {
    Bind();
    glBufferData(GL_ARRAY_BUFFER, size, p, GL_STATIC_DRAW);
    Unbind();
  }

  void Draw(GLuint drawcount, GLuint offset = 0) {
    Bind();
    glDrawArrays(GL_TRIANGLES, offset, drawcount);
    Unbind();
  }
};

class Ibo {
  uint32_t ibo_ = 0;

public:
  uint32_t valuetype_ = 0;
  Ibo(uint32_t ibo, uint32_t valuetype) : ibo_(ibo), valuetype_(valuetype) {}
  ~Ibo() { glDeleteBuffers(1, &ibo_); }
  static std::shared_ptr<Ibo> Create(uint32_t size, const void *data,
                                     uint32_t valuetype) {
    GLuint ibo;
    glGenBuffers(1, &ibo);
    auto ptr = std::shared_ptr<Ibo>(new Ibo(ibo, valuetype));
    ptr->Bind();
    if (data) {
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    } else {
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }
    ptr->Unbind();
    return ptr;
  }
  template <typename T>
  static std::shared_ptr<Ibo> Create(const std::vector<T> &values) {
    switch (sizeof(T)) {
    case 1:
      return Create(values.size(), values.data(), GL_UNSIGNED_BYTE);

    case 2:
      return Create(values.size() * 2, values.data(), GL_UNSIGNED_SHORT);

    case 4:
      return Create(values.size() * 4, values.data(), GL_UNSIGNED_INT);
    }
    return {};
  }
  void Bind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_); }
  void Unbind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
};

struct Attribute {
  GLuint Location;
  GLuint ElementCount;
  GLenum ElementType;
  GLboolean Normalize;
  GLuint Stride;
  GLuint Offset;
};

class Vao {
  GLuint m_vao;
  std::shared_ptr<Vbo> m_vbo;
  std::shared_ptr<Ibo> m_ibo;

public:
  Vao() { glGenVertexArrays(1, &m_vao); }
  ~Vao() { glDeleteVertexArrays(1, &m_vao); }
  void Bind() { glBindVertexArray(m_vao); }
  void Unbind() { glBindVertexArray(0); }

  static std::shared_ptr<Vao>
  Create(const std::shared_ptr<Vbo> &vbo, std::span<const Attribute> attributes,
         const std::shared_ptr<Ibo> &ibo = nullptr) {
    auto ptr = std::make_shared<Vao>();

    ptr->Bind();
    if (ibo) {
      ibo->Bind();
    }

    for (auto &attribute : attributes) {
      glEnableVertexAttribArray(attribute.Location);
      vbo->Bind();
      glVertexAttribPointer(attribute.Location, attribute.ElementCount,
                            attribute.ElementType, attribute.Normalize,
                            attribute.Stride,
                            (void *)(int64_t)attribute.Offset);
    }
    ptr->Unbind();

    vbo->Unbind();
    ptr->m_vbo = vbo;

    if (ibo) {
      ibo->Unbind();
      ptr->m_ibo = ibo;
    }

    return ptr;
  }

  void Draw(GLenum mode, GLuint count, GLuint offset = 0) {
    Bind();
    if (m_ibo) {
      glDrawElements(mode, count, m_ibo->valuetype_,
                     reinterpret_cast<void *>(static_cast<uint64_t>(offset)));
    } else {
      glDrawArrays(mode, offset, count);
    }
    Unbind();
  }
};

struct Ubo {
  uint32_t ubo_ = 0;

  static std::shared_ptr<Ubo> Create(uint32_t size, const void *data) {
    auto ptr = std::make_shared<Ubo>();

    glGenBuffers(1, &ptr->ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ptr->ubo_);
    if (data) {
      glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
    } else {
      glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return ptr;
  }
  template <typename T> static std::shared_ptr<Ubo> Create() {
    return Create(sizeof(T), nullptr);
  }
  template <typename T> static std::shared_ptr<Ubo> Create(const T &value) {
    return Create(sizeof(T), (const void *)&value);
  }

  void Bind() { glBindBuffer(GL_UNIFORM_BUFFER, ubo_); }
  void Unbind() { glBindBuffer(GL_UNIFORM_BUFFER, 0); }
  void Upload(uint32_t size, const void *data) {
    Bind();
    glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
    Unbind();
  }
  template <typename T> void Upload(const T &data) { Upload(sizeof(T), &data); }
  void SetBindingPoint(uint32_t binding_point) {
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, ubo_);
  }
};

} // namespace gl
