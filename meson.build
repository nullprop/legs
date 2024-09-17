project(
  'legs',
  'cpp',
  default_options: [
    'cpp_std=c++23',
    'b_ndebug=if-release',
    'warning_level=3',
  ],
  meson_version: '>=1.1',
)

cpp = meson.get_compiler('cpp')
buildtype = get_option('buildtype')

compiler_args = [
  '-Wcast-align',
  '-Wconversion',
  '-Wdouble-promotion',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wformat=2',
  '-Wimplicit-fallthrough',
  '-Wlogical-op',
  '-Wmisleading-indentation',
  '-Wnon-virtual-dtor',
  '-Wnrvo',
  '-Wnull-dereference',
  '-Wold-style-cast',
  '-Woverloaded-virtual',
  '-Wshadow',
  '-Wsign-conversion',
  '-Wsuggest-override',
  '-Wunused',
  '-Wuseless-cast',

  '-DGLM_FORCE_DEPTH_ZERO_TO_ONE',

  '-m64',

  '-DLINUX',
  '-D_LINUX',
  '-DPOSIX',

  '-DJPH_OBJECT_STREAM',
  '-DCPP_RTTI_ENABLED',
]

linker_args = [
  '-m64',
]

if buildtype == 'debug'
  compiler_args += [
    '-DJPH_SHARED_LIBRARY',
    '-DJPH_PROFILE_ENABLED',
    '-DJPH_DEBUG_RENDERER',
  ]
endif

add_project_arguments(cpp.get_supported_arguments(compiler_args), language: 'cpp')
add_project_link_arguments(cpp.get_supported_link_arguments(linker_args), language: 'cpp')

# https://github.com/mesonbuild/meson/issues/13601
# jolt_opt = cmake.subproject_options()
# jolt_dep = cmake.subproject('Jolt', options: jolt_opt)
jolt_dep = cpp.find_library('Jolt', dirs: [meson.current_source_dir() + '/lib'])

# TODO: is there a way to disable warnings for these?
imgui_src = files(

  'include/imgui/backends/imgui_impl_sdl2.cpp',
  'include/imgui/backends/imgui_impl_vulkan.cpp',
  'include/imgui/imgui.cpp',
  'include/imgui/imgui_demo.cpp',
  'include/imgui/imgui_draw.cpp',
  'include/imgui/imgui_tables.cpp',
  'include/imgui/imgui_widgets.cpp',
)

glsl_compiler = find_program('glslang', 'glslangValidator')
glsl_args = [
  '--target-env', 'vulkan1.3',
  '--vn', '@BASENAME@',
  '--depfile', '@DEPFILE@',
  '@INPUT@',
  '-o', '@OUTPUT@',
]
glsl_generator = generator(
  glsl_compiler,
  output: ['@BASENAME@.h'],
  depfile: '@BASENAME@.h.d',
  arguments: glsl_args,
)

legs_includes = include_directories(
  'include/VulkanMemoryAllocator/include',

  'include/glm',

  'include/JoltPhysics',

  'include/imgui',
  'include/imgui/backends',

  is_system: true,
)

legs_version = vcs_tag(
  command: ['git', 'describe', '--always', '--dirty=+'],
  input: 'version.h.in',
  output: 'version.h',
)

subdir('src')

examples = get_option('examples')
if examples == true
  subdir('examples')
endif
