cmake_minimum_required(VERSION 3.25)

project(smstrader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

include(FetchContent)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Qt6 COMPONENTS Widgets Network WebSockets REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)

set(CMAKE_AUTOMOC ON)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
if (WIN32)
    target_compile_definitions(asio INTERFACE _WIN32_WINNT=0x0A00)
    target_link_libraries(asio INTERFACE ws2_32 mswsock)
endif()

FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG master
)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(crow)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(catch2)

add_executable(smstrader
    src/main.cpp
    src/command.cpp
    src/message.cpp
    src/datetime.cpp
    src/search.cpp
    src/storage.cpp
    src/validation.cpp
)
target_link_libraries(smstrader PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(smstrader PRIVATE src)

if (MSVC)
    target_compile_options(smstrader PRIVATE /W4 /EHsc)
endif()

add_executable(smstrader_server
    src/server/main.cpp
    src/message.cpp
    src/datetime.cpp
    src/search.cpp
    src/postgres_storage.cpp
)
target_link_libraries(smstrader_server PRIVATE asio Threads::Threads OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json PostgreSQL::PostgreSQL)
target_include_directories(smstrader_server PRIVATE src)
if (MSVC)
    target_compile_options(smstrader_server PRIVATE /W4 /EHsc)
endif()

add_executable(smstrader_client src/client/main.cpp)
target_link_libraries(smstrader_client PRIVATE asio Threads::Threads OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)
target_include_directories(smstrader_client PRIVATE src)
if (MSVC)
    target_compile_options(smstrader_client PRIVATE /W4 /EHsc)
endif()

add_executable(smstrader_gui
    src/gui/main.cpp
    src/gui/login_dialog.cpp
    src/gui/main_window.cpp
    src/gui/network_client.cpp
)
target_link_libraries(smstrader_gui PRIVATE Threads::Threads nlohmann_json::nlohmann_json Qt6::Widgets Qt6::Network Qt6::WebSockets)
target_include_directories(smstrader_gui PRIVATE src src/gui)
if (MSVC)
    target_compile_options(smstrader_gui PRIVATE /W4 /EHsc)
endif()

# Qt ищет свои платформенные плагины (qwindows.dll) в подпапке "platforms/"
# рядом с exe. vcpkg их туда не кладёт сам — копируем после сборки.
add_custom_command(TARGET smstrader_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:smstrader_gui>/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/Qt6/plugins/platforms/qwindowsd.dll,${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/platforms/qwindows.dll>"
        "$<TARGET_FILE_DIR:smstrader_gui>/platforms/"
    COMMENT "Copying Qt platform plugin"
)

# Тот же баг, другой плагин: с Qt6 TLS-бэкенды (OpenSSL/Schannel/...) тоже
# грузятся как плагины из подпапки "tls/" рядом с exe, а не линкуются
# напрямую — без этого QNetworkAccessManager/QWebSocket падают с "No
# functional TLS backend was found" при любой попытке HTTPS/WSS-соединения.
add_custom_command(TARGET smstrader_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:smstrader_gui>/tls"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/Qt6/plugins/tls/qopensslbackendd.dll,${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/tls/qopensslbackend.dll>"
        "$<TARGET_FILE_DIR:smstrader_gui>/tls/"
    COMMENT "Copying Qt TLS backend plugin"
)

add_executable(smstrader_http
    src/http/main.cpp
    src/message.cpp
    src/datetime.cpp
    src/search.cpp
    src/postgres_storage.cpp
    src/user_storage.cpp
    src/password_hash.cpp
)
target_link_libraries(smstrader_http PRIVATE
    Crow::Crow
    asio
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    PostgreSQL::PostgreSQL
    unofficial::argon2::libargon2
    jwt-cpp::jwt-cpp
)
target_include_directories(smstrader_http PRIVATE src)
if (MSVC)
    # /bigobj: шаблонный код Crow (особенно websocket.h) генерирует больше секций
    # в объектном файле, чем стандартный формат COFF допускает по умолчанию
    target_compile_options(smstrader_http PRIVATE /W4 /EHsc /bigobj)
endif()

enable_testing()

# Один тестовый бинарник на все модули с логикой (Catch2 — см. ROADMAP,
# Этап 2). Модули без содержательной логики (main.cpp каждого исполняемого
# файла, GUI, сетевой код) тестами не покрываем — там нечего проверять
# юнит-тестом, кроме "компилируется и не падает", что уже даёт сама сборка.
add_executable(smstrader_tests
    tests/command_test.cpp
    tests/datetime_test.cpp
    tests/password_hash_test.cpp
    tests/search_test.cpp
    tests/validation_test.cpp
    src/command.cpp
    src/datetime.cpp
    src/password_hash.cpp
    src/search.cpp
    src/validation.cpp
)
target_link_libraries(smstrader_tests PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
    OpenSSL::Crypto
    unofficial::argon2::libargon2
)
target_include_directories(smstrader_tests PRIVATE src)
if (MSVC)
    target_compile_options(smstrader_tests PRIVATE /W4 /EHsc)
endif()

include(CTest)
include(Catch)
catch_discover_tests(smstrader_tests)
