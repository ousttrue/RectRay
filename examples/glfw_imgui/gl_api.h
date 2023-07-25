#pragma once
#include <assert.h>
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#define GL_GLEXT_PROTOTYPES
#define GL_SILENCE_DEPRECATION
#include <GLES3/gl32.h>
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

enum class PixelFormat {
  u8_RGBA,
  u8_RGB,
  // grayscale
  u8_R,
  f16_RGB,
  f32_RGB,
};

enum class ColorSpace {
  Linear,
  sRGB,
};

inline std::optional<uint32_t> GLImageFormat(PixelFormat format,
                                             ColorSpace colorspace) {
  if (colorspace == ColorSpace::Linear) {
    switch (format) {
    case PixelFormat::f32_RGB:
      return GL_RGB32F;
    case PixelFormat::f16_RGB:
      return GL_RGB16F;
    case PixelFormat::u8_RGBA:
      return GL_RGBA;
    case PixelFormat::u8_RGB:
      return GL_RGB;
    case PixelFormat::u8_R:
      return GL_RED;
    default:
      break;
    }
  } else {
    switch (format) {
    case PixelFormat::u8_RGBA:
      return GL_SRGB8_ALPHA8;
    case PixelFormat::u8_RGB:
      return GL_SRGB8;
    default:
      break;
    }
  }

  assert(false);
  return std::nullopt;
}

inline uint32_t GLInternalFormat(PixelFormat format) {
  switch (format) {
  case PixelFormat::u8_RGBA:
    return GL_RGBA;

  case PixelFormat::u8_R:
    return GL_RED;

  default:
    break;
  }
  return GL_RGB;
}

struct Image {
  int Width;
  int Height;
  PixelFormat Format;
  ColorSpace ColorSpace = ColorSpace::Linear;
  const uint8_t *Pixels = nullptr;
};

class Texture {
  uint32_t m_handle;
  int m_width = 0;
  int m_height = 0;

public:
  Texture() { glGenTextures(1, &m_handle); }
  ~Texture() { glDeleteTextures(1, &m_handle); }
  void Bind() const { glBindTexture(GL_TEXTURE_2D, m_handle); }
  void Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }
  void Activate(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_handle);
  }
  static void Deactivate(uint32_t unit) { glDisable(GL_TEXTURE0 + unit); }
  const uint32_t &Handle() const { return m_handle; }
  int Width() const { return m_width; }
  int Height() const { return m_height; }

  static std::shared_ptr<Texture> Create(const Image &data,
                                         bool useFloat = false) {
    auto ptr = std::shared_ptr<Texture>(new Texture());
    ptr->Upload(data, useFloat);
    return ptr;
  }

  void Upload(const Image &data, bool useFloat) {
    SamplingLinear();
    WrapClamp();
    Bind();
    if (auto format = GLImageFormat(data.Format, data.ColorSpace)) {
      glTexImage2D(GL_TEXTURE_2D, 0, *format, data.Width, data.Height, 0,
                   GLInternalFormat(data.Format),
                   useFloat ? GL_FLOAT : GL_UNSIGNED_BYTE, data.Pixels);
      glGenerateMipmap(GL_TEXTURE_2D);
      // glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_width);
      // glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT,
      // &m_height);
      m_width = data.Width;
      m_height = data.Height;
    }
    Unbind();
  }

  void WrapClamp() {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    Unbind();
  }

  void WrapRepeat() {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    Unbind();
  }

  void SamplingPoint() {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    Unbind();
  }

  void SamplingLinear(bool mip = false) {
    Bind();
    if (mip) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    Unbind();
  }
};

struct ClearParam {
  bool Depth = true;
  bool ApplyAlpha = false;
};

inline void ClearViewport(int width, int height, const float clear_color[4],
                          std::optional<float> depth = {},
                          bool apply_alpha = false) {
  glViewport(0, 0, width, height);
  glScissor(0, 0, width, height);
  if (apply_alpha) {
    glClearColor(clear_color[0] * clear_color[3],
                 clear_color[1] * clear_color[3],
                 clear_color[2] * clear_color[3], clear_color[3]);
  } else {
    glClearColor(clear_color[0], clear_color[1], clear_color[2],
                 clear_color[3]);
  }
  if (depth) {
    glClearDepthf(*depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  } else {
    glClear(GL_COLOR_BUFFER_BIT);
  }
}

struct Fbo {
  uint32_t m_fbo = 0;
  uint32_t m_rbo = 0;
  Fbo() { glGenFramebuffers(1, &m_fbo); }
  ~Fbo() {
    glDeleteFramebuffers(1, &m_fbo);
    if (m_rbo) {
      glDeleteRenderbuffers(1, &m_rbo);
    }
  }
  Fbo(const Fbo &) = delete;
  Fbo &operator=(const Fbo &) = delete;

  void Bind() { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); }
  void Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

  void AttachDepth(int width, int height) {
    Bind();
    if (!m_rbo) {
      glGenRenderbuffers(1, &m_rbo);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void AttachTexture2D(uint32_t texture, int mipLevel = 0) {
    Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture, mipLevel);
    // uint32_t buffers[] = { GL_COLOR_ATTACHMENT0 };
    // glDrawBuffers(1, buffers);
  }

  void AttachCubeMap(int i, uint32_t texture, int mipLevel = 0) {
    Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, texture,
                           mipLevel);
  }
};

struct RenderTarget {
  std::shared_ptr<Fbo> Fbo;
  std::shared_ptr<Texture> FboTexture;

  uint32_t Begin(float width, float height, const float color[4]) {
    if (width == 0 || height == 0) {
      return 0;
    }
    if (!Fbo) {
      Fbo = std::make_shared<struct Fbo>();
    }

    if (FboTexture) {
      if (FboTexture->Width() != width || FboTexture->Height() != height) {
        FboTexture = nullptr;
      }
    }
    if (!FboTexture) {
      FboTexture = Texture::Create({
          static_cast<int>(width),
          static_cast<int>(height),
          PixelFormat::u8_RGB,
          ColorSpace::Linear,
      });
      Fbo->AttachTexture2D(FboTexture->Handle());
      Fbo->AttachDepth(static_cast<int>(width), static_cast<int>(height));
    }

    Fbo->Bind();
    ClearViewport((int)width, (int)height, color, 1.0f);

    return FboTexture->Handle();
  }

  void End() { Fbo->Unbind(); }
};

} // namespace gl
