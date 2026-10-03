#include <filesystem>
#include <map>

#include <fbxsdk.h>
#include <utils/fbx-loader.h>

namespace eXngine::Utils
{
    namespace
    {
        // Exporters store absolute paths of the machine the file was made on: fall back to
        // the relative path, then to the file name next to the model.
        std::string FindTextureFile(FbxFileTexture *texture, const std::filesystem::path &directory)
        {
            const std::filesystem::path candidates[] = {
                texture->GetFileName(),
                directory / texture->GetRelativeFileName(),
                directory / std::filesystem::path(texture->GetFileName()).filename(),
            };

            for (const auto &candidate : candidates)
            {
                std::error_code ignored;

                if (!candidate.empty() && std::filesystem::is_regular_file(candidate, ignored))
                    return candidate.lexically_normal().generic_string();
            }

            return {};
        }

        void ReadMaterial(FbxSurfaceMaterial *material, const std::filesystem::path &directory, Assets::ModelPart &part)
        {
            if (material == EXN_NULL_HANDLE)
                return;

            const FbxProperty diffuse = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

            if (!diffuse.IsValid())
                return;

            const FbxDouble3 color = diffuse.Get<FbxDouble3>();
            part.DiffuseColor = eXvec3(static_cast<float>(color[0]), static_cast<float>(color[1]), static_cast<float>(color[2]));

            if (FbxFileTexture *texture = diffuse.GetSrcObject<FbxFileTexture>(0))
            {
                part.DiffuseTexture = FindTextureFile(texture, directory);

                // The texture carries the color; many exporters leave the factor at black.
                if (!part.DiffuseTexture.empty())
                    part.DiffuseColor = eXvec3(1.0f, 1.0f, 1.0f);
            }
        }

        void ReadMesh(FbxNode *node, const std::filesystem::path &directory, Assets::ModelData &data)
        {
            FbxMesh *mesh = node->GetMesh();

            if (mesh == EXN_NULL_HANDLE)
                return;

            // Node transform plus the geometric offset that applies to this node only.
            FbxAMatrix geometry;
            geometry.SetT(node->GetGeometricTranslation(FbxNode::eSourcePivot));
            geometry.SetR(node->GetGeometricRotation(FbxNode::eSourcePivot));
            geometry.SetS(node->GetGeometricScaling(FbxNode::eSourcePivot));

            const FbxAMatrix transform = node->EvaluateGlobalTransform() * geometry;
            FbxAMatrix normalTransform = transform;
            normalTransform.SetT(FbxVector4(0.0, 0.0, 0.0, 0.0));
            normalTransform = normalTransform.Inverse().Transpose();

            FbxStringList uvSets;
            mesh->GetUVSetNames(uvSets);
            const char *uvSet = uvSets.GetCount() > 0 ? uvSets[0].Buffer() : EXN_NULL_HANDLE;

            const FbxGeometryElementMaterial *materials = mesh->GetElementMaterial();
            const bool materialPerPolygon = materials != EXN_NULL_HANDLE && materials->GetMappingMode() == FbxGeometryElement::eByPolygon;
            const FbxVector4 *controlPoints = mesh->GetControlPoints();

            // One part per material of this node.
            std::map<int, size_t> parts;

            for (int polygon = 0; polygon < mesh->GetPolygonCount(); ++polygon)
            {
                const int material = materialPerPolygon ? materials->GetIndexArray().GetAt(polygon) : 0;
                auto [it, inserted] = parts.try_emplace(material, data.Parts.size());

                if (inserted)
                {
                    Assets::ModelPart &part = data.Parts.emplace_back();
                    FbxSurfaceMaterial *surface = node->GetMaterial(material);

                    part.Name = node->GetName();

                    if (surface != EXN_NULL_HANDLE)
                        part.Name += std::string(" (") + surface->GetName() + ")";

                    ReadMaterial(surface, directory, part);
                }

                Assets::ModelPart &part = data.Parts[it->second];
                const EXUINT32 first = static_cast<EXUINT32>(part.Vertices.size());
                const int size = mesh->GetPolygonSize(polygon);

                for (int corner = 0; corner < size; ++corner)
                {
                    const FbxVector4 position = transform.MultT(controlPoints[mesh->GetPolygonVertex(polygon, corner)]);

                    FbxVector4 normal(0.0, 0.0, 0.0, 0.0);
                    if (mesh->GetPolygonVertexNormal(polygon, corner, normal))
                        normal = normalTransform.MultT(normal);

                    FbxVector2 uv(0.0, 0.0);
                    bool unmapped = true;
                    if (uvSet != EXN_NULL_HANDLE)
                        mesh->GetPolygonVertexUV(polygon, corner, uvSet, uv, unmapped);

                    Vertex vertex;
                    vertex.coordinates = eXvec3(static_cast<float>(position[0]), static_cast<float>(position[1]), static_cast<float>(position[2]));
                    vertex.color = eXvec3(1.0f, 1.0f, 1.0f);
                    vertex.uv = eXvec2(static_cast<float>(uv[0]), static_cast<float>(uv[1]));

                    part.Vertices.push_back(vertex);
                    part.Normals.push_back(eXvec3(static_cast<float>(normal[0]), static_cast<float>(normal[1]), static_cast<float>(normal[2])));
                }

                // Polygons are already triangles unless triangulation failed: fan the rest.
                for (int corner = 2; corner < size; ++corner)
                {
                    part.Indices.push_back(first);
                    part.Indices.push_back(first + corner - 1);
                    part.Indices.push_back(first + corner);
                }
            }
        }

        void ReadNode(FbxNode *node, const std::filesystem::path &directory, Assets::ModelData &data)
        {
            ReadMesh(node, directory, data);

            for (int child = 0; child < node->GetChildCount(); ++child)
                ReadNode(node->GetChild(child), directory, data);
        }
    }

    bool LoadFbxModel(const std::string &path, Assets::ModelData &data, std::string &error)
    {
        // The manager owns every SDK object created below; destroying it frees them all.
        FbxManager *manager = FbxManager::Create();
        manager->SetIOSettings(FbxIOSettings::Create(manager, IOSROOT));

        FbxImporter *importer = FbxImporter::Create(manager, "");
        FbxScene *scene = FbxScene::Create(manager, "scene");

        const bool imported = importer->Initialize(path.c_str(), -1, manager->GetIOSettings()) && importer->Import(scene);

        if (!imported)
        {
            error = importer->GetStatus().GetErrorString();
            manager->Destroy();
            return false;
        }

        // Y-up, right-handed like the engine; quads and n-gons become triangles.
        FbxAxisSystem::OpenGL.ConvertScene(scene);
        FbxGeometryConverter(manager).Triangulate(scene, true);

        ReadNode(scene->GetRootNode(), std::filesystem::path(path).parent_path(), data);
        manager->Destroy();

        std::erase_if(data.Parts, [](const Assets::ModelPart &part)
                      { return part.Indices.empty(); });

        if (data.Parts.empty())
        {
            error = "the file has no meshes";
            return false;
        }

        return true;
    }
}
