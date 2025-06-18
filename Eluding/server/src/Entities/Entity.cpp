#include "Entity.h"
#include <cmath>
#include <random>
#include <iostream>
#include <limits>
#include "NormalEnemy.h"
#include "CursedEnemy.h"
#include "WallEnemy.h"
#include "SlowingEnemy.h"
#include "ImmuneEnemy.h"
#include "WaveringEnemy.h"
#include "ExpanderEnemy.h"
#include "SilenceEnemy.h"
#include "SniperEnemy.h"
#include "SniperBullet.h"
#include "DasherEnemy.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace evades {

Entity::Entity(float x, float y, float radius, float speed)
    : m_position(x, y), m_velocity(0, 0), m_radius(radius), m_speed(speed) {
}

void Entity::update(float deltaTime, const std::shared_ptr<GameMap>& map) {

    m_position.x += m_velocity.x * deltaTime;
    m_position.y += m_velocity.y * deltaTime;

    handleMapCollisions(map);
}

bool Entity::isCollidingWith(const Entity& other) const {

    float dx = m_position.x - other.m_position.x;
    float dy = m_position.y - other.m_position.y;
    float distanceSquared = dx * dx + dy * dy;

    float radiusSum = m_radius + other.m_radius;
    return distanceSquared <= (radiusSum * radiusSum);
}

