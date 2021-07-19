# Sources:
# - https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
# - https://medium.com/@sushantsha/load-a-different-library-at-runtime-than-the-one-you-linked-against-cmake-26b4234fd271

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
if(APPLE)
   SET(CMAKE_INSTALL_RPATH "@executable_path/../lib")
else()
   SET(CMAKE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib")
endif()
ENDIF("${isSystemDir}" STREQUAL "-1")
