#pragma once

#include "Entity.h"

namespace evades {

class SniperBullet : public Enemy {
public:
    SniperBullet(float x, float y, float radius, float speed, float angle);
    virtual ~SniperBullet() = default;
    
    static constexpr float LIFETIME = 3.0f;
    
    bool shouldRemove() const { return m_timeLived >= LIFETIME; }
    
protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
    bool handleMapCollisions(const std::shared_ptr<GameMap>& map) override;
    
private:
    float m_timeLived = 0.0f;
    bool m_isImmune = true;
    bool m_isWeak = true;
};

} // namespace evades 