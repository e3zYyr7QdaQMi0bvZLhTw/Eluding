#include "SniperBullet.h"
#include <cmath>

namespace evades {

SniperBullet::SniperBullet(float x, float y, float radius, float speed, float angle)
    : Enemy(x, y, radius, speed, Enemy::Type::SniperBullet) {

    m_velocity.x = std::cos(angle) * speed;
    m_velocity.y = std::sin(angle) * speed;
}

void SniperBullet::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {

    m_timeLived += deltaTime;

}

bool SniperBullet::handleMapCollisions(const std::shared_ptr<GameMap>& map) {
    if (!map) return false;

    float newX = m_position.x;
    float newY = m_position.y;

    bool positionWasAdjusted = map->resolveCollision(newX, newY, m_radius, true);

    if (positionWasAdjusted) {

        m_timeLived = LIFETIME;
        return true;
    }

    return false;
}

} 
