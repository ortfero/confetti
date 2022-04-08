project('confetti', 'cpp',
    license: 'MIT',
    version: '0.1.1',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3'])

headers = [
    'include/confetti/confetti.hpp'
]

incdirs = include_directories('./include')

confetti = declare_dependency(
    version: meson.project_version(),
    include_directories: incdirs,
    sources: headers
)

subdir('test')

install_headers(headers, subdir: 'confetti')

pkg = import('pkgconfig')
pkg.generate(
    name: 'confetti',
    filebase: 'confetti',
    subdirs: ['confetti'],
    version: meson.project_version(),
    description: 'C++17 one-header library to parse ini files with some toml extensions'
)
