cmake_minimum_required(VERSION 3.8)
project(mosc_try)

set(CMAKE_C_STANDARD 99)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(MSC_DEPS)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g")

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)


file(GLOB SOURCE_FILES_C ../src/**/*.c)
file(GLOB SOURCE_FILES_H ../src/**/*.h)
file(GLOB SOURCE_FILES_INC ../src/**/*.h)
set(SOURCE_FILES ${SOURCE_FILES_C} ${SOURCE_FILES_H} ${SOURCE_FILES_INC})

IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    LIST(APPEND MSC_DEPS m)
ELSEIF (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    LIST(APPEND MSC_DEPS m)
ENDIF ()

add_library(mosc ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(mosc PRIVATE ${MSC_DEPS})
set(SOURCE_FILES main.try.c)
add_executable(mosc-try ${SOURCE_FILES})

set_target_properties(mosc-try
        PROPERTIES SUFFIX ".html"
        LINK_FLAGS " -s WASM=1 -s FILESYSTEM=0 -s EXIT_RUNTIME=0 -s ENVIRONMENT='web' -s EXPORTED_FUNCTIONS=\"['_main', '_mosc_compile']\" -s EXTRA_EXPORTED_RUNTIME_METHODS=\"['ccall', 'cwrap']\"")

target_link_libraries(mosc-try mosc)