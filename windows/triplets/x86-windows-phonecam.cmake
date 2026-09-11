# x86 用トリプレット（x86-release プリセットでフィルタのみビルド）。
# フィルタは CRT 静的リンク（/MT、SPEC §9.2）。x86 では FFmpeg / ImGui を取得しない。
set(VCPKG_TARGET_ARCHITECTURE x86)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)
