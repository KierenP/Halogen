set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON) 
set (CMAKE_CXX_EXTENSIONS OFF) 

cmake_minimum_required(VERSION "3.18.0-rc1")

project("Halogen")

file(GLOB Halogen_SRC
    "*.h"
    "*.cpp"
)

add_executable("${PROJECT_NAME}" ${Halogen_SRC})
