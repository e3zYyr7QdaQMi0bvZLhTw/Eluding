#pragma once

#include "Entity.h"
#include <chrono>

namespace evades {

class WaveringEnemy : public Enemy {
public:
    WaveringEnemy(float x, float y, float radius, float speed, 
                 float minSpeed = 3.0f, float maxSpeed = 15.0f, float speedChangeInterval = 0.5f);
    virtual ~WaveringEnemy() = default;
    
    static std::unique_ptr<WaveringEnemy> create(
        float x, float y, float radius, float speed,
        float minSpeed, float maxSpeed, float speedChangeInterval = 0.5f);
    
    float getCurrentSpeed() const { return m_speed; }
    float getMinSpeed() const { return m_minSpeed; }
    float getMaxSpeed() const { return m_maxSpeed; }
    bool isSpeedIncreasing() const { return m_speedDirection; }
    float getChangeProgress() const;

protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
    
private:
    void updateColor();
    void computeSpeed();
    
    bool m_speedDirection;
    float m_minSpeed;
    float m_maxSpeed;
    float m_speedChangeInterval;
    float m_clock;
    float m_baseSpeed;
};

} // namespace evades 