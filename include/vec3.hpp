#pragma once

#include <cmath>

namespace geom 
{

    struct Vec3 
    {
        double x{};
        double y{};
        double z{};

        Vec3 operator-(const Vec3 &other) const noexcept 
        {
            return 
            {
                x - other.x,
                y - other.y,
                z - other.z
            };
        }

        // Vector addition
        Vec3 operator+(const Vec3 &other) const noexcept 
        {
            return 
            {
                x + other.x,
                y + other.y,
                z + other.z
            };
        }

        // Scalar multiplication: vector * scalar
        Vec3 operator*(double scalar) const noexcept 
        {
            return 
            {
                x * scalar,
                y * scalar,
                z * scalar
            };
        }

        // Scalar division: vector / scalar
        Vec3 operator/(double scalar) const noexcept 
        {
            return 
            {
                x / scalar,
                y / scalar,
                z / scalar
            };
        }

        // Dot product
        double dot(const Vec3& other) const noexcept 
        {
            return
            { 
                x * other.x
                + y * other.y
                + z * other.z
            };
        }

        // Cross product
        Vec3 cross(const Vec3 &other) const noexcept 
        {
            return 
            {
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            };
        }

        //squared magnitude
        double squaredNorm() const noexcept 
        {
            return dot(*this);
        }

        // Magnitude
        double norm() const noexcept 
        {
            return std::sqrt(squaredNorm());
        }

    };

}
