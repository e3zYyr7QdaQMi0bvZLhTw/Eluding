#pragma once

#include "Entity.h"

namespace evades {

class NormalEnemy : public Enemy {
public:
    NormalEnemy(float x, float y, float radius, float speed);
    virtual ~NormalEnemy() = default;

protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
};

} // namespace evades 