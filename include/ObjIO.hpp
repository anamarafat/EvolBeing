#pragma once

#include "SurfaceMesh.hpp"
#include <filesystem>

namespace geom
{
    SurfaceMesh loadOBJ(const std::filesystem::path& filename);

}