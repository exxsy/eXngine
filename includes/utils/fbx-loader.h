#pragma once

#include <fbxsdk.h>
#include <vector>
#include <renderers/defines.h>
#include <utils/vertex.h>
#include <utils/mesh.h>

namespace eXngine::Utils
{
    class FbxLoader
    {
    public:
        explicit FbxLoader( const char* pathToFbxFile );

        [[nodiscard]] const std::vector<Mesh>& GetMeshes() const { return m_meshes; }

    private:
        std::vector<Mesh> m_meshes;
        Mesh ReadMesh( FbxNodeAttribute* pAttribute );

        /* Tab character ("\t") counter */
        int m_numTabs = 0;

        void PrintNode(FbxNode* pNode );
        void PrintTabs();
        void PrintAttribute(FbxNodeAttribute* pAttribute );
        FbxString GetAttributeTypeName( FbxNodeAttribute::EType type );
    };
}
