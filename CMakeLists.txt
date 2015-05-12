cmake_minimum_required (VERSION 2.6)

project (pbr)

include(FindProtobuf)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})
if (MSVC)
  set(SFML_STATIC_LIBRARIES TRUE)
endif()
find_package(SFML 2 REQUIRED system window graphics)
find_package(OpenGL)

file(GLOB SRC "*.cpp" "*.hpp" "imgui/*.cpp" "imgui/*.h")

add_executable(${PROJECT_NAME} ${SRC})

if (APPLE)
  # change c++ standard library to libc++ (llvm)
  include_directories(${SFML_INCLUDE_DIR} "/Users/dooz/projects/tbb43/include" )
  set(COMMON_FLAGS "-Wno-switch-enum")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -std=c++11 -stdlib=libc++")
  find_library(APP_SERVICES ApplicationServices)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/precompiled.hpp"
    XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES"
    LINK_FLAGS "-F/Library/Frameworks")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

  target_link_libraries(${PROJECT_NAME}
    ${SFML_LIBRARIES}
    ${SFML_LIBRARIES}
    ${OPENGL_LIBRARIES}
    "/Users/dooz/projects/tbb43/lib/libtbb.dylib"
    "/usr/local/Cellar/glfw3/3.0.4/lib/libglfw3.dylib"
    ${APP_SERVICES} )
else()
  if (MSVC)
    include_directories(${SFML_INCLUDE_DIR} "c:/projects/tbb43/include")
    # global all the root level .cpp files
    file(GLOB ROOT_SRC "*.cpp")

    # add precompiled header, and force include it on all the root level .cpp files
    foreach( src_file ${ROOT_SRC} )
        set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS "/Yuprecompiled.hpp /FIprecompiled.hpp")
    endforeach( src_file ${ROOT_SRC} )

    set_source_files_properties(precompiled.cpp PROPERTIES COMPILE_FLAGS "/Ycprecompiled.hpp")

    # Force static runtime libraries
    foreach(flag CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_DEBUG)
      STRING(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
      SET("${flag}" "${${flag}}")
    endforeach()

    target_link_libraries(${PROJECT_NAME}
      debug "c:/projects/tbb43/lib/intel64/vc12/tbb_debug.lib" optimized "c:/projects/tbb43/lib/intel64/vc12/tbb.lib"
      "glfw3/glfw3dll"
      "opengl32"
    )
      #debug ${SFML_GRAPHICS_LIBRARY_DEBUG} optimized ${SFML_GRAPHICS_LIBRARY_RELEASE}
      #debug ${SFML_SYSTEM_LIBRARY_DEBUG} optimized ${SFML_SYSTEM_LIBRARY_RELEASE}
      #debug ${SFML_WINDOW_LIBRARY_DEBUG} optimized ${SFML_WINDOW_LIBRARY_RELEASE})
  endif(MSVC)
endif()
