#include "ExpanderEnemy.h"
#include <random>

namespace evades {

ExpanderEnemy::ExpanderEnemy(float x, float y, float radius, float speed)
    : NormalEnemy(x, y, radius, speed) {
    m_type = Enemy::Type::Expander;
}

void ExpanderEnemy::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {
    if (m_velocity.x == 0 && m_velocity.y == 0) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        m_velocity = getRandomDirection(gen) * m_speed;
    }
}

} // namespace evades 
