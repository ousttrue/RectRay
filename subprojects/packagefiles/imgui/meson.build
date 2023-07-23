project('imgui', 'cpp')

deps = []
args = [
    '-DIMGUI_USE_WCHAR32=1',
    '-DImDrawIdx=unsigned int',
    # '-DIMGUI_ENABLE_FREETYPE',
]
imgui_inc = include_directories(
    '.',
    'backends',
)

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'emscripten'
    link_args += ['-sUSE_GLFW=3', '-sUSE_WEBGL2=1']
else
    glfw_dep = dependency('glfw3', default_options: ['install=true'])
    deps += [glfw_dep]
endif

imgui_lib = static_library(
    'imgui',
    [
        'imgui.cpp',
        'imgui_demo.cpp',
        'imgui_draw.cpp',
        'imgui_tables.cpp',
        'imgui_widgets.cpp',
        'backends/imgui_impl_glfw.cpp',
        'backends/imgui_impl_opengl3.cpp',
        'misc/cpp/imgui_stdlib.cpp',
        # 'misc/freetype/imgui_freetype.cpp',
    ],
    dependencies: deps,
    cpp_args: args,
)

imgui_dep = declare_dependency(
    link_with: imgui_lib,
    include_directories: imgui_inc,
    compile_args: args,
    dependencies: deps,
)