bool Entity::handleMapCollisions(const std::shared_ptr<GameMap>& map) {
    if (!map) return false;

    bool collided = false;

    float newX = m_position.x;
    float newY = m_position.y;

    bool isEnemy = dynamic_cast<const Enemy*>(this) != nullptr;
    bool positionWasAdjusted = map->resolveCollision(newX, newY, m_radius, isEnemy);

    if (positionWasAdjusted) {

        float normalX = 0.0f;
        float normalY = 0.0f;

        if (std::abs(newX - m_position.x) > 0.01f) {
            normalX = (newX > m_position.x) ? 1.0f : -1.0f;
        }

        if (std::abs(newY - m_position.y) > 0.01f) {
            normalY = (newY > m_position.y) ? 1.0f : -1.0f;
        }

        m_position.x = newX;
        m_position.y = newY;

        if (normalX != 0.0f || normalY != 0.0f) {
            reflectVelocity(Vector2(normalX, normalY));
            collided = true;
        }
    }

    if (dynamic_cast<const Enemy*>(this) == nullptr) {

        const MapZone* zone = map->getZoneAt(m_position.x, m_position.y);
        if (zone) {
            if (zone->type == ZoneType::Safe || zone->type == ZoneType::Exit || zone->type == ZoneType::Teleport) {
                const MapArea* area = map->getAreaAt(m_position.x, m_position.y);
                if (area) {
                    for (const auto& z : area->zones) {
                        if (z.type == ZoneType::Active) {
                            float zoneLeft = area->x + z.x;
                            float zoneTop = area->y + z.y;
                            float zoneRight = zoneLeft + z.width;
                            float zoneBottom = zoneTop + z.height;

                            float closestX = std::max(zoneLeft, std::min(m_position.x, zoneRight));
                            float closestY = std::max(zoneTop, std::min(m_position.y, zoneBottom));

                            if (closestX != m_position.x || closestY != m_position.y) {
                                float dx = m_position.x - closestX;
                                float dy = m_position.y - closestY;
                                float dist = std::sqrt(dx * dx + dy * dy);

                                if (dist > 0) {
                                    reflectVelocity(Vector2(dx / dist, dy / dist));
                                    m_position.x = closestX + (dx / dist) * m_radius * 1.1f;
                                    m_position.y = closestY + (dy / dist) * m_radius * 1.1f;
                                    collided = true;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return collided;
}

void Entity::reflectVelocity(const Vector2& normal) {

    Vector2 normalizedNormal = normal;
    if (normal.x != 0.0f || normal.y != 0.0f) {
        float length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
        normalizedNormal.x = normal.x / length;
        normalizedNormal.y = normal.y / length;
    }

    float dotProduct = m_velocity.x * normalizedNormal.x + m_velocity.y * normalizedNormal.y;

    m_velocity.x = m_velocity.x - 2 * dotProduct * normalizedNormal.x;
    m_velocity.y = m_velocity.y - 2 * dotProduct * normalizedNormal.y;
}

void Entity::serialize(std::vector<uint8_t>& buffer) const {
    size_t offset = buffer.size();
    buffer.resize(offset + sizeof(float) * 3); 

    memcpy(&buffer[offset], &m_position.x, sizeof(float));
    offset += sizeof(float);

    memcpy(&buffer[offset], &m_position.y, sizeof(float));
    offset += sizeof(float);

    memcpy(&buffer[offset], &m_radius, sizeof(float));
}

Enemy::Enemy(float x, float y, float radius, float speed, Type type)
    : Entity(x, y, radius, speed), m_type(type) {
}

void Enemy::update(float deltaTime, const std::shared_ptr<GameMap>& map) {
    updateBehavior(deltaTime, map);
    Entity::update(deltaTime, map);
    updateHarmlessState();
}

void Enemy::makeHarmless(float duration) {
    m_isHarmless = true;
    m_harmlessStartTime = std::chrono::steady_clock::now();
    m_harmlessDuration = duration;
}

float Enemy::getHarmlessTimeRemaining() const {
    if (!m_isHarmless) {
        return 0.0f;
    }

    auto currentTime = std::chrono::steady_clock::now();
    float elapsedSeconds = std::chrono::duration<float>(currentTime - m_harmlessStartTime).count();
    float remaining = m_harmlessDuration - elapsedSeconds;

    return std::max(0.0f, remaining);
}

float Enemy::getHarmlessProgress() const {
    if (!m_isHarmless) {
        return 0.0f;
    }

    float remaining = getHarmlessTimeRemaining();
    if (remaining <= 0) {
        return 0.0f;
    }

    return remaining / m_harmlessDuration;
}

void Enemy::updateHarmlessState() {
    if (m_isHarmless && getHarmlessTimeRemaining() <= 0) {
        m_isHarmless = false;
    }
}

void Enemy::serialize(std::vector<uint8_t>& buffer) const {
    Entity::serialize(buffer);
    uint8_t type = static_cast<uint8_t>(m_type);
    buffer.push_back(type);

    buffer.push_back(m_isHarmless ? 1 : 0);

    if (m_isHarmless) {
        uint8_t progress = static_cast<uint8_t>(getHarmlessProgress() * 255);
        buffer.push_back(progress);
    }
}

std::unique_ptr<Enemy> Enemy::createEnemy(Type type, float x, float y, float radius, float speed) {
    switch (type) {
        case Type::Normal:
            return std::make_unique<NormalEnemy>(x, y, radius, speed);
        case Type::Cursed:
            return std::make_unique<CursedEnemy>(x, y, radius, speed);
        case Type::Wall:
            return std::make_unique<WallEnemy>(x, y, radius, speed);
        case Type::Slowing:
            return std::make_unique<SlowingEnemy>(x, y, radius, speed);
        case Type::Immune:
            return std::make_unique<ImmuneEnemy>(x, y, radius, speed);
        case Type::Wavering:
            return std::make_unique<WaveringEnemy>(x, y, radius, speed);
        case Type::Expander:
            return std::make_unique<ExpanderEnemy>(x, y, radius, speed);
        case Type::Silence:
            return std::make_unique<SilenceEnemy>(x, y, radius, speed);
        case Type::Sniper:
            return std::make_unique<SniperEnemy>(x, y, radius, speed);
        case Type::SniperBullet:
            return std::make_unique<SniperBullet>(x, y, radius, speed, 0.0f);
        case Type::Dasher:
            return std::make_unique<DasherEnemy>(x, y, radius, speed);
        default:
            return std::make_unique<NormalEnemy>(x, y, radius, speed);
    }
}

std::unique_ptr<Enemy> Enemy::createWallEnemy(
    float zoneX, float zoneY, float zoneWidth, float zoneHeight, 
    float speed, float radius, int wallIndex, int totalCount,
    int initialSide, bool moveClockwise) {

    return WallEnemy::create(
        zoneX, zoneY, zoneWidth, zoneHeight,
        speed, radius, wallIndex, totalCount,
        initialSide, moveClockwise);
}

Enemy::Type Enemy::stringToEnemyType(const std::string& typeStr) {
    if (typeStr == "normal") {
        return Type::Normal;
    }
    if (typeStr == "cursed") {
        return Type::Cursed;
    }
    if (typeStr == "wall") {
        return Type::Wall;
    }
    if (typeStr == "slowing") {
        return Type::Slowing;
    }
    if (typeStr == "immune") {
        return Type::Immune;
    }
    if (typeStr == "wavering") {
        return Type::Wavering;
    }
    if (typeStr == "expander") {
        return Type::Expander;
    }
    if (typeStr == "silence") {
        return Type::Silence;
    }
    if (typeStr == "sniper") {
        return Type::Sniper;
    }
    if (typeStr == "sniper_bullet") {
        return Type::SniperBullet;
    }
    if (typeStr == "dasher") {
        return Type::Dasher;
    }

    return Type::Normal;  
}

Vector2 getRandomDirection(std::mt19937& random) {
    std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
    float angle = angleDist(random);
    return Vector2(std::cos(angle), std::sin(angle));
}

std::unique_ptr<Enemy> Enemy::createWaveringEnemy(
    float x, float y, float radius, float speed,
    float minSpeed, float maxSpeed, float speedChangeInterval) {

    return WaveringEnemy::create(x, y, radius, speed, minSpeed, maxSpeed, speedChangeInterval);
}

std::unique_ptr<Enemy> Enemy::createSniperBullet(
    float x, float y, float radius, float speed, float angle) {

    return std::make_unique<SniperBullet>(x, y, radius, speed, angle);
}

} 
