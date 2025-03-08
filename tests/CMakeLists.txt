## Definition of tests

# Dependency: Google Test
include(CTest)
find_package(GTest REQUIRED)
include(GoogleTest)

# Create test application
add_executable(runUnitTests
    # Mocks & Utils
    # Tests
    "src/tests.cpp" )

target_include_directories(runUnitTests PRIVATE "include")

target_link_libraries(runUnitTests PRIVATE
    ${CMAKE_PROJECT_NAME}
    GTest::GTest
    GTest::gmock
    GTest::Main )

gtest_discover_tests(runUnitTests)
