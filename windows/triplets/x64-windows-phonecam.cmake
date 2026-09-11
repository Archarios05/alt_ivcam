# x64 用トリプレット。
# フィルタは CRT 静的リンク（/MT、SPEC §9.2）が必須で、同じビルドツリーの receiver / testpattern も
# 同じ libyuv を使うため、既定はすべて静的ライブラリ + 静的 CRT とする。
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# FFmpeg は LGPL の範囲で使うため DLL で動的リンクする（SPEC §4.1 / §13）。
# FFmpeg の C API は確保・解放を av_malloc / av_free で完結させるので、CRT が異なっても問題ない。
if(PORT STREQUAL "ffmpeg")
    set(VCPKG_CRT_LINKAGE dynamic)
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
