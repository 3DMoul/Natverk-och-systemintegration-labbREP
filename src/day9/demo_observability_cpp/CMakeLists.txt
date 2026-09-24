cmake_minimum_required(VERSION 3.16)
project(iot25_observability_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(observability_server server.cpp)
add_executable(observability_client client.cpp)

if(MSVC)
  target_compile_options(observability_server PRIVATE /W4)
  target_compile_options(observability_client PRIVATE /W4)
else()
  target_compile_options(observability_server PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(observability_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
  target_link_libraries(observability_server PRIVATE ws2_32)
  target_link_libraries(observability_client PRIVATE ws2_32)
endif()
