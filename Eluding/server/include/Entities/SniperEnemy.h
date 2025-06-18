#pragma once

#include "Entity.h"
#include "../../shared/include/protocol.h"
#include <chrono>

namespace evades {

class SniperEnemy : public Enemy {
public:
    SniperEnemy(float x, float y, float radius, float speed);
    virtual ~SniperEnemy() = default;

    static constexpr float RELEASE_TIME = 3.0f;
    static constexpr float DETECTION_DISTANCE = 600.0f / 32.0f * 32.0f;
    
    bool canShoot() const { return m_timeSinceLastShot >= RELEASE_TIME; }
    
    void resetShotTimer() { m_timeSinceLastShot = 0.0f; }
    
    float getSpeed() const { return m_speed; }
    
    const PlayerState* findClosestPlayer(const std::vector<PlayerState>& players, 
                                        const MapZone* activeZone, 
                                        const MapArea* area,
                                        const std::shared_ptr<GameMap>& map) const;

protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;

private:
    float m_timeSinceLastShot = 0.0f;
    float m_bulletRadius;
    float m_bulletSpeed;
};

} // namespace evades 