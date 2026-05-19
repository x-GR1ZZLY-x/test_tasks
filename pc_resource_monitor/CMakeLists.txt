cmake_minimum_required(VERSION 3.16)

project(pc_resource_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pc-monitor
    src/main.cpp
)

if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "This project targets Linux because metrics are read from /proc.")
endif()

find_program(NODE_EXECUTABLE node)
if(NOT NODE_EXECUTABLE)
    message(FATAL_ERROR "node is required to build the TypeScript frontend. Install nodejs and npm, then run npm install.")
endif()

set(TS_COMPILER "${CMAKE_SOURCE_DIR}/node_modules/typescript/bin/tsc")
set(WEB_JS "${CMAKE_SOURCE_DIR}/web/app.js")

if(NOT EXISTS "${TS_COMPILER}")
    message(FATAL_ERROR "TypeScript compiler is not installed. Run npm install before configuring or building the project.")
endif()

add_custom_command(
    OUTPUT "${WEB_JS}"
    COMMAND "${NODE_EXECUTABLE}" "${TS_COMPILER}" -p "${CMAKE_SOURCE_DIR}/tsconfig.json"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/web/app.ts"
        "${CMAKE_SOURCE_DIR}/tsconfig.json"
        "${CMAKE_SOURCE_DIR}/package.json"
        "${CMAKE_SOURCE_DIR}/package-lock.json"
        "${TS_COMPILER}"
    COMMENT "Building TypeScript WebUI"
)

add_custom_target(build-web ALL DEPENDS "${WEB_JS}")

add_custom_target(copy-web ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:pc-monitor>/web"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/web/index.html"
            "${CMAKE_SOURCE_DIR}/web/styles.css"
            "${WEB_JS}"
            "$<TARGET_FILE_DIR:pc-monitor>/web"
    COMMENT "Copying WebUI files"
)

add_dependencies(copy-web pc-monitor build-web)
