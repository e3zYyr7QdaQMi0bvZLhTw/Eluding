#pragma once

#include "GameServer.h"
#include "Entities/Entity.h"
#include "PlayerManager.h"

namespace evades {

class GameServer;

class EntityManager {
public:
    EntityManager(GameServer* server);

    // Entity-related functions
    void spawnEnemiesForArea(int areaIndex);
    void despawnEnemiesInArea(int areaIndex);
    void checkAndSpawnEnemies();
    void spawnEnemiesFromSpawner(const MapArea& area, const MapZone& zone, const Spawner& spawner);
    void updateEnemies(float deltaTime);
    void updateSniperEnemies(float deltaTime);
    void broadcastEnemyState();

private:
    GameServer* m_server;
};

} // namespace evades 