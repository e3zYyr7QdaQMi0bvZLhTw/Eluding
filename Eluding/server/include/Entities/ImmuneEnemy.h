#pragma once

#include "Entity.h"

namespace evades {

class ImmuneEnemy : public Enemy {
public:
    ImmuneEnemy(float x, float y, float radius, float speed);
    virtual ~ImmuneEnemy() = default;

protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
};

} // namespace evades 