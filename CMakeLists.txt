cmake_minimum_required (VERSION 2.6)

project (pbr)

include(FindProtobuf)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})
if (MSVC)
  set(SFML_STATIC_LIBRARIES TRUE)
endif()
find_package(SFML 2 REQUIRED system window graphics)
#find_package(Boost REQUIRED)
#find_package(Protobuf REQUIRED)
# todo, why is this required to be upper case?
#find_package(BRISTOL COMPONENTS main sfml)

#include_directories(${Boost_INCLUDE_DIR})
include_directories(${SFML_INCLUDE_DIR})
#include_directories(${PROTOBUF_INCLUDE_DIR})
#include_directories(${BRISTOL_INCLUDE_DIR})

file(GLOB SRC "*.cpp" "*.hpp" "protocol/*.pb.cc")

add_executable(${PROJECT_NAME} ${SRC})

if (APPLE)
  # change c++ standard library to libc++ (llvm)
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

  # specifically link against a protobuf and boost build with libc++
  target_link_libraries(${PROJECT_NAME}
    ${SFML_LIBRARIES}
#    "/Users/dooz/Library/Caches/appCode30/DerivedData/bristol-e7802bc5/Build/Products/Debug/libbristol_main.a"
#    "/Users/dooz//Library/Caches/appCode30/DerivedData/bristol-e7802bc5/Build/Products/Debug/libbristol_sfml.a"
#    "/opt/local/boost/lib/libboost_date_time.a"
#    "/opt/local/protobuf/lib/libprotobuf.a"
    ${APP_SERVICES} )
else()
  if (MSVC)
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
      debug ${SFML_GRAPHICS_LIBRARY_DEBUG} optimized ${SFML_GRAPHICS_LIBRARY_RELEASE}
      debug ${SFML_SYSTEM_LIBRARY_DEBUG} optimized ${SFML_SYSTEM_LIBRARY_RELEASE}
      debug ${SFML_WINDOW_LIBRARY_DEBUG} optimized ${SFML_WINDOW_LIBRARY_RELEASE}
      debug ${BRISTOL_MAIN_LIBRARY_DEBUG} optimized ${BRISTOL_MAIN_LIBRARY_RELEASE}
      debug ${BRISTOL_SFML_LIBRARY_DEBUG} optimized ${BRISTOL_SFML_LIBRARY_RELEASE}
      ${Boost_DATETIME_LIBRARY}
      debug ${PROTOBUF_LIBRARY_DEBUG} optimized ${PROTOBUF_LIBRARY})
  endif(MSVC)
endif()
