# ----------------------------------------------------
# ---------- DO NOT EDIT -----------------------------
# ----------------------------------------------------
# This file is autogenerate from a script:
# run form scripts directory
# ./generateFilelistForModule.sh IO
# ----------------------------------------------------
set( io_sources
    CameraLoader/CameraLoader.cpp
)

set( io_headers
    CameraLoader/CameraLoader.hpp
    RaIO.hpp
)

set( io_inlines
   
)

if(RADIUM_IO_DEPRECATED)
list(APPEND io_sources
    deprecated/OBJFileManager.cpp
    deprecated/OFFFileManager.cpp
)

list(APPEND io_headers
    deprecated/FileManager.hpp
    deprecated/OBJFileManager.hpp
    deprecated/OFFFileManager.hpp
)

list(APPEND io_inlines
    deprecated/FileManager.inl
)
endif(RADIUM_IO_DEPRECATED)

if( RADIUM_IO_ASSIMP )
list(APPEND io_sources
    AssimpLoader/AssimpAnimationDataLoader.cpp
    AssimpLoader/AssimpCameraDataLoader.cpp
    AssimpLoader/AssimpFileLoader.cpp
    AssimpLoader/AssimpGeometryDataLoader.cpp
    AssimpLoader/AssimpHandleDataLoader.cpp
    AssimpLoader/AssimpLightDataLoader.cpp
)

list(APPEND io_headers
    AssimpLoader/AssimpAnimationDataLoader.hpp
    AssimpLoader/AssimpCameraDataLoader.hpp
    AssimpLoader/AssimpFileLoader.hpp
    AssimpLoader/AssimpGeometryDataLoader.hpp
    AssimpLoader/AssimpHandleDataLoader.hpp
    AssimpLoader/AssimpLightDataLoader.hpp
    AssimpLoader/AssimpWrapper.hpp
)
endif( RADIUM_IO_ASSIMP )

if( RADIUM_IO_TINYPLY )
list(APPEND io_sources
    TinyPlyLoader/TinyPlyFileLoader.cpp
)

list(APPEND io_headers
    TinyPlyLoader/TinyPlyFileLoader.hpp
)
endif( RADIUM_IO_TINYPLY )

if( RADIUM_IO_VOLUMES )
list(APPEND io_sources
    VolumesLoader/pvmutils.cpp
    VolumesLoader/VolumeLoader.cpp
)

list(APPEND io_headers
    VolumesLoader/pvmutils.hpp
    VolumesLoader/VolumeLoader.hpp
)
endif( RADIUM_IO_VOLUMES )
