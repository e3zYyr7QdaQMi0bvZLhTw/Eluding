#pragma once

#include "../../shared/include/game.h"
#include "../../shared/include/map.h"
#include <string>
#include <memory>
#include <random>
#include <chrono>

namespace evades {

class Entity {
public:
    Entity(float x, float y, float radius, float speed);
    virtual ~Entity() = default;

    virtual void update(float deltaTime, const std::shared_ptr<GameMap>& map);

    float getX() const { return m_position.x; }
    float getY() const { return m_position.y; }
    float getRadius() const { return m_radius; }
    const Vector2& getPosition() const { return m_position; }
    const Vector2& getVelocity() const { return m_velocity; }

    bool isCollidingWith(const Entity& other) const;

    virtual void serialize(std::vector<uint8_t>& buffer) const;

protected:
    Vector2 m_position;
    Vector2 m_velocity;
    float m_radius;
    float m_speed;

    virtual bool handleMapCollisions(const std::shared_ptr<GameMap>& map);

    virtual void reflectVelocity(const Vector2& normal);
};

class Enemy;
class NormalEnemy;
class CursedEnemy;
class WallEnemy;
class SlowingEnemy;
class ImmuneEnemy;
class ExpanderEnemy;
class SilenceEnemy;
class SniperEnemy;
class SniperBullet;
class DasherEnemy;

class Enemy : public Entity {
public:
    enum class Type {
        Normal,
        Cursed,
        Wall,
        Slowing,
        Immune,
        Wavering,
        Expander,
        Silence,
        Sniper,
        SniperBullet,
        Dasher,
        Homing
    };

    Enemy(float x, float y, float radius, float speed, Type type);
    virtual ~Enemy() = default;

    void update(float deltaTime, const std::shared_ptr<GameMap>& map) override;

    Type getType() const { return m_type; }

    bool isHarmless() const { return m_isHarmless; }
    void makeHarmless(float duration = 1.5f);
    float getHarmlessTimeRemaining() const;
    float getHarmlessProgress() const; 

    void serialize(std::vector<uint8_t>& buffer) const override;

    static std::unique_ptr<Enemy> createEnemy(Type type, float x, float y, float radius, float speed);

    static std::unique_ptr<Enemy> createWallEnemy(
        float zoneX, float zoneY, float zoneWidth, float zoneHeight, 
        float speed, float radius, int wallIndex, int totalCount,
        int initialSide = -1, bool moveClockwise = true);

    static std::unique_ptr<Enemy> createWaveringEnemy(
        float x, float y, float radius, float speed,
        float minSpeed, float maxSpeed, float speedChangeInterval = 0.5f);

    static std::unique_ptr<Enemy> createSniperBullet(
        float x, float y, float radius, float speed, float angle);

    static Type stringToEnemyType(const std::string& typeStr);

protected:
    Type m_type;
    bool m_isHarmless = false;
    std::chrono::steady_clock::time_point m_harmlessStartTime;
    float m_harmlessDuration = 0.0f;

    virtual void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) = 0;

    void updateHarmlessState();
};

Vector2 getRandomDirection(std::mt19937& random);

}