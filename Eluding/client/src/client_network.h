#pragma once

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

#include "../../shared/include/network.h"
#include "../../shared/include/protocol.h"
#include "../../shared/include/map.h"

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <iostream>

namespace evades {

class ClientNetwork {
public:

    ClientNetwork(const std::string& serverAddress, int serverPort);

    bool connectToServer();
    void processNetworkMessages();
    void sendPlayerInput(const PlayerInput& input);
    void checkConnection();

    void sendPacket(const std::vector<uint8_t>& data) {
        m_socket.sendTo(data, m_serverAddress, m_serverPort);
    }

    using GameStateCallback = std::function<void(const GameStatePacket&)>;
    using MapDataCallback = std::function<void(const MapDataPacket&)>;
    using TeleportCallback = std::function<void(const PlayerTeleportPacket&)>;
    using EnemyUpdateCallback = std::function<void(const EnemyUpdatePacket&)>;
    using PlayerDownedCallback = std::function<void(const PlayerDownedPacket&)>;
    using PlayerCursedCallback = std::function<void(const PlayerCursedPacket&)>;

    void setGameStateCallback(GameStateCallback callback) { m_gameStateCallback = callback; }
    void setMapDataCallback(MapDataCallback callback) { m_mapDataCallback = callback; }
    void setTeleportCallback(TeleportCallback callback) { m_teleportCallback = callback; }
    void setEnemyUpdateCallback(EnemyUpdateCallback callback) { m_enemyUpdateCallback = callback; }
    void setPlayerDownedCallback(PlayerDownedCallback callback) { m_playerDownedCallback = callback; }
    void setPlayerCursedCallback(PlayerCursedCallback callback) { m_playerCursedCallback = callback; }

    bool isConnected() const { return m_connected; }
    bool isRunning() const { return m_running; }
    uint32_t getClientId() const { return m_clientId; }

    void stop() { 
        if (m_connected) {
            std::cout << "Sending disconnect notification to server" << std::endl;
            DisconnectPacket packet;
            auto disconnectData = packet.serialize();

            m_socket.sendTo(disconnectData, m_serverAddress, m_serverPort);
            m_socket.sendTo(disconnectData, m_serverAddress, m_serverPort);
            m_socket.sendTo(disconnectData, m_serverAddress, m_serverPort);

            m_connected = false;
        }
        m_running = false; 
    }

private:

    std::string m_serverAddress;
    int m_serverPort;
    UDPSocket m_socket;
    std::chrono::steady_clock::time_point m_lastServerResponseTime;
    std::chrono::steady_clock::time_point m_lastPingTime;

    bool m_running;
    bool m_connected;
    uint32_t m_clientId;

    void handleGameState(const std::vector<uint8_t>& buffer);
    void handleMapData(const std::vector<uint8_t>& buffer);
    void handlePlayerTeleport(const std::vector<uint8_t>& buffer);
    void handleEnemyUpdate(const std::vector<uint8_t>& buffer);
    void handlePlayerDowned(const std::vector<uint8_t>& buffer);
    void handlePlayerCursed(const std::vector<uint8_t>& buffer);

    GameStateCallback m_gameStateCallback;
    MapDataCallback m_mapDataCallback;
    TeleportCallback m_teleportCallback;
    EnemyUpdateCallback m_enemyUpdateCallback;
    PlayerDownedCallback m_playerDownedCallback;
    PlayerCursedCallback m_playerCursedCallback;
};

} 