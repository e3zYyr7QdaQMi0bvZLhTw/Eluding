#pragma once

#include "GameServer.h"
#include "PlayerManager.h"

namespace evades {

class GameServer;

class NetworkManager {
public:
    NetworkManager(GameServer* server);

    // Network-related functions
    void processNetworkMessages();
    void handlePlayerConnect(const std::string& clientAddress, int clientPort);
    void handlePlayerDisconnect(const std::string& clientAddress, int clientPort);
    void handlePlayerInput(const std::vector<uint8_t>& buffer, const std::string& clientAddress, int clientPort);
    void broadcastGameState();
    void cleanupDisconnectedClients();
    
private:
    GameServer* m_server;
};

} // namespace evades 