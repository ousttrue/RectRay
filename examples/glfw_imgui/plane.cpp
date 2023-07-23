#include "plane.h"
#include "gl_api.h"
#include <rectray.h>

// https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/

const auto VS = R"(
in vec3 aPos;

out float near; //0.01
out float far; //100
out vec3 nearPoint;
out vec3 farPoint;
out mat4 fragView;
out mat4 fragProj;
out vec4 outColor;

// Shared set between most vertex shaders
uniform ViewUniforms {
    mat4 view;
    mat4 proj;
    vec3 pos;
} view;

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

// normal vertice projection
void main() {
    vec3 p = aPos;
    // unprojecting on the near plane
    nearPoint = UnprojectPoint(p.x, p.y, -1.0, view.view, view.proj).xyz;
    // unprojecting on the far plane
    farPoint = UnprojectPoint(p.x, p.y, 1.0, view.view, view.proj).xyz;
    // using directly the clipped coordinates
    gl_Position = vec4(p, 1.0);

    fragView = view.view;
    fragProj = view.proj;

    near = view.pos.x;
    far = view.pos.y;
}

)";

const auto FS = R"(
in float near; //0.01
in float far; //100
// nearPoint calculated in vertex shader
in vec3 nearPoint; 
// farPoint calculated in vertex shader
in vec3 farPoint;
in mat4 fragView;
in mat4 fragProj;
out vec4 outColor;

const vec4 line_color = vec4(0.4, 0.4, 0.4, 1.0);

vec4 grid(vec3 fragPos3D, float scale, bool drawAxis) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = line_color;
    color.a-=min(line, 1.0);
    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.z = 1.0;
    // x axis
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.x = 1.0;
    return color;
}

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    // https://community.khronos.org/t/computing-new-gl-fragdepth-value/105107
    return (clip_space_pos.z / clip_space_pos.w + 1.0)*0.5;
}

float computeLinearDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w);
    float linearDepth = (2.0 * near * far) / (far + near - clip_space_depth * (far - near)); // get linear value between 0.01 and 100
    return linearDepth / far; // normalize
}

void main() {
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    gl_FragDepth = computeDepth(fragPos3D);

    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0, (0.5 - linearDepth));

    outColor = (grid(fragPos3D, 10, true) + grid(fragPos3D, 1, true))* float(t > 0); // adding multiple resolution for the grid
    outColor.a *= fading;
}
)";

struct PlaneImpl {
  std::shared_ptr<gl::ShaderProgram> m_shader;
  std::shared_ptr<gl::Ubo> m_ubo;
  std::shared_ptr<gl::Vao> m_vao;

  struct ViewUniforms {
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 proj;
    DirectX::XMFLOAT3 pos;
  };
  ViewUniforms m_ubo_data;

  PlaneImpl() {
    m_shader = gl::ShaderProgram::FromSource(VS, FS);
    m_ubo = gl::Ubo::Create(m_ubo_data);

    // Grid position are in xy clipped space
    DirectX::XMFLOAT3 GridPlane[] = {
        {1, 1, 0},   {-1, -1, 0}, {-1, 1, 0}, // |/
        {-1, -1, 0}, {1, 1, 0},   {1, -1, 0}, // /|
    };
    auto vbo = gl::Vbo::Create(GridPlane);
    // auto ibo = gl::Ibo::Create(indices);
    gl::Attribute attributes[] = {
        {0, 3, GL_FLOAT, GL_FALSE, sizeof(DirectX::XMFLOAT3), 0},
    };
    m_vao = gl::Vao::Create(vbo, attributes);
  }

  void Render(const rectray::Camera &camera) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_ubo_data.view = camera.ViewMatrix;
    m_ubo_data.proj = camera.ProjectionMatrix;
    m_ubo_data.pos.x = camera.Projection.NearZ;
    m_ubo_data.pos.y = camera.Projection.FarZ;

    m_ubo->Upload(m_ubo_data);
    m_ubo->SetBindingPoint(0);

    m_shader->UboBind(0, 0);
    m_shader->Use();
    // m_shader->SetMat4(m_mvp, &viewProjection._11);
    m_vao->Draw(GL_TRIANGLES, 6);
  }
};

Plane::Plane() : m_impl(new PlaneImpl) {}

Plane::~Plane() { delete m_impl; }

void Plane::Render(const rectray::Camera &camera) { m_impl->Render(camera); }
