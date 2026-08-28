cmake_minimum_required(VERSION 3.25)

project(learning_journey LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(chat src/main.cpp)
target_link_libraries(chat PRIVATE nlohmann_json::nlohmann_json)

if (MSVC)
    target_compile_options(chat PRIVATE /W4 /EHsc)
endif()
