
# укажите здесь логику для конкретного проекта.
cmake_minimum_required (VERSION 3.28)
project ("pinqergen")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


add_library(PrngCore STATIC
    "src/Errors.cpp"
    "src/BigUint.cpp"
    "src/Lfsr.cpp"
    "src/Statistics.cpp"
    "src/ClassicGenerators.cpp"
    "src/CryptoGenerators.cpp"
    "src/GeneratorManager.cpp"
)
target_include_directories(PrngCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")


add_executable (pinqergen "main.cpp")
target_link_libraries(pinqergen PRIVATE PrngCore)


include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(PrngTests "tests/test_prng.cpp")
target_link_libraries(PrngTests PRIVATE PrngCore gtest_main)

include(GoogleTest)
gtest_discover_tests(PrngTests)



# TODO: Добавьте тесты и целевые объекты, если это необходимо.
