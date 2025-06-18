#pragma once

#include "NormalEnemy.h"

namespace evades {

class ExpanderEnemy : public NormalEnemy {
public:
    ExpanderEnemy(float x, float y, float radius, float speed);
    virtual ~ExpanderEnemy() = default;

    static constexpr float RADIUS_INCREASE = 5.0f;
    static constexpr int MAX_HITS = 5;

protected:
    void updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) override;
};

} // namespace evades 