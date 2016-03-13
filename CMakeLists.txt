cmake_minimum_required (VERSION 3.4)

project ("Windows Disk ACL Editor" LANGUAGES CXX RC)

add_compile_options (-std=c++14 -Wpedantic -Wall -Wextra -municode)
add_definitions (-DSTRICT -DWIN32_MEAN_AND_LEAN -D_UNICODE)

add_executable (wdae WIN32 disks.cpp explain.cpp main_window.cpp wdae.cpp wdae-rc.rc)
target_link_libraries (wdae PUBLIC comctl32 ole32 wbemuuid setupapi)
target_link_libraries (wdae PRIVATE -static -municode -Wl,--dynamicbase -Wl,--high-entropy-va -Wl,--nxcompat)
