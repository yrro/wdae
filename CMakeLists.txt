cmake_minimum_required(VERSION 3.4)

project("Windows Disk ACL Editor" LANGUAGES C CXX RC)

add_executable(wdae WIN32 disks.cpp explain.cpp main_window.cpp wdae.cpp wdae-rc.rc)

target_compile_definitions(wdae PUBLIC -DSTRICT -DWIN32_MEAN_AND_LEAN -D_UNICODE)

target_compile_options(wdae PUBLIC -std=c++14 -Wpedantic -Wall -Wextra -municode)

target_link_libraries(wdae PUBLIC comctl32 ole32 wbemuuid setupapi vboxglue)
target_link_libraries(wdae PUBLIC -static -municode -Wl,--dynamicbase -Wl,--high-entropy-va -Wl,--nxcompat)

file(DOWNLOAD http://download.virtualbox.org/virtualbox/5.1.4/VirtualBoxSDK-5.1.4-110228.zip vboxsdk/VirtualBoxSDK-5.1.4-110228.zip EXPECTED_HASH SHA256=9216a91ba32a39788f3a5e7ba160268aecf1e6d7d3af4169bbc1a569e0a87359)
add_custom_command(
    OUTPUT vboxsdk/sdk/bindings/c/glue/VBoxCAPIGlue.c
    COMMAND "${CMAKE_COMMAND}" -E tar xf VirtualBoxSDK-5.1.4-110228.zip && patch -p1 < "${CMAKE_SOURCE_DIR}/VBoxCAPI_v5_1-fixcase.patch"
    WORKING_DIRECTORY vboxsdk
    DEPENDS "${CMAKE_BINARY_DIR}/vboxsdk/VirtualBoxSDK-5.1.4-110228.zip"
)
add_library(vboxglue STATIC "${CMAKE_BINARY_DIR}/vboxsdk/sdk/bindings/c/glue/VBoxCAPIGlue.c")
target_include_directories(vboxglue PUBLIC "${CMAKE_BINARY_DIR}/vboxsdk/sdk/bindings/c/include")
target_include_directories(vboxglue PUBLIC "${CMAKE_BINARY_DIR}/vboxsdk/sdk/bindings/mscom/include")

file(DOWNLOAD https://www.virtualbox.org/svn/vbox/trunk/include/iprt/cdefs.h vboxsdk/extra/iprt/cdefs.h TLS_VERIFY ON)
file(DOWNLOAD https://www.virtualbox.org/svn/vbox/trunk/include/iprt/stdint.h vboxsdk/extra/iprt/stdint.h TLS_VERIFY ON)
file(DOWNLOAD https://www.virtualbox.org/svn/vbox/trunk/include/iprt/win/windows.h vboxsdk/extra/iprt/win/windows.h TLS_VERIFY ON)
target_include_directories(vboxglue PUBLIC "${CMAKE_BINARY_DIR}/vboxsdk/extra")
target_compile_definitions(vboxglue PUBLIC -DIN_RING3)
