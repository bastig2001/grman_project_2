project('grman_project_2', 'cpp',
        license : ['Boost'],
        version : '0.1.6',
        default_options : ['cpp_std=c++17', 'warning_level=3', 'werror=true'])

thread = dependency('threads')
crypto = dependency('libcrypto')

# begin asio: https://think-async.com/Asio/
# add_global_arguments('-I' + get_option('asio_include_dir'), language : 'cpp')
# add_global_arguments('-DASIO_STANDALONE', language : 'cpp')
add_global_arguments('-DASIO_NO_DEPRECATED', language : 'cpp')
# end asio

# CLI11: https://github.com/CLIUtils/CLI11
add_global_arguments('-I' + get_option('cli11_include_dir'), language : 'cpp')

# fmt: https://github.com/fmtlib/fmt
fmt = dependency('fmt')

# json: https://github.com/nlohmann/json
# add_global_arguments('-I' + get_option('json_include_dir'), language : 'cpp')
json = dependency('nlohmann_json')

# peglib: https://github.com/yhirose/cpp-peglib
add_global_arguments('-I' + get_option('peglib_include_dir'), language: 'cpp')

# spdlog: https://github.com/gabime/spdlog
# add_global_arguments('-I' + get_option('spdlog_include_dir'), language : 'cpp')
# add_global_arguments('-DSPDLOG_FMT_EXTERNAL=ON', language : 'cpp')

# sqlite orm: https://github.com/fnc12/sqlite_orm
add_global_arguments('-I' + get_option('sqlite_orm_include_dir'), language : 'cpp')
add_global_arguments('-DSQLITE_ORM_OPTIONAL_SUPPORTED', language: 'cpp')

# begin protobuf: https://developers.google.com/protocol-buffers/
# protobuf: must be installed independently and has to be found...
protoc = find_program('protoc', required : true)
protobuf = dependency('protobuf', required : true)

protoc = generator(
    protoc,
    output    : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
    arguments : ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@']
)

# change proto files accordingly
messages = protoc.process('messages/all.proto',
                          'messages/sync.proto', 
                          'messages/info.proto', 
                          'messages/download.proto',
                          'messages/basic.proto',
                          preserve_path_from : meson.current_source_dir()
                         )
# end protobuf

inc_dir = include_directories('include')

src = [
    'src/client.cpp',
    'src/config.cpp',
    'src/database.cpp',
    'src/file_operator.cpp',
    'src/message_utils.cpp',
    'src/server.cpp',
    'src/utils.cpp',
    'src/file_operator/filesystem.cpp',
    'src/file_operator/operator_utils.cpp',
    'src/file_operator/signatures.cpp',
    'src/file_operator/sync_system.cpp',
    'src/presentation/command_line.cpp',
    'src/presentation/format_utils.cpp',
    'src/presentation/logger_config.cpp'
]

unit_tests_src = [
    'src/unit_tests/json_utils.cpp',
    'src/unit_tests/main.cpp',
    'src/unit_tests/operator_utils.cpp',
    'src/unit_tests/pipe.cpp',
    'src/unit_tests/signatures.cpp',
    'src/unit_tests/type.cpp',
    'src/unit_tests/utils.cpp'
]

dependencies = [thread, fmt, protobuf, crypto, json]

executable('sync', 
           messages,
           sources : ['src/main.cpp'] + src, 
           include_directories : inc_dir,
           dependencies : dependencies
          )

executable('unit_tests', 
           messages,
           sources : unit_tests_src + src, 
           include_directories : inc_dir,
           dependencies : dependencies,
           cpp_args : [
               '-DUNIT_TESTS', 
               # doctest: https://github.com/onqtam/doctest
               '-DDOCTEST_CONFIG_SUPER_FAST_ASSERTS', 
               '-I' + get_option('doctest_include_dir')
            ]
          )
