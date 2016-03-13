cmake_minimum_required(VERSION 3.4)

project("Windows Disk ACL Editor" LANGUAGES CXX RC)

add_executable(wdae WIN32 disks.cpp explain.cpp main_window.cpp wdae.cpp wdae-rc.rc)

target_compile_definitions(wdae PUBLIC -DSTRICT -DWIN32_MEAN_AND_LEAN -D_UNICODE)

target_compile_options(wdae PUBLIC -std=c++14 -Wpedantic -Wall -Wextra -municode)

target_link_libraries(wdae PUBLIC comctl32 ole32 wbemuuid setupapi)
target_link_libraries(wdae PUBLIC -static -municode -Wl,--dynamicbase -Wl,--high-entropy-va -Wl,--nxcompat)
