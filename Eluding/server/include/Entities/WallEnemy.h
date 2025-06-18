#pragma once

#include "Entity.h"

namespace evades {

class WallEnemy : public Enemy {
public:
    enum class WallDirection {
        Up = 0,
        Right = 1,
        Down = 2,
        Left = 3
    };

    WallEnemy(float x, float y, float radius, float speed);
    virtual ~WallEnemy() = default;

    static std::unique_ptr<WallEnemy> create(
        float zoneX, float zoneY, float zoneWidth, float zoneHeight, 
        float speed, float radius, int wallIndex, int totalCount,
        int initialSide = -1, bool moveClockwise = true);

protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
    
private:
    WallDirection m_wallDirection{WallDirection::Right};
    bool m_moveClockwise{true};

    static WallDirection rotateDirection(WallDirection direction, bool clockwise);
    void setVelocityFromWallDirection();
};

} // namespace evades 