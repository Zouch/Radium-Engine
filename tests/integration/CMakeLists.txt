add_custom_target(integration)

# this target generates all references files with HEAD version of the source code, use it when
# updating the tests while not updating the libs
add_custom_target(integration_create_ref)

set(INTEGRATION_TEST TinyPlyFileLoader)
# TinyPlyFileLoader read tests
add_executable(integration_${INTEGRATION_TEST} ${INTEGRATION_TEST}/main.cpp)
target_link_libraries(integration_${INTEGRATION_TEST} PUBLIC Core IO)

add_custom_target(integration_${INTEGRATION_TEST}_create_ref)
add_dependencies(integration_create_ref integration_${INTEGRATION_TEST}_create_ref)

add_custom_command(
    TARGET integration_${INTEGRATION_TEST}_create_ref
    PRE_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${INTEGRATION_TEST}/
    COMMAND
        bash -c
        "$<TARGET_FILE:integration_${INTEGRATION_TEST}>  2>&1 | cut -f4- -d: | grep -v \"Loading Time\" > ${CMAKE_CURRENT_SOURCE_DIR}/data/references/${INTEGRATION_TEST}.txt"
    COMMAND_EXPAND_LISTS VERBATIM
    COMMENT "generating references for integration_${INTEGRATION_TEST} integration test" DEPENDS
            integration_${INTEGRATION_TEST}
)

add_test(
    NAME integration_${INTEGRATION_TEST}_save_output
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${INTEGRATION_TEST}/
    COMMAND
        bash -c
        "$<TARGET_FILE:integration_${INTEGRATION_TEST}> 2>&1 | cut -f4- -d: | grep -v \"Loading Time\" > ${CMAKE_CURRENT_BINARY_DIR}/output.txt"
)

add_test(
    NAME integration_${INTEGRATION_TEST}_compare_to_ref
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${INTEGRATION_TEST}/
    COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/output.txt
            ${CMAKE_CURRENT_SOURCE_DIR}/data/references/${INTEGRATION_TEST}.txt
)
set_tests_properties(
    integration_${INTEGRATION_TEST}_compare_to_ref
    PROPERTIES DEPENDS integration_${INTEGRATION_TEST}_save_output
)

add_dependencies(integration integration_${INTEGRATION_TEST})

set(INTEGRATION_TEST KeyMappingManager)

find_program(LIBXML2_XMLLINT_EXECUTABLE xmllint)
find_program(SED_EXECUTABLE sed)

if(LIBXML2_XMLLINT_EXECUTABLE AND SED_EXECUTABLE)
    find_package(Filesystem COMPONENTS Final Experimental REQUIRED)
    find_package(Qt5 COMPONENTS Core Widgets OpenGL Xml REQUIRED)

    set(Qt5_LIBRARIES Qt5::Core Qt5::Widgets Qt5::OpenGL Qt5::Xml)

    # KeyMappingManager saveConfiguration tests
    add_executable(integration_${INTEGRATION_TEST} ${INTEGRATION_TEST}/main.cpp)
    target_link_libraries(
        integration_${INTEGRATION_TEST} PUBLIC Core Gui PRIVATE std::filesystem ${Qt5_LIBRARIES}
    )
    target_compile_definitions(
        integration_${INTEGRATION_TEST}
        PRIVATE -DCXX_FILESYSTEM_HAVE_FS
                -DCXX_FILESYSTEM_IS_EXPERIMENTAL=$<BOOL:${CXX_FILESYSTEM_IS_EXPERIMENTAL}>
                -DCXX_FILESYSTEM_NAMESPACE=${CXX_FILESYSTEM_NAMESPACE}
    )

    add_custom_target(integration_${INTEGRATION_TEST}_create_ref)
    add_test(
        NAME integration_${INTEGRATION_TEST}_save_output
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${INTEGRATION_TEST}/
        COMMAND
            bash -c
            "$<TARGET_FILE:integration_${INTEGRATION_TEST}>  ${CMAKE_CURRENT_BINARY_DIR} &&\
 ${LIBXML2_XMLLINT_EXECUTABLE} --c14n11  ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid.xml > ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid-out-lint.xml &&\
 ${LIBXML2_XMLLINT_EXECUTABLE} --c14n11  ${CMAKE_CURRENT_SOURCE_DIR}/data/inputs/keymapping-valid.xml > ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid-in-lint.xml &&\
 ${SED_EXECUTABLE} -i.orig '/<!--.*-->/d' ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid-in-lint.xml &&\
 ${SED_EXECUTABLE} -i.orig '/<!--.*-->/d' ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid-out-lint.xml &&\
 ${SED_EXECUTABLE} -i.orig '/<!--/,/-->/d' ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid-in-lint.xml &&\
 ${SED_EXECUTABLE} -i.orig '/<!--/,/-->/d' ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid-out-lint.xml; "
    )

    add_test(
        NAME integration_${INTEGRATION_TEST}_compare_to_ref
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${INTEGRATION_TEST}/
        COMMAND
        COMMAND
            ${CMAKE_COMMAND} -E compare_files
            ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid-out-lint.xml
            ${CMAKE_CURRENT_BINARY_DIR}/keymapping-valid-in-lint.xml
    )

    add_dependencies(integration integration_${INTEGRATION_TEST})
else()
    message(
        WARNING
            "Could not add integration test for KeyMappingManager: sed (${SED_EXECUTABLE}) or xmllint (${LIBXML2_XMLLINT_EXECUTABLE})"
    )
endif()

find_package(Filesystem COMPONENTS Final Experimental REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets OpenGL Xml REQUIRED)

macro(generate_includes _name)
    include(filelist${_name})
    string(TOLOWER ${_name} lowername)
    set(filelist "${lowername}_headers")
    set(tmp ${${filelist}})
    list(TRANSFORM tmp PREPEND "#include <${_name}/")
    list(TRANSFORM tmp APPEND ">\n")
    string(REPLACE ";" "" str_${lowername}_headers "${tmp}")
endmacro()

generate_includes(Core)
generate_includes(Engine)
generate_includes(Gui)
generate_includes(IO)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/includes.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/includes.cpp"
)

set(Qt5_LIBRARIES Qt5::Core Qt5::Widgets Qt5::OpenGL Qt5::Xml)
add_executable(integration_includes "${CMAKE_CURRENT_BINARY_DIR}/includes.cpp")
target_link_libraries(
    integration_includes PUBLIC Core Gui Engine PRIVATE std::filesystem ${Qt5_LIBRARIES}
)
target_compile_definitions(
    integration_includes
    PRIVATE -DCXX_FILESYSTEM_HAVE_FS
            -DCXX_FILESYSTEM_IS_EXPERIMENTAL=$<BOOL:${CXX_FILESYSTEM_IS_EXPERIMENTAL}>
            -DCXX_FILESYSTEM_NAMESPACE=${CXX_FILESYSTEM_NAMESPACE}
)

# there is no run here, the fact that it compiles is the test. maybe use try_compile, but harder to
# do ;)
add_dependencies(integration integration_includes)
