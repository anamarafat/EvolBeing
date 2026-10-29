cmake_minimum_required(VERSION 3.20)

project(EvolBeing_Gen_Alpha LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(GeomCore STATIC 
    src/SurfaceMesh.cpp
    src/ObjIO.cpp
    src/CartesianGrid.cpp
)
target_include_directories(GeomCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(GeomCore PUBLIC cxx_std_20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(EvolBeing_Gen_Alpha
            app/main.cpp
            app/MainWindow.cpp
            include/MainWindow.hpp
            app/Gui2DWindow.cpp
            include/Gui2DWindow.hpp
            app/MeshGraphicsView.cpp
            include/MeshGraphicsView.hpp
            )
target_include_directories(EvolBeing_Gen_Alpha
                    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
                    )

target_link_libraries(EvolBeing_Gen_Alpha 
                      PRIVATE 
                        GeomCore
                        Qt6::Widgets
                      )

if(MSVC)
    target_compile_options(GeomCore PRIVATE /W4 /permissive-)
    target_compile_options(EvolBeing_Gen_Alpha PRIVATE /W4 /permissive-)
endif()