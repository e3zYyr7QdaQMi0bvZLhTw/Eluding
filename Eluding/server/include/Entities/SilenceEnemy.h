#pragma once

#include "NormalEnemy.h"

namespace evades {

class SilenceEnemy : public NormalEnemy {
public:
    SilenceEnemy(float x, float y, float radius, float speed);
    virtual ~SilenceEnemy() = default;
    
    static constexpr float AURA_RADIUS = 150.0f;
    static constexpr float AURA_SHRINK_RATE = 1.1f;
    static constexpr float AURA_GROW_RATE = 0.85f;
    static constexpr float SMOOTHING_FACTOR = 0.1f;
    
    bool isPlayerInAura(float playerX, float playerY) const;
    
    void updateAuraSize(float deltaTime, bool playersInAura);
    
    float getAuraSize() const { return m_auraSize; }
    
protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
    
private:
    float m_auraSize = AURA_RADIUS;
    float m_maxAuraSize = AURA_RADIUS;
    float m_targetAuraSize = AURA_RADIUS;
    bool m_hasSilencedPlayer = false;
    float m_playerInAuraTimer = 0.0f;
    bool m_wasPlayerInAuraLastFrame = false;
};

} // namespace evades 