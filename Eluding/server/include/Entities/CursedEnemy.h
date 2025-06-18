#pragma once

#include "Entity.h"

namespace evades {

class CursedEnemy : public Enemy {
public:
    CursedEnemy(float x, float y, float radius, float speed);
    virtual ~CursedEnemy() = default;

protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
};

} // namespace evades 