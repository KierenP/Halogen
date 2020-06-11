cmake_minimum_required(VERSION "3.18.0-rc1")

project("Halogen")

file(GLOB Halogen_SRC
    "*.h"
    "*.cpp"
)

add_executable("${PROJECT_NAME}" ${Halogen_SRC})