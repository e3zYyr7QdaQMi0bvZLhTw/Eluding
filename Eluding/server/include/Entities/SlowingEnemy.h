#pragma once

#include "NormalEnemy.h"

namespace evades {

class SlowingEnemy : public NormalEnemy {
public:
    SlowingEnemy(float x, float y, float radius, float speed);
    virtual ~SlowingEnemy() = default;
    
    static constexpr float AURA_RADIUS = 150.0f;
    
    static constexpr float SLOW_FACTOR = 0.7f;
    
    bool isPlayerInAura(float playerX, float playerY) const;
    
protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
};

} // namespace evades 