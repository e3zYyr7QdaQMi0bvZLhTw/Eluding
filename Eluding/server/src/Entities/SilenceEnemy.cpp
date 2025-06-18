#include "SilenceEnemy.h"
#include <cmath>
#include <algorithm>

namespace evades {

SilenceEnemy::SilenceEnemy(float x, float y, float radius, float speed)
    : NormalEnemy(x, y, radius, speed) {
    m_type = Enemy::Type::Silence;
    m_auraSize = AURA_RADIUS;
    m_maxAuraSize = AURA_RADIUS;
    m_targetAuraSize = AURA_RADIUS;
}

void SilenceEnemy::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {

    NormalEnemy::updateBehavior(deltaTime, map);

    if (m_hasSilencedPlayer) {
        if (!m_wasPlayerInAuraLastFrame) {

            m_playerInAuraTimer = 0.3f; 
        }
        m_wasPlayerInAuraLastFrame = true;
    } else {
        if (m_wasPlayerInAuraLastFrame) {

            m_playerInAuraTimer = 0.0f;
        }
        m_wasPlayerInAuraLastFrame = false;
    }

    bool playersInAura = m_hasSilencedPlayer;
    if (m_playerInAuraTimer > 0) {
        m_playerInAuraTimer -= deltaTime;
        if (m_playerInAuraTimer <= 0) {
            m_playerInAuraTimer = 0.0f;
        } else {

            playersInAura = false;
        }
    }

    updateAuraSize(deltaTime, playersInAura);

    m_hasSilencedPlayer = false;
}

bool SilenceEnemy::isPlayerInAura(float playerX, float playerY) const {

    float dx = playerX - m_position.x;
    float dy = playerY - m_position.y;
    float distanceSquared = dx * dx + dy * dy;

    return distanceSquared <= (m_auraSize * m_auraSize);
}

void SilenceEnemy::updateAuraSize(float deltaTime, bool playersInAura) {

    float timeFactor = deltaTime * 280.0f; 

    if (playersInAura) {

        m_hasSilencedPlayer = true;

        m_targetAuraSize = std::max(0.0f, m_auraSize - AURA_SHRINK_RATE * timeFactor);
    } else {

        m_targetAuraSize = std::min(m_maxAuraSize, m_auraSize + AURA_GROW_RATE * timeFactor);
    }

    float diff = m_targetAuraSize - m_auraSize;
    m_auraSize += diff * SMOOTHING_FACTOR;

    m_auraSize = std::max(0.0f, std::min(m_auraSize, m_maxAuraSize));
}

} 
