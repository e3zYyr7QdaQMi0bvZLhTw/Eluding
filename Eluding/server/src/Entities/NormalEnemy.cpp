#include "NormalEnemy.h"
#include <random>

namespace evades {

NormalEnemy::NormalEnemy(float x, float y, float radius, float speed)
    : Enemy(x, y, radius, speed, Enemy::Type::Normal) {
}

void NormalEnemy::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {
    if (m_velocity.x == 0 && m_velocity.y == 0) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        m_velocity = getRandomDirection(gen) * m_speed;
    }
}

} // namespace evades 
