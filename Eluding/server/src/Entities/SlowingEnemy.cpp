#include "SlowingEnemy.h"
#include <cmath>

namespace evades {

SlowingEnemy::SlowingEnemy(float x, float y, float radius, float speed)
    : NormalEnemy(x, y, radius, speed) {
    m_type = Enemy::Type::Slowing; 
}

void SlowingEnemy::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {

    NormalEnemy::updateBehavior(deltaTime, map);
}

bool SlowingEnemy::isPlayerInAura(float playerX, float playerY) const {

    float dx = playerX - m_position.x;
    float dy = playerY - m_position.y;
    float distanceSquared = dx * dx + dy * dy;

    return distanceSquared <= (AURA_RADIUS * AURA_RADIUS);
}

} 
