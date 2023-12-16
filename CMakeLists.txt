# project definition
cmake_minimum_required(VERSION 3.14)
project(mylibrary)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# define primary build target
add_library(${PROJECT_NAME} STATIC
    "food.cpp"
    "cookie.cpp"
)

# one-time test configuration
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory("googletest/" EXCLUDE_FROM_ALL)
enable_testing()

# define test hooks
function(add_gtest test_name test_source)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PUBLIC ${CMAKE_SOURCE_DIR})
    target_link_libraries(${test_name} gtest gtest_main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# add individual test modules
add_gtest(test_addition "tests/test_addition.cpp")
