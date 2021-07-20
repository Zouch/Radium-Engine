# WARNING : if some paths are changed in the following variables, think to change it also in CMakeLists.txt
if(NOT DEFINED glm_DIR)
    set(glm_sub_DIR lib/cmake/glm/ CACHE INTERNAL "")
    set(glm_DIR ${CMAKE_INSTALL_PREFIX}/${glm_sub_DIR})
endif()

if(NOT DEFINED glbinding_DIR)
    set(glbinding_sub_DIR share/glbinding/ CACHE INTERNAL "")
    set(glbinding_DIR ${CMAKE_INSTALL_PREFIX}/${glbinding_sub_DIR})
endif()

if(NOT DEFINED globjects_DIR)
    set(globjects_sub_DIR share/globjects/ CACHE INTERNAL "")
    set(globjects_DIR ${CMAKE_INSTALL_PREFIX}/${globjects_sub_DIR})
endif()

if(NOT DEFINED Eigen3_DIR)
    set(Eigen3_sub_DIR share/eigen3/cmake/ CACHE INTERNAL "")
    set(Eigen3_DIR ${CMAKE_INSTALL_PREFIX}/${Eigen3_sub_DIR})
endif()
