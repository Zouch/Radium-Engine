
add_custom_target( update )
external_download_now_git(Catch2
    https://github.com/catchorg/Catch2.git
    v2.12.4)

set(CATCH_USE_VALGRIND OFF CACHE BOOL "Perform SelfTests with Valgrind")
set(CATCH_BUILD_TESTING OFF CACHE BOOL "Build SelfTest project")
set(CATCH_BUILD_EXAMPLES OFF CACHE BOOL "Build documentation examples")
set(CATCH_BUILD_EXTRA_TESTS OFF CACHE BOOL "Build extra tests")
set(CATCH_ENABLE_COVERAGE OFF CACHE BOOL "Generate coverage for codecov.io")
set(CATCH_ENABLE_WERROR OFF CACHE BOOL "Enable all warnings as errors")
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "Install documentation alongside library")
set(CATCH_INSTALL_HELPERS OFF CACHE BOOL "Install contrib alongside library")

add_subdirectory(${Catch2_SOURCE_DIR} EXCLUDE_FROM_ALL)
