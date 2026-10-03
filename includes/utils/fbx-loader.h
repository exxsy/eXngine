#pragma once

#include <string>
#include <assets/model.h>

namespace eXngine::Utils
{
    // Autodesk FBX through the FBX SDK, which only the example links. An Assets::ModelLoader:
    //   assets->RegisterModelLoader(".fbx", eXngine::Utils::LoadFbxModel);
    // Every mesh node becomes one part per material, in world space, Y-up.
    bool LoadFbxModel(const std::string &path, Assets::ModelData &data, std::string &error);
}
