#pragma once

#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>

namespace evades {

constexpr int GRID_SIZE = 32;        
constexpr int WORLD_WIDTH = 2000;    
constexpr int WORLD_HEIGHT = 2000;   
constexpr int CLIENT_WIDTH = 800;    
constexpr int CLIENT_HEIGHT = 600;   

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;
    Vector2(float x_, float y_) : x(x_), y(y_) {}

    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    Vector2 normalized() const {
        float len = length();
        if (len > 0.0f) {
            return Vector2(x / len, y / len);
        }
        return Vector2(0.0f, 0.0f);
    }

    void normalize() {
        float len = length();
        if (len > 0.0f) {
            x /= len;
            y /= len;
        }
    }

    static float distance(const Vector2& a, const Vector2& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    float dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }
};

struct AABB {
    float left, top, right, bottom;

    AABB() : left(0), top(0), right(0), bottom(0) {}

    AABB(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}

    static AABB fromPositionAndSize(float x, float y, float width, float height) {
        return AABB(x, y, x + width, y + height);
    }

    static AABB fromCircle(float x, float y, float radius) {
        return AABB(x - radius, y - radius, x + radius, y + radius);
    }

    bool intersects(const AABB& other) const {
        return !(right < other.left || left > other.right || bottom < other.top || top > other.bottom);
    }
};

struct Collision {
    static bool circleVsAABB(float circleX, float circleY, float radius, 
                           const AABB& aabb, float& correctedX, float& correctedY) {

        AABB circleAABB = AABB::fromCircle(circleX, circleY, radius);

        if (circleAABB.intersects(aabb)) {
            float closestX = (aabb.left > circleX) ? aabb.left : ((circleX > aabb.right) ? aabb.right : circleX);
            float closestY = (aabb.top > circleY) ? aabb.top : ((circleY > aabb.bottom) ? aabb.bottom : circleY);

            float distanceX = circleX - closestX;
            float distanceY = circleY - closestY;
            float distanceSquared = distanceX * distanceX + distanceY * distanceY;

            if (distanceSquared <= radius * radius) {
                if (std::abs(distanceX) > 0 || std::abs(distanceY) > 0) {
                    float distance = std::sqrt(distanceSquared);
                    float normalX = distanceX / distance;
                    float normalY = distanceY / distance;

                    correctedX = closestX + normalX * radius;
                    correctedY = closestY + normalY * radius;
                } else {
                    float leftDist = circleX - aabb.left;
                    float rightDist = aabb.right - circleX;
                    float topDist = circleY - aabb.top;
                    float bottomDist = aabb.bottom - circleY;

                    float minDist1 = (leftDist < rightDist) ? leftDist : rightDist;
                    float minDist2 = (topDist < bottomDist) ? topDist : bottomDist;
                    float minDist = (minDist1 < minDist2) ? minDist1 : minDist2;

                    if (minDist == leftDist) {
                        correctedX = aabb.left - radius;
                    } else if (minDist == rightDist) {
                        correctedX = aabb.right + radius;
                    } else {
                        correctedX = circleX;
                    }

                    if (minDist == topDist) {
                        correctedY = aabb.top - radius;
                    } else if (minDist == bottomDist) {
                        correctedY = aabb.bottom + radius;
                    } else {
                        correctedY = circleY;
                    }
                }
                return true;
            }
        }
        return false;
    }
};

} 