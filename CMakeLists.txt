cmake_minimum_required(VERSION 3.20)

project(EvolBeing_Gen_Alpha LANGUAGES CXX)

add_library(GeomCore STATIC src/SurfaceMesh.cpp)
target_include_directories(GeomCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(GeomCore PUBLIC cxx_std_20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(EvolBeing_Gen_Alpha app/main.cpp)
target_link_libraries(EvolBeing_Gen_Alpha PRIVATE GeomCore)

if(MSVC)
    target_compile_options(GeomCore PRIVATE /W4 /permissive-)
    target_compile_options(EvolBeing_Gen_Alpha PRIVATE /W4 /permissive-)
endif()