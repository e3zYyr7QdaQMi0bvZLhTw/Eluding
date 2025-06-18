#pragma once

#include "Entity.h"

namespace evades {

class DasherEnemy : public Enemy {
public:
    DasherEnemy(float x, float y, float radius, float speed);
    virtual ~DasherEnemy() = default;

    static constexpr float TIME_TO_PREPARE = 0.75f;        
    static constexpr float TIME_TO_DASH = 3.0f;            
    static constexpr float TIME_BETWEEN_DASHES = 0.75f;    
    static constexpr float PREPARE_SPEED_FACTOR = 0.2f;    
    static constexpr float BASE_SPEED_FACTOR = 0.2f;       

protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
    void reflectVelocity(const Vector2& normal) override;

private:
    float m_normalSpeed;          
    float m_prepareSpeed;         
    float m_dashSpeed;            
    float m_baseSpeed;            

    float m_timeDashing = 0.0f;   
    float m_timePreparing = 0.0f; 
    float m_timeSinceLastDash = 0.0f; 

    float m_oldAngle = 0.0f;      

    void computeSpeed();
};

} 