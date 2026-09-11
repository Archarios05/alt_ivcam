# DirectShow BaseClasses（microsoft/Windows-classic-samples からベンダリング、MIT）。README.md 参照。
add_library(baseclasses STATIC
    amextra.cpp
    amfilter.cpp
    amvideo.cpp
    arithutil.cpp
    combase.cpp
    cprop.cpp
    ctlutil.cpp
    ddmm.cpp
    dllentry.cpp
    dllsetup.cpp
    mtype.cpp
    outputq.cpp
    perflog.cpp
    pstream.cpp
    pullpin.cpp
    refclock.cpp
    renbase.cpp
    schedule.cpp
    seekpt.cpp
    source.cpp
    strmctl.cpp
    sysclock.cpp
    transfrm.cpp
    transip.cpp
    videoctl.cpp
    vtrans.cpp
    winctrl.cpp
    winutil.cpp
    wxdebug.cpp
    wxlist.cpp
    wxutil.cpp
)
add_library(phonecam::baseclasses ALIAS baseclasses)

target_include_directories(baseclasses PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 古いコードのため C++17・非準拠モードでビルドする（/std:c++20 は /permissive- を暗黙に有効にする）。
# DEBUG マクロは streams.h が _DEBUG（/MTd で自動定義）から定義するので、ここでは指定しない。
set_target_properties(baseclasses PROPERTIES CXX_STANDARD 17)
target_compile_options(baseclasses PRIVATE /permissive /W3)

target_link_libraries(baseclasses PUBLIC strmiids winmm ole32 oleaut32 uuid)
