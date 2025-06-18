#include "WaveringEnemy.h"
#include <random>
#include <cmath>

namespace evades {

WaveringEnemy::WaveringEnemy(float x, float y, float radius, float speed, 
                           float minSpeed, float maxSpeed, float speedChangeInterval)
    : Enemy(x, y, radius, speed, Enemy::Type::Wavering),
      m_minSpeed(minSpeed),
      m_maxSpeed(maxSpeed),
      m_speedChangeInterval(speedChangeInterval),
      m_clock(0) {

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> speedDist(minSpeed, maxSpeed);
    m_speed = speedDist(gen); 
    m_baseSpeed = m_speed;    

    if (m_speed == minSpeed) {
        m_speedDirection = true; 
    } else if (m_speed == maxSpeed) {
        m_speedDirection = false; 
    } else {
        std::uniform_int_distribution<int> dist(0, 1);
        m_speedDirection = dist(gen) == 1; 
    }

    if (m_velocity.x == 0 && m_velocity.y == 0) {
        m_velocity = getRandomDirection(gen) * m_speed;
    }

    computeSpeed();
}

std::unique_ptr<WaveringEnemy> WaveringEnemy::create(
    float x, float y, float radius, float speed,
    float minSpeed, float maxSpeed, float speedChangeInterval) {

    return std::make_unique<WaveringEnemy>(x, y, radius, speed, minSpeed, maxSpeed, speedChangeInterval);
}

float WaveringEnemy::getChangeProgress() const {
    const float transitionStart = m_speedChangeInterval - 0.25f; 

    if (m_clock > transitionStart) {
        return (m_clock - transitionStart) / 0.25f; 
    }

    return 0.0f;
}

void WaveringEnemy::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {

    const float speedChangeFactor = 2.5f; 

    m_clock += deltaTime * speedChangeFactor;

    if (m_clock > m_speedChangeInterval) {
        if (m_speedDirection) {
            m_speed += 2.0f; 
        } else {
            m_speed -= 2.0f; 
        }

        if (m_speed <= m_minSpeed || m_speed >= m_maxSpeed) {
            m_speedDirection = !m_speedDirection;

            m_speed = std::max(m_minSpeed, std::min(m_speed, m_maxSpeed));
        }

        m_clock = 0;
        computeSpeed();
    }

    if (m_velocity.x == 0 && m_velocity.y == 0) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        m_velocity = getRandomDirection(gen) * m_speed;
    } else {

        Vector2 direction = m_velocity;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
            m_velocity = Vector2(direction.x * m_speed, direction.y * m_speed);
        }
    }
}

void WaveringEnemy::computeSpeed() {

    if (m_velocity.x != 0 || m_velocity.y != 0) {
        Vector2 direction = m_velocity;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;
            m_velocity = Vector2(direction.x * m_speed, direction.y * m_speed);
        }
    }
}

void WaveringEnemy::updateColor() {

}

} 
