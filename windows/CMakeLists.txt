cmake_minimum_required(VERSION 3.25)

project(PhoneCam VERSION 0.1.0 LANGUAGES C CXX)

option(PHONECAM_BUILD_RECEIVER "Receiver と testpattern をビルドする（x64 のみ）" ON)

if(NOT MSVC)
    message(FATAL_ERROR "PhoneCam の Windows コードは MSVC でビルドする")
endif()
if(PHONECAM_BUILD_RECEIVER AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Receiver / testpattern は x64 専用。x86 では PHONECAM_BUILD_RECEIVER=OFF にする")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CRT は静的リンク（/MT、Debug は /MTd）。フィルタは SPEC §9.2 で必須。
# 同じツリーの receiver / testpattern も vcpkg トリプレット（triplets/）に合わせて /MT にする。
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# サードパーティを含む全ターゲット共通
add_compile_definitions(UNICODE _UNICODE _WIN32_WINNT=0x0A00 WINVER=0x0A00)
add_compile_options(/utf-8)

# PhoneCam 自身のコードにだけ適用する設定
add_library(phonecam_options INTERFACE)
target_compile_definitions(phonecam_options INTERFACE NOMINMAX)
target_compile_options(phonecam_options INTERFACE /W4 /permissive- /Zc:__cplusplus)

# 両プロセス共通の定義（protocol.h / shared_frame.h / version.h）
add_library(phonecam_common INTERFACE)
add_library(phonecam::common ALIAS phonecam_common)
target_include_directories(phonecam_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/common/include)

# 共有ヘッダの static_assert を x86 / x64 の全プリセットで評価させる
add_library(phonecam_layout_check OBJECT common/layout_check.cpp)
target_link_libraries(phonecam_layout_check PRIVATE phonecam_options phonecam::common)

find_package(libyuv CONFIG REQUIRED)
find_package(wil CONFIG REQUIRED)

add_subdirectory(third_party/baseclasses)
add_subdirectory(filter)

if(PHONECAM_BUILD_RECEIVER)
    add_subdirectory(receiver)
    add_subdirectory(tools/testpattern)
endif()
