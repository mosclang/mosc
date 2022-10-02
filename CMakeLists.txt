cmake_minimum_required(VERSION 3.8)
project(mosc)

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

#set(SOURCE_FILES
#        src/api/msc.h
#        src/builtin/Core.c src/builtin/Core.h
#        src/builtin/Primitive.c src/builtin/Primitive.h
#        src/builtin/core/core.msc.inc
#
#        src/common/codes.h src/common/common.h src/common/constants.h src/common/opcodes.h
#
#        src/compiler/Compiler.c src/compiler/Compiler.h
#        src/compiler/Parser.c src/compiler/Parser.h
#        src/compiler/Token.c src/compiler/Token.h
#
#        src/helpers/Helper.c src/helpers/Helper.h
#        src/memory/Value.c src/memory/Value.h
#        src/memory/GC.c src/memory/GC.h
#
#        src/runtime/debuger.c src/runtime/debuger.h
#        src/runtime/MVM.c src/runtime/MVM.h
#        src/meta/Fan.h src/meta/Fan.c src/meta/Fan.msc.inc src/meta/Kunfe.c src/meta/Kunfe.h src/meta/Kunfe.msc.inc)
# include_directories(src/api src/builtin src/common src/compiler src/helpers src/memory src/meta src/runtime)

file(GLOB SOURCE_FILES_C src/**/*.c)
file(GLOB SOURCE_FILES_H src/**/*.h)
file(GLOB SOURCE_FILES_INC src/**/*.h)
set(SOURCE_FILES ${SOURCE_FILES_C} ${SOURCE_FILES_H} ${SOURCE_FILES_INC})

IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    LIST(APPEND MSC_DEPS m)
ELSEIF (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    LIST(APPEND MSC_DEPS m)
ENDIF ()

add_library(mosc SHARED ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(mosc PRIVATE ${MSC_DEPS})
add_library(moscs ${SOURCE_FILES})
set(SOURCE_FILES test/main.c)
add_executable(moscc ${SOURCE_FILES})

link_directories(${PROJECT_BINARY_DIR}/out/library)

target_link_libraries(moscc mosc)

set_target_properties(moscs PROPERTIES OUTPUT_NAME "mosc")