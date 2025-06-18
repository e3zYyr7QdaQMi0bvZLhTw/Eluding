#pragma once

#include "GameServer.h"

namespace evades {

class GameServer;

class AreaManager {
public:
    AreaManager(GameServer* server);

    // Area-related functions
    int findClosestArea(float x, float y);
    void loadMap(const std::string& mapFile);
    std::vector<std::pair<const MapArea*, const MapZone*>> getIntersectingZones(float x, float y, float radius) const;
    bool playerCircleIntersectsZone(float playerX, float playerY, float playerRadius, const MapArea* area, const MapZone* zone) const;

private:
    GameServer* m_server;
};

} // namespace evades 