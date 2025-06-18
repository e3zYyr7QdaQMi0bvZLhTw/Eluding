#include "DasherEnemy.h"
#include <random>
#include <cmath>

namespace evades {

DasherEnemy::DasherEnemy(float x, float y, float radius, float speed)
    : Enemy(x, y, radius, speed, Enemy::Type::Dasher),
      m_normalSpeed(speed),
      m_prepareSpeed(speed * PREPARE_SPEED_FACTOR),
      m_dashSpeed(speed),
      m_baseSpeed(speed * BASE_SPEED_FACTOR) {

    static std::random_device rd;
    static std::mt19937 gen(rd());

    Vector2 randDir = getRandomDirection(gen);
    m_velocity = randDir * (m_normalSpeed * BASE_SPEED_FACTOR);

    m_oldAngle = std::atan2(m_velocity.y, m_velocity.x);
}

void DasherEnemy::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {

    float currentAngle = std::atan2(m_velocity.y, m_velocity.x);
    if (std::abs(currentAngle - m_oldAngle) > 0.1f) {
        m_oldAngle = currentAngle;
    }

    if (m_timeSinceLastDash < TIME_BETWEEN_DASHES && m_timePreparing == 0.0f && m_timeDashing == 0.0f) {

        m_timeSinceLastDash += deltaTime;
    }
    else if (m_timePreparing > 0.0f) {

        m_timePreparing += deltaTime;
        if (m_timePreparing >= TIME_TO_PREPARE) {
            m_timePreparing = 0.0f;
            m_timeDashing = deltaTime;  
        }
    }
    else if (m_timeDashing > 0.0f) {

        m_timeDashing += deltaTime;
        if (m_timeDashing >= TIME_TO_DASH) {
            m_timeDashing = 0.0f;
            m_timeSinceLastDash = 0.0f;  
        }
    }
    else if (m_timeSinceLastDash >= TIME_BETWEEN_DASHES) {

        m_timeSinceLastDash = 0.0f;
        m_timePreparing = deltaTime;
    }

    computeSpeed();
}

void DasherEnemy::computeSpeed() {
    float currentSpeed;

    if (m_timeSinceLastDash < TIME_BETWEEN_DASHES && m_timePreparing == 0.0f && m_timeDashing == 0.0f) {

        currentSpeed = m_normalSpeed * BASE_SPEED_FACTOR;
    }
    else if (m_timePreparing > 0.0f) {

        float prepareProgress = m_timePreparing / TIME_TO_PREPARE;
        currentSpeed = m_normalSpeed * PREPARE_SPEED_FACTOR * (1.0f - prepareProgress);
    }
    else if (m_timeDashing > 0.0f) {

        float dashProgress = m_timeDashing / TIME_TO_DASH;
        currentSpeed = m_dashSpeed * (1.0f - dashProgress * 0.5f); 
    }
    else {

        currentSpeed = m_normalSpeed;
    }

    if (m_velocity.length() > 0.001f) {

        m_velocity = m_velocity.normalized() * currentSpeed;
    } else {

        m_velocity = Vector2(std::cos(m_oldAngle), std::sin(m_oldAngle)) * currentSpeed;
    }

    m_speed = currentSpeed;
}

void DasherEnemy::reflectVelocity(const Vector2& normal) {
    Entity::reflectVelocity(normal);

    m_oldAngle = std::atan2(m_velocity.y, m_velocity.x);
}

} 
