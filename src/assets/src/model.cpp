#include <algorithm>
#include <cfloat>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <assets/model.h>

namespace eXngine::Assets
{
    namespace
    {
        struct ObjMaterial
        {
            eXvec3 DiffuseColor = eXvec3(1.0f, 1.0f, 1.0f);
            std::string DiffuseTexture;
        };

        // A face corner "v/vt/vn" as 0-based indices, -1 when missing.
        struct ObjCorner
        {
            EXINT32 Position = -1, UV = -1, Normal = -1;

            bool operator==(const ObjCorner &) const = default;
        };

        struct ObjCornerHash
        {
            size_t operator()(const ObjCorner &corner) const
            {
                size_t hash = std::hash<EXINT32>()(corner.Position);
                hash = hash * 31 + std::hash<EXINT32>()(corner.UV);
                return hash * 31 + std::hash<EXINT32>()(corner.Normal);
            }
        };

        // OBJ indices start at 1; negative ones count back from the last element.
        EXINT32 ResolveObjIndex(const std::string &token, size_t count)
        {
            if (token.empty())
                return -1;

            const EXINT32 index = std::stoi(token);
            const EXINT32 resolved = index < 0 ? static_cast<EXINT32>(count) + index : index - 1;

            return resolved >= 0 && resolved < static_cast<EXINT32>(count) ? resolved : -1;
        }

        // Texture statements may carry options (map_Kd -s 1 1 1 file.png): the file is last.
        // File names with spaces are kept whole when there are no options.
        std::string ReadMapFileName(std::istringstream &line)
        {
            std::string rest;
            std::getline(line, rest);

            const size_t first = rest.find_first_not_of(" \t");
            if (first == std::string::npos)
                return {};

            rest = rest.substr(first, rest.find_last_not_of(" \t\r") - first + 1);

            if (rest[0] != '-')
                return rest;

            const size_t last = rest.find_last_of(" \t");
            return last != std::string::npos ? rest.substr(last + 1) : rest;
        }

        std::string ResolveRelative(const std::filesystem::path &directory, const std::string &file)
        {
            std::filesystem::path path(file);

            if (path.is_relative())
                path = directory / path;

            return path.lexically_normal().generic_string();
        }

        void LoadObjMaterials(const std::filesystem::path &path, std::unordered_map<std::string, ObjMaterial> &materials)
        {
            std::ifstream file(path);

            if (!file.is_open())
            {
                EX_WARNING("OBJ: material library not found: %s", path.string().c_str());
                return;
            }

            ObjMaterial *current = EXN_NULL_HANDLE;
            std::string text;

            while (std::getline(file, text))
            {
                std::istringstream line(text);
                std::string keyword;
                line >> keyword;

                if (keyword == "newmtl")
                {
                    std::string name;
                    line >> name;
                    current = &materials[name];
                }
                else if (current == EXN_NULL_HANDLE)
                {
                    continue;
                }
                else if (keyword == "Kd")
                {
                    line >> current->DiffuseColor.x >> current->DiffuseColor.y >> current->DiffuseColor.z;
                }
                else if (keyword == "map_Kd")
                {
                    const std::string fileName = ReadMapFileName(line);

                    if (!fileName.empty())
                        current->DiffuseTexture = ResolveRelative(path.parent_path(), fileName);
                }
            }
        }
    }

