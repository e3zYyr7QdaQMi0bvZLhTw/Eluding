#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

#include "client_network.h"
#include <iostream>

namespace evades {

ClientNetwork::ClientNetwork(const std::string& serverAddress, int serverPort)
    : m_serverAddress(serverAddress),
      m_serverPort(serverPort),
      m_running(true),
      m_connected(false),
      m_clientId(0) {

    try {
        m_socket.setNonBlocking(true);
        m_lastServerResponseTime = std::chrono::steady_clock::now();
        m_lastPingTime = std::chrono::steady_clock::now();
        std::cout << "Network initialized" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Network error: " << e.what() << std::endl;
        m_running = false;
    }
}

bool ClientNetwork::connectToServer() {
    std::cout << "Connecting to server at " << m_serverAddress << ":" << m_serverPort << std::endl;

    ConnectPacket connectPacket;
    auto packetData = connectPacket.serialize();

    if (m_socket.sendTo(packetData, m_serverAddress, m_serverPort) > 0) {
        m_lastServerResponseTime = std::chrono::steady_clock::now();
        std::cout << "Connection request sent" << std::endl;
        return true;
    } else {
        std::cerr << "Failed to send connection request" << std::endl;
        return false;
    }
}

void ClientNetwork::processNetworkMessages() {
    std::vector<uint8_t> buffer;
    std::string fromAddress;
    int fromPort;

    while (m_socket.receiveFrom(buffer, fromAddress, fromPort) > 0) {
        if (buffer.empty()) {
            continue;
        }

        m_lastServerResponseTime = std::chrono::steady_clock::now();
        m_connected = true;

        MessageType messageType = static_cast<MessageType>(buffer[0]);

        switch (messageType) {
            case MessageType::GameState:
                handleGameState(buffer);
                break;

            case MessageType::Pong:

                break;

            case MessageType::MapData:
                handleMapData(buffer);
                break;

            case MessageType::PlayerTeleport:
                handlePlayerTeleport(buffer);
                break;

            case MessageType::EnemyUpdate:
                handleEnemyUpdate(buffer);
                break;

            case MessageType::PlayerDowned:
                handlePlayerDowned(buffer);
                break;

            case MessageType::PlayerCursed:
                handlePlayerCursed(buffer);
                break;

            default:
                std::cerr << "Unknown message type from server: " << static_cast<int>(messageType) << std::endl;
                break;
        }
    }
}

void ClientNetwork::handleGameState(const std::vector<uint8_t>& buffer) {
    GameStatePacket statePacket = GameStatePacket::deserialize(buffer);

    if (m_clientId == 0 && !statePacket.players.empty()) {

        m_clientId = statePacket.players[0].id;
    }

    if (m_gameStateCallback) {
        m_gameStateCallback(statePacket);
    }
}

void ClientNetwork::handleMapData(const std::vector<uint8_t>& buffer) {
    MapDataPacket mapPacket = MapDataPacket::deserialize(buffer);

    std::cout << "Received map data from server" << std::endl;

    if (m_mapDataCallback) {
        m_mapDataCallback(mapPacket);
    }
}

void ClientNetwork::handlePlayerTeleport(const std::vector<uint8_t>& buffer) {
    PlayerTeleportPacket teleport = PlayerTeleportPacket::deserialize(buffer);

    if (teleport.playerId == m_clientId) {
        std::cout << "Teleported to: " << teleport.x << ", " << teleport.y << std::endl;

        if (m_teleportCallback) {
            m_teleportCallback(teleport);
        }
    }
}

void ClientNetwork::handleEnemyUpdate(const std::vector<uint8_t>& buffer) {
    EnemyUpdatePacket packet = EnemyUpdatePacket::deserialize(buffer);

    if (m_enemyUpdateCallback) {
        m_enemyUpdateCallback(packet);
    }
}

void ClientNetwork::handlePlayerDowned(const std::vector<uint8_t>& buffer) {
    PlayerDownedPacket packet = PlayerDownedPacket::deserialize(buffer);

    if (m_playerDownedCallback) {
        m_playerDownedCallback(packet);
    }
}

void ClientNetwork::handlePlayerCursed(const std::vector<uint8_t>& buffer) {
    PlayerCursedPacket packet = PlayerCursedPacket::deserialize(buffer);

    if (m_playerCursedCallback) {
        m_playerCursedCallback(packet);
    }
}

void ClientNetwork::sendPlayerInput(const PlayerInput& input) {
    auto currentTime = std::chrono::steady_clock::now();
    static auto lastInputTime = currentTime;

    if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastInputTime).count() >= 1) {
        auto packetData = input.serialize();
        m_socket.sendTo(packetData, m_serverAddress, m_serverPort);
        lastInputTime = currentTime;
    }
}

void ClientNetwork::checkConnection() {
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - m_lastServerResponseTime).count();

    if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - m_lastPingTime).count() >= 1) {
        std::vector<uint8_t> pingData = { static_cast<uint8_t>(MessageType::Ping) };
        m_socket.sendTo(pingData, m_serverAddress, m_serverPort);
        m_lastPingTime = currentTime;
    }

    if (elapsed > 5) {
        if (m_connected) {
            std::cout << "Connection to server lost" << std::endl;
            m_connected = false;
        }

        if (elapsed % 5 == 0) {
            connectToServer();
        }
    }
}

} 