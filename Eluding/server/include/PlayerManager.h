#pragma once

#include "GameServer.h"
#include "Entities/Entity.h"
#include "AreaManager.h"
#include "EntityManager.h"
#include "NetworkManager.h"

namespace evades {

class GameServer;

class PlayerManager {
public:
    PlayerManager(GameServer* server);

    // Player-related functions
    void downPlayer(Client& client);
    void respawnPlayer(Client& client);
    void cursePlayer(Client& client);
    void updateDownedPlayers();
    void updateCursedPlayers();
    void teleportToFirstSafeZone(Client& client);
    void checkForPlayerSaving(const Client& activePlayer);
    void savePlayer(Client& client);
    void handleExpanderCollision(Client& client, Enemy* enemy);
    void handleResetPosition(const std::vector<uint8_t>& buffer, const std::string& clientAddress, int clientPort);
    
private:
    GameServer* m_server;
};

} // namespace evades 