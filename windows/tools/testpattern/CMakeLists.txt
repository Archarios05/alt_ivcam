add_executable(testpattern main.cpp)

target_link_libraries(testpattern PRIVATE
    phonecam_options
    phonecam::common
    yuv
    WIL::WIL
)
