add_library(PhoneCamFilter SHARED
    src/dllmain.cpp
    src/guids.h
    filter.def
)

target_include_directories(PhoneCamFilter PRIVATE src)

# 依存は BaseClasses、libyuv、WIL と OS ライブラリのみ（SPEC §9.2）。FFmpeg / ImGui を入れないこと。
target_link_libraries(PhoneCamFilter PRIVATE
    phonecam_options
    phonecam::common
    phonecam::baseclasses
    yuv
    WIL::WIL
    strmiids
    winmm
    ole32
    oleaut32
)
