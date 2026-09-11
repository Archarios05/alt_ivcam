find_package(FFMPEG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(Receiver src/main.cpp)

# FFmpeg は DLL（vcpkg の applocal デプロイで出力フォルダにコピーされる）
target_include_directories(Receiver PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(Receiver PRIVATE ${FFMPEG_LIBRARY_DIRS})

target_link_libraries(Receiver PRIVATE
    phonecam_options
    phonecam::common
    ${FFMPEG_LIBRARIES}
    imgui::imgui
    yuv
    WIL::WIL
)
