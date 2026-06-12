cmake_minimum_required(VERSION 3.20)

project(calculator)

find_program(CLANG_TIDY_EXE clang-tidy)

if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
endif()

add_executable(
    calculator

    src/main.cpp
    src/parser.cpp
    src/checker.cpp
    src/calculator.cpp
    src/printer.cpp
    src/runner.cpp
)

find_program(CLANG_FORMAT_EXE clang-format)

if(CLANG_FORMAT_EXE)
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT_EXE}
        -i
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/include/*.hpp
    )
endif()

target_include_directories(
    calculator
    PRIVATE
    include
)

include(FetchContent)

FetchContent_Declare(
    mathlib
    GIT_REPOSITORY https://github.com/x-GR1ZZLY-x/mathlib.git
    GIT_TAG 2d18fb2
)

FetchContent_MakeAvailable(mathlib)

target_link_libraries(
    calculator
    PRIVATE
    mathlib
)

install(
    TARGETS calculator
    RUNTIME DESTINATION bin
)