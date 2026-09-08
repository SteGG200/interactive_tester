cmake_minimum_required(VERSION 3.14)
project(interactive_tester CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/interactive_tester.cpp
)

if(WIN32)
    list(APPEND SOURCES src/process_win.cpp)
else()
    list(APPEND SOURCES src/process_unix.cpp)
endif()

add_executable(interactive_tester ${SOURCES})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(interactive_tester PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(BUILD_TESTS "Build test fixtures" ON)
if(BUILD_TESTS)
    add_executable(echo_interactor test/echo_interactor.cpp)
    add_executable(guess_solution test/guess_solution.cpp)
    add_executable(hang_solution test/hang_solution.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(echo_interactor PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(guess_solution PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(hang_solution PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS interactive_tester RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# CPack packaging configuration
set(CPACK_PACKAGE_NAME "interactive-tester")
set(CPACK_PACKAGE_VENDOR "SteGG")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Interactive tester for competitive programming")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "InteractiveTester")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" COPYONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")
endif()

# DEB-specific settings
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SteGG")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17)")

# RPM-specific settings
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")

# WiX-specific settings
set(CPACK_WIX_UPGRADE_GUID "A2F89C41-9E8D-4E13-908F-43B7C2018A5D")

include(CPack)

