#include "ObjIO.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace geom
{
    SurfaceMesh loadOBJ(const std::filesystem::path &filename)
    {
        std::ifstream file(filename);

        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open OBJ file");
        }

        SurfaceMesh mesh;

        std::string line;

        while (std::getline(file, line))
        {
            std::istringstream stream(line);
            std::string type;

            stream >> type;

            if (type == "v")
            {
                double x, y, z;

                if (!(stream >> x >> y >> z))
                {
                    throw std::runtime_error("Invalid OBJ vertex");
                }

                mesh.addVertex({x, y, z});
            }
            else if (type == "f")
            {
                unsigned int v0, v1, v2;
                std::string extra;

                if (!(stream >> v0 >> v1 >> v2) || (stream >> extra))
                {
                    throw std::runtime_error("Expected triangular OBJ face");
                }

                if (v0 == 0 || v1 == 0 || v2 == 0)
                {
                    throw std::runtime_error("Invalid OBJ vertex index");
                }

                // OBJ indices start at 1; our mesh indices start at 0.
                mesh.addTriangle(v0 - 1, v1 - 1, v2 - 1);
            }
        }

        if (file.bad())
        {
            throw std::runtime_error("Error reading OBJ file");
        }

        return mesh;
    }
}