if(NOT DEFINED Eigen3_DIR)
    set(Eigen3_sub_DIR share/eigen3/cmake/ CACHE INTERNAL "")
    set(Eigen3_DIR ${CMAKE_INSTALL_PREFIX}/${Eigen3_sub_DIR})
endif()

if(NOT DEFINED OpenMesh_DIR)
    set(OpenMesh_sub_DIR share/OpenMesh/cmake/ CACHE INTERNAL "")
    set(OpenMesh_DIR ${CMAKE_INSTALL_PREFIX}/${OpenMesh_sub_DIR})
endif()

if(NOT DEFINED cpplocate_DIR)
    set(cpplocate_sub_DIR share/cpplocate/ CACHE INTERNAL "")
    set(cpplocate_DIR ${CMAKE_INSTALL_PREFIX}/${cpplocate_sub_DIR})
endif()