    bool LoadObjModel(const std::string &path, ModelData &data, std::string &error)
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            error = "cannot open file";
            return false;
        }

        const std::filesystem::path directory = std::filesystem::path(path).parent_path();

        std::vector<eXvec3> positions, colors, normals;
        std::vector<eXvec2> uvs;
        std::unordered_map<std::string, ObjMaterial> materials;

        // The part being filled and the deduplicated corners of its vertices.
        std::string groupName = "default", materialName;
        ModelPart *part = EXN_NULL_HANDLE;
        std::unordered_map<ObjCorner, EXUINT32, ObjCornerHash> corners;

        // A new part starts with the first face after an o/g/usemtl statement.
        bool partChanged = true;

        std::string text;
        std::vector<EXUINT32> polygon;

        while (std::getline(file, text))
        {
            std::istringstream line(text);
            std::string keyword;
            line >> keyword;

            if (keyword == "v")
            {
                eXvec3 position, color(1.0f, 1.0f, 1.0f);
                line >> position.x >> position.y >> position.z;

                // Optional vertex colors: "v x y z r g b".
                if (!(line >> color.x >> color.y >> color.z))
                    color = eXvec3(1.0f, 1.0f, 1.0f);

                positions.push_back(position);
                colors.push_back(color);
            }
            else if (keyword == "vt")
            {
                eXvec2 uv(0.0f, 0.0f);
                line >> uv.x >> uv.y;
                uvs.push_back(uv);
            }
            else if (keyword == "vn")
            {
                eXvec3 normal;
                line >> normal.x >> normal.y >> normal.z;
                normals.push_back(normal);
            }
            else if (keyword == "o" || keyword == "g")
            {
                std::getline(line >> std::ws, groupName);
                partChanged = true;
            }
            else if (keyword == "usemtl")
            {
                line >> materialName;
                partChanged = true;
            }
            else if (keyword == "mtllib")
            {
                std::string library;
                std::getline(line >> std::ws, library);
                LoadObjMaterials(directory / library, materials);
            }
            else if (keyword == "f")
            {
                if (partChanged || part == EXN_NULL_HANDLE)
                {
                    // Reuse the last part when it is still empty (e.g. "g" followed by "usemtl").
                    if (part == EXN_NULL_HANDLE || !part->Indices.empty())
                        part = &data.Parts.emplace_back();

                    part->Name = materialName.empty() ? groupName : groupName + " (" + materialName + ")";

                    const auto material = materials.find(materialName);
                    part->DiffuseColor = material != materials.end() ? material->second.DiffuseColor : eXvec3(1.0f, 1.0f, 1.0f);
                    part->DiffuseTexture = material != materials.end() ? material->second.DiffuseTexture : std::string();

                    corners.clear();
                    partChanged = false;
                }

                polygon.clear();
                std::string token;

                while (line >> token)
                {
                    // "v", "v/vt", "v//vn" or "v/vt/vn"
                    const size_t slash1 = token.find('/');
                    const size_t slash2 = slash1 != std::string::npos ? token.find('/', slash1 + 1) : std::string::npos;

                    ObjCorner corner;
                    corner.Position = ResolveObjIndex(token.substr(0, slash1), positions.size());

                    if (slash1 != std::string::npos)
                        corner.UV = ResolveObjIndex(token.substr(slash1 + 1, slash2 - slash1 - 1), uvs.size());
                    if (slash2 != std::string::npos)
                        corner.Normal = ResolveObjIndex(token.substr(slash2 + 1), normals.size());

                    if (corner.Position < 0)
                    {
                        error = "invalid face index in line: " + text;
                        return false;
                    }

                    const auto [it, inserted] = corners.try_emplace(corner, static_cast<EXUINT32>(part->Vertices.size()));

                    if (inserted)
                    {
                        Utils::Vertex vertex;
                        vertex.coordinates = positions[corner.Position];
                        vertex.color = colors[corner.Position];
                        vertex.uv = corner.UV >= 0 ? uvs[corner.UV] : eXvec2(0.0f, 0.0f);

                        part->Vertices.push_back(vertex);

                        // Either every vertex of the part has a normal or none does.
                        if (corner.Normal >= 0)
                            part->Normals.push_back(normals[corner.Normal]);
                    }

                    polygon.push_back(it->second);
                }

                // Triangle fan: fine for the convex polygons OBJ exporters write.
                for (size_t i = 2; i < polygon.size(); ++i)
                {
                    part->Indices.push_back(polygon[0]);
                    part->Indices.push_back(polygon[i - 1]);
                    part->Indices.push_back(polygon[i]);
                }
            }
        }

        std::erase_if(data.Parts, [](const ModelPart &p)
                      { return p.Indices.empty(); });

        if (data.Parts.empty())
        {
            error = "the file has no faces";
            return false;
        }

        return true;
    }

    void ProcessModel(ModelData &data, const ModelImportSettings &settings)
    {
        for (auto &part : data.Parts)
        {
            if (part.Normals.size() != part.Vertices.size())
                part.Normals.assign(part.Vertices.size(), eXvec3(0.0f, 0.0f, 0.0f));

            if (settings.SwapYZ)
            {
                for (auto &vertex : part.Vertices)
                    vertex.coordinates = eXvec3(vertex.coordinates.x, vertex.coordinates.z, -vertex.coordinates.y);

                for (auto &normal : part.Normals)
                    normal = eXvec3(normal.x, normal.z, -normal.y);
            }

            // Missing normals: average the normals of the faces around each vertex.
            const bool hasNormals = std::any_of(part.Normals.begin(), part.Normals.end(), [](const eXvec3 &n)
                                                { return EXMATH::dot(EXMATH::vec3(n), EXMATH::vec3(n)) > 0.0f; });

            if (!hasNormals)
            {
                for (size_t i = 0; i + 2 < part.Indices.size(); i += 3)
                {
                    const EXMATH::vec3 a = part.Vertices[part.Indices[i]].coordinates;
                    const EXMATH::vec3 b = part.Vertices[part.Indices[i + 1]].coordinates;
                    const EXMATH::vec3 c = part.Vertices[part.Indices[i + 2]].coordinates;
                    const EXMATH::vec3 face = EXMATH::cross(b - a, c - a);

                    for (size_t k = 0; k < 3; ++k)
                        part.Normals[part.Indices[i + k]] = EXMATH::vec3(part.Normals[part.Indices[i + k]]) + face;
                }
            }

            for (auto &normal : part.Normals)
            {
                const EXMATH::vec3 n = normal;
                normal = EXMATH::dot(n, n) > 0.0f ? EXMATH::normalize(n) : EXMATH::vec3(0.0f, 1.0f, 0.0f);
            }
        }

        // Bounding box of the whole model, so its parts stay where they are relative to each other.
        EXMATH::vec3 min(FLT_MAX), max(-FLT_MAX);

        for (const auto &part : data.Parts)
        {
            for (const auto &vertex : part.Vertices)
            {
                min = EXMATH::min(min, EXMATH::vec3(vertex.coordinates));
                max = EXMATH::max(max, EXMATH::vec3(vertex.coordinates));
            }
        }

        const EXMATH::vec3 size = max - min;
        const EXFLOAT largest = std::max({size.x, size.y, size.z});
        const EXMATH::vec3 center = settings.CenterPivot ? (min + max) * 0.5f : EXMATH::vec3(0.0f);
        const EXFLOAT scale = settings.Scale * (settings.FitToUnitSize && largest > 0.0f ? 1.0f / largest : 1.0f);
        const EXMATH::vec3 light = EXMATH::normalize(EXMATH::vec3(settings.LightDirection));

        for (auto &part : data.Parts)
        {
            for (size_t i = 0; i < part.Vertices.size(); ++i)
            {
                Utils::Vertex &vertex = part.Vertices[i];

                vertex.coordinates = (EXMATH::vec3(vertex.coordinates) - center) * scale;

                if (settings.FlipV)
                    vertex.uv.y = 1.0f - vertex.uv.y;

                EXMATH::vec3 color = EXMATH::vec3(vertex.color) * EXMATH::vec3(part.DiffuseColor);

                if (settings.BakeLighting)
                {
                    const EXFLOAT diffuse = std::max(EXMATH::dot(EXMATH::vec3(part.Normals[i]), light), 0.0f);
                    color *= settings.Ambient + (1.0f - settings.Ambient) * diffuse;
                }

                vertex.color = color;
            }
        }
    }
}
