#pragma once

#include <functional>
#include <string>
#include <vector>

#include <eXngine.h>
#include <types/vector.h>
#include <utils/vertex.h>

namespace eXngine::Assets
{
    // One drawable piece of a model: a triangle list painted with a single texture.
    // Files are split into parts by object/group and by material.
    struct ModelPart
    {
        std::string Name;
        std::vector<Utils::Vertex> Vertices;
        // One per vertex. Empty when the file has none: ProcessModel computes them.
        std::vector<eXvec3> Normals;
        std::vector<EXUINT32> Indices;
        // Full path of the diffuse texture, empty when the part is untextured.
        std::string DiffuseTexture;
        eXvec3 DiffuseColor = eXvec3(1.0f, 1.0f, 1.0f);
    };

    // What a model file holds once it is parsed, before anything is sent to the GPU.
    struct ModelData
    {
        std::vector<ModelPart> Parts;

        EXSIZE GetVertexCount() const
        {
            EXSIZE count = 0;

            for (const auto &part : Parts)
                count += static_cast<EXSIZE>(part.Vertices.size());

            return count;
        }

        EXSIZE GetTriangleCount() const
        {
            EXSIZE count = 0;

            for (const auto &part : Parts)
                count += static_cast<EXSIZE>(part.Indices.size() / 3);

            return count;
        }
    };

    // Applied to every model by ProcessModel, whatever its file format.
    struct ModelImportSettings
    {
        // Rotates Z-up files (3ds Max, Blender exports without axis conversion) to Y-up.
        EXBOOL SwapYZ = false;
        // Moves the center of the bounding box to the origin.
        EXBOOL CenterPivot = true;
        // Scales the model so its largest side is 1 unit long, before Scale is applied.
        EXBOOL FitToUnitSize = true;
        EXFLOAT Scale = 1.0f;
        // OBJ and FBX count v from the bottom of the image, the renderer from the top.
        EXBOOL FlipV = true;
        // The default shaders have no lighting: bake a directional light into the vertex
        // colors so untextured models do not render as flat silhouettes.
        EXBOOL BakeLighting = true;
        eXvec3 LightDirection = eXvec3(-0.4f, 0.8f, 0.6f);
        EXFLOAT Ambient = 0.35f;
    };

    // Fills data from the file at path; returns false and describes the problem in error.
    // Loaders run on the asset manager's worker thread: they must not touch the renderer.
    using ModelLoader = std::function<bool(const std::string &path, ModelData &data, std::string &error)>;

    // Wavefront OBJ (+ MTL for diffuse colors and textures). Polygons are triangulated.
    EXNEXPORT bool LoadObjModel(const std::string &path, ModelData &data, std::string &error);

    // Applies settings to data and computes the normals of parts that have none.
    EXNEXPORT void ProcessModel(ModelData &data, const ModelImportSettings &settings);
}
