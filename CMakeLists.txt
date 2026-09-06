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

include(GNUInstallDirs)
install(TARGETS interactive_tester RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
