#include <IO/AssimpLoader/AssimpGeometryDataLoader.hpp>

#include <assimp/mesh.h>

#include <assimp/scene.h>

#include <Core/Asset/GeometryData.hpp>
#include <Core/Utils/Log.hpp>

#include <Core/Asset/BlinnPhongMaterialData.hpp>
#include <IO/AssimpLoader/AssimpWrapper.hpp>

namespace Ra {
namespace IO {

using namespace Core::Utils; // log
using namespace Core::Asset; // log

AssimpGeometryDataLoader::AssimpGeometryDataLoader( const std::string& filepath,
                                                    const bool VERBOSE_MODE ) :
    DataLoader<GeometryData>( VERBOSE_MODE ), m_filepath( filepath ) {}

AssimpGeometryDataLoader::~AssimpGeometryDataLoader() = default;

void AssimpGeometryDataLoader::loadData( const aiScene* scene,
                                         std::vector<std::unique_ptr<GeometryData>>& data ) {
    data.clear();

    if ( scene == nullptr ) {
        LOG( logINFO ) << "AssimpGeometryDataLoader : scene is nullptr.";
        return;
    }

    if ( !sceneHasGeometry( scene ) ) {
        LOG( logINFO ) << "AssimpGeometryDataLoader : scene has no mesh.";
        return;
    }

    if ( m_verbose ) {
        LOG( logINFO ) << "File contains geometry.";
        LOG( logINFO ) << "Geometry Loading begin...";
    }

    loadGeometryData( scene, data );

    if ( m_verbose ) { LOG( logINFO ) << "Geometry Loading end.\n"; }
}

bool AssimpGeometryDataLoader::sceneHasGeometry( const aiScene* scene ) const {
    return ( sceneGeometrySize( scene ) != 0 );
}

uint AssimpGeometryDataLoader::sceneGeometrySize( const aiScene* scene ) const {
    uint mesh_size = 0;
    if ( scene->HasMeshes() ) {
        const uint size = scene->mNumMeshes;
        for ( uint i = 0; i < size; ++i ) {
            aiMesh* mesh = scene->mMeshes[i];
            if ( mesh->HasPositions() ) { ++mesh_size; }
        }
    }
    return mesh_size;
}

void AssimpGeometryDataLoader::loadMeshData( const aiMesh& mesh,
                                             GeometryData& data,
                                             std::set<std::string>& usedNames ) {
    fetchName( mesh, data, usedNames );
    fetchType( mesh, data );
    fetchVertices( mesh, data );
    if ( data.isLineMesh() ) { fetchEdges( mesh, data ); }
    else {
        fetchFaces( mesh, data );
    }

    if ( data.isTetraMesh() || data.isHexMesh() ) { fetchPolyhedron( mesh, data ); }

    if ( mesh.HasNormals() ) { fetchNormals( mesh, data ); }

    if ( mesh.HasTangentsAndBitangents() ) {
        fetchTangents( mesh, data );
        fetchBitangents( mesh, data );
    }

    // Radium V2 : allow to have several UV channels
    // use MATKEY_UVWSRC to know if any
    if ( mesh.GetNumUVChannels() > 1 ) {
        LOG( logWARNING )
            << "Assimp loader : several UV channels are set, Radium will use only the 1st";
    }
    if ( mesh.HasTextureCoords( 0 ) ) { fetchTextureCoordinates( mesh, data ); }

    /*
     if( mesh.HasVertexColors() ) {
     fetchColors( mesh, data );
     }
     */
}

void AssimpGeometryDataLoader::loadMeshFrame(
    const aiNode* node,
    const Core::Transform& parentFrame,
    const std::map<uint, size_t>& indexTable,
    std::vector<std::unique_ptr<GeometryData>>& data ) const {
    const uint child_size = node->mNumChildren;
    const uint mesh_size  = node->mNumMeshes;
    if ( ( child_size == 0 ) && ( mesh_size == 0 ) ) { return; }

    Core::Transform frame = parentFrame * assimpToCore( node->mTransformation );
    for ( uint i = 0; i < mesh_size; ++i ) {
        const uint ID = node->mMeshes[i];
        auto it       = indexTable.find( ID );
        if ( it != indexTable.end() ) { data[it->second]->setFrame( frame ); }
    }

    for ( uint i = 0; i < child_size; ++i ) {
        loadMeshFrame( node->mChildren[i], frame, indexTable, data );
    }
}

void AssimpGeometryDataLoader::fetchName( const aiMesh& mesh,
                                          GeometryData& data,
                                          std::set<std::string>& usedNames ) const {
    std::string name = assimpToCore( mesh.mName );
    while ( usedNames.find( name ) != usedNames.end() ) {
        name.append( "_" );
    }
    usedNames.insert( name );
    data.setName( name );
}

void AssimpGeometryDataLoader::fetchType( const aiMesh& mesh, GeometryData& data ) const {
    data.setType( GeometryData::UNKNOWN );
    uint face_type = 0;
    for ( uint i = 0; i < mesh.mNumFaces; ++i ) {
        face_type = std::max( face_type, mesh.mFaces[i].mNumIndices );
    }
    if ( face_type != 1 ) {
        switch ( face_type ) {
        case 0: {
            data.setType( GeometryData::POINT_CLOUD );
        } break;
        case 2: {
            data.setType( GeometryData::LINE_MESH );
        } break;
        case 3: {
            data.setType( GeometryData::TRI_MESH );
        } break;
        case 4: {
            data.setType( GeometryData::QUAD_MESH );
        } break;
        default: {
            data.setType( GeometryData::POLY_MESH );
        } break;
        }
    }
}
/** \todo have something like
template <typename I, typename TYPE, int DIM>
void fetchVectorData( size_t sz,
                      const I& input,
                      Core::VectorArray<Eigen::Matrix<TYPE, DIM, 1>>& output ) {
    const auto size = int( sz );
    output.resize( sz );

#pragma omp parallel for
    for ( int i = 0; i < size; ++i )
    {
        output[i] = assimpToCore( input[i] );
    }
}
*/
void AssimpGeometryDataLoader::fetchVertices( const aiMesh& mesh, GeometryData& data ) {
    const int size = mesh.mNumVertices;
    auto& vertex   = data.getVertices();
    vertex.resize( mesh.mNumVertices );
#pragma omp parallel for
    for ( int i = 0; i < size; ++i ) {
        vertex[i] = assimpToCore( mesh.mVertices[i] );
    }
    //    fetchVectorData( mesh.mNumVertices, mesh.mVertices, vertex );
}

void AssimpGeometryDataLoader::fetchEdges( const aiMesh& mesh, GeometryData& data ) const {
    const int size = mesh.mNumFaces;
    auto& edge     = data.getEdges();
    edge.resize( mesh.mNumFaces );
#pragma omp parallel for
    for ( int i = 0; i < size; ++i ) {
        edge[i] = assimpToCore( mesh.mFaces[i].mIndices, mesh.mFaces[i].mNumIndices ).cast<uint>();
    }
}

void AssimpGeometryDataLoader::fetchFaces( const aiMesh& mesh, GeometryData& data ) const {
    const int size = mesh.mNumFaces;
    auto& face     = data.getFaces();
    face.resize( mesh.mNumFaces );
#pragma omp parallel for
    for ( int i = 0; i < size; ++i ) {
        face[i] = assimpToCore( mesh.mFaces[i].mIndices, mesh.mFaces[i].mNumIndices ).cast<uint>();
    }
}

void AssimpGeometryDataLoader::fetchPolyhedron( const aiMesh& mesh, GeometryData& data ) const {
    CORE_UNUSED( mesh );
    CORE_UNUSED( data );
    // TO DO
}

void AssimpGeometryDataLoader::fetchNormals( const aiMesh& mesh, GeometryData& data ) const {
    const int size = mesh.mNumVertices;
    auto& normal   = data.getNormals();
    normal.resize( mesh.mNumVertices, Core::Vector3::Zero() );
#pragma omp parallel for
    for ( int i = 0; i < size; ++i ) {
        normal[i] = assimpToCore( mesh.mNormals[i] );
        normal[i].normalize();
    }
}

void AssimpGeometryDataLoader::fetchTangents( const aiMesh& mesh, GeometryData& data ) const {
    const int size = mesh.mNumVertices;
    auto& tangent  = data.getTangents();
    tangent.resize( mesh.mNumVertices, Core::Vector3::Zero() );
#pragma omp parallel for
    for ( int i = 0; i < int( size ); ++i ) {
        tangent[i] = assimpToCore( mesh.mTangents[i] );
    }
}

void AssimpGeometryDataLoader::fetchBitangents( const aiMesh& mesh, GeometryData& data ) const {
    const int size  = mesh.mNumVertices;
    auto& bitangent = data.getBiTangents();
    bitangent.resize( mesh.mNumVertices );
#pragma omp parallel for
    for ( int i = 0; i < size; ++i ) {
        bitangent[i] = assimpToCore( mesh.mBitangents[i] );
    }
}

void AssimpGeometryDataLoader::fetchTextureCoordinates( const aiMesh& mesh,
                                                        GeometryData& data ) const {
    const int size = mesh.mNumVertices;
    auto& texcoord = data.getTexCoords();
    texcoord.resize( mesh.mNumVertices );
#pragma omp parallel for
    for ( int i = 0; i < size; ++i ) {
        // Radium V2 : allow to have several UV channels
        texcoord.at( i ) = assimpToCore( mesh.mTextureCoords[0][i] );
    }
}

void AssimpGeometryDataLoader::fetchColors( const aiMesh& mesh, GeometryData& data ) const {
    // TO DO
    CORE_UNUSED( mesh );
    CORE_UNUSED( data );
}

void AssimpGeometryDataLoader::loadMaterial( const aiMaterial& material,
                                             GeometryData& data ) const {

    std::string matName;
    aiString assimpName;
    if ( AI_SUCCESS == material.Get( AI_MATKEY_NAME, assimpName ) ) {
        matName = assimpName.C_Str();
    }
    // Radium V2 : use AI_MATKEY_SHADING_MODEL to select the apropriate model
    // (http://assimp.sourceforge.net/lib_html/material_8h.html#a93e23e0201d6ed86fb4287e15218e4cf)
    auto blinnPhongMaterial = new BlinnPhongMaterialData( matName );
    if ( AI_DEFAULT_MATERIAL_NAME != matName ) {
        aiColor4D color;
        float shininess;
        float opacity;
        aiString name;

        if ( AI_SUCCESS == material.Get( AI_MATKEY_COLOR_DIFFUSE, color ) ) {
            blinnPhongMaterial->m_hasDiffuse = true;
            blinnPhongMaterial->m_diffuse    = assimpToCore( color );
        }

        if ( AI_SUCCESS == material.Get( AI_MATKEY_COLOR_SPECULAR, color ) ) {
            blinnPhongMaterial->m_hasSpecular = true;
            blinnPhongMaterial->m_specular    = assimpToCore( color );
        }

        if ( AI_SUCCESS == material.Get( AI_MATKEY_SHININESS, shininess ) ) {
            blinnPhongMaterial->m_hasShininess = true;
            // Assimp gives the Phong exponent, we use the Blinn-Phong exponent
            blinnPhongMaterial->m_shininess = shininess * 4;
        }

        if ( AI_SUCCESS == material.Get( AI_MATKEY_OPACITY, opacity ) ) {
            blinnPhongMaterial->m_hasOpacity = true;
            // NOTE(charly): Due to collada way of handling objects that have an alpha map, we must
            // ensure
            //               we do not have zeros in here.
            blinnPhongMaterial->m_opacity = opacity < 1e-5 ? 1 : opacity;
        }

        if ( AI_SUCCESS == material.Get( AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE, 0 ), name ) ) {
            blinnPhongMaterial->m_texDiffuse    = m_filepath + "/" + assimpToCore( name );
            blinnPhongMaterial->m_hasTexDiffuse = true;
        }

        if ( AI_SUCCESS == material.Get( AI_MATKEY_TEXTURE( aiTextureType_SPECULAR, 0 ), name ) ) {
            blinnPhongMaterial->m_texSpecular    = m_filepath + "/" + assimpToCore( name );
            blinnPhongMaterial->m_hasTexSpecular = true;
        }

        if ( AI_SUCCESS == material.Get( AI_MATKEY_TEXTURE( aiTextureType_SHININESS, 0 ), name ) ) {
            blinnPhongMaterial->m_texShininess    = m_filepath + "/" + assimpToCore( name );
            blinnPhongMaterial->m_hasTexShininess = true;
        }

        if ( AI_SUCCESS == material.Get( AI_MATKEY_TEXTURE( aiTextureType_NORMALS, 0 ), name ) ) {
            blinnPhongMaterial->m_texNormal    = m_filepath + "/" + assimpToCore( name );
            blinnPhongMaterial->m_hasTexNormal = true;
        }

        // Assimp loads objs bump maps as height maps, gj bro
        if ( AI_SUCCESS == material.Get( AI_MATKEY_TEXTURE( aiTextureType_HEIGHT, 0 ), name ) ) {
            blinnPhongMaterial->m_texNormal    = m_filepath + "/" + assimpToCore( name );
            blinnPhongMaterial->m_hasTexNormal = true;
        }

        if ( AI_SUCCESS == material.Get( AI_MATKEY_TEXTURE( aiTextureType_OPACITY, 0 ), name ) ) {
            blinnPhongMaterial->m_texOpacity    = m_filepath + "/" + assimpToCore( name );
            blinnPhongMaterial->m_hasTexOpacity = true;
        }
    }
    else {
        LOG( logINFO ) << "Found assimp default material " << matName;
    }
    data.setMaterial( blinnPhongMaterial );
}

void AssimpGeometryDataLoader::loadGeometryData(
    const aiScene* scene,
    std::vector<std::unique_ptr<GeometryData>>& data ) {
    const uint size = scene->mNumMeshes;
    std::map<uint, std::size_t> indexTable;
    std::set<std::string> usedNames;
    for ( uint i = 0; i < size; ++i ) {
        aiMesh* mesh = scene->mMeshes[i];
        if ( mesh->HasPositions() ) {
            auto geometry = new GeometryData();
            loadMeshData( *mesh, *geometry, usedNames );
            // This returns always true (see assimp documentation)
            if ( scene->HasMaterials() ) {
                const uint matID = mesh->mMaterialIndex;
                if ( matID < scene->mNumMaterials ) {
                    aiMaterial* material = scene->mMaterials[matID];
                    loadMaterial( *material, *geometry );
                }
            }
            data.push_back( std::unique_ptr<GeometryData>( geometry ) );
            indexTable[i] = data.size() - 1;

            if ( m_verbose ) { geometry->displayInfo(); }
        }
    }
    loadMeshFrame( scene->mRootNode, Core::Transform::Identity(), indexTable, data );
}

} // namespace IO
} // namespace Ra
