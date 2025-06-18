#include "../include/NetworkManager.h"
#include "../include/PlayerManager.h"
#include "../include/Entities/SlowingEnemy.h"

namespace evades {

NetworkManager::NetworkManager(GameServer* server) : m_server(server) {
}

void NetworkManager::processNetworkMessages() {
    std::vector<uint8_t> buffer;
    std::string clientAddress;
    int clientPort;

    while (m_server->m_socket.receiveFrom(buffer, clientAddress, clientPort) > 0) {
        if (buffer.empty()) {
            continue;
        }

        MessageType messageType = static_cast<MessageType>(buffer[0]);
        switch (messageType) {
            case MessageType::PlayerConnect:
            {
                handlePlayerConnect(clientAddress, clientPort);
                break;
            }

            case MessageType::PlayerInput:
            {
                handlePlayerInput(buffer, clientAddress, clientPort);
                break;
            }

            case MessageType::Ping:
            {
                std::vector<uint8_t> pongData = { static_cast<uint8_t>(MessageType::Pong) };
                m_server->m_socket.sendTo(pongData, clientAddress, clientPort);
                break;
            }

            case MessageType::PlayerDisconnect:
            {
                std::cout << "Received disconnect notification from " << clientAddress << ":" << clientPort << std::endl;
                handlePlayerDisconnect(clientAddress, clientPort);
                break;
            }

            case MessageType::ResetPosition:
            {
                m_server->m_playerManager->handleResetPosition(buffer, clientAddress, clientPort);
                break;
            }

            default:
            {
                std::cout << "Unknown message type: " << static_cast<int>(messageType) << std::endl;
                break;
            }
        }
    }
}

void NetworkManager::handlePlayerConnect(const std::string& clientAddress, int clientPort) {
    std::lock_guard<std::mutex> lock(m_server->m_clientsMutex);

    for (const auto& [id, client] : m_server->m_clients) {
        if (client.address == clientAddress && client.port == clientPort) {
            std::cout << "Client already connected: " << clientAddress << ":" << clientPort << std::endl;
            return;
        }
    }

    uint32_t clientId = m_server->m_nextClientId++;
    Client newClient;
    newClient.id = clientId;
    newClient.address = clientAddress;
    newClient.port = clientPort;
    newClient.lastPacketTime = std::chrono::steady_clock::now();
    newClient.currentArea = 0; 

    newClient.state.id = clientId;

    if (m_server->m_map) {
        Vector2 spawnPoint = m_server->m_map->findSpawnPoint();
        newClient.state.x = spawnPoint.x;
        newClient.state.y = spawnPoint.y;

        if (m_server->m_areasWithEnemies.find(newClient.currentArea) == m_server->m_areasWithEnemies.end()) {
            m_server->m_entityManager->spawnEnemiesForArea(newClient.currentArea);
        }
    } else {
        newClient.state.x = m_server->m_random() % static_cast<int>(WORLD_WIDTH * 0.8f) + WORLD_WIDTH * 0.1f;
        newClient.state.y = m_server->m_random() % static_cast<int>(WORLD_HEIGHT * 0.8f) + WORLD_HEIGHT * 0.1f;
    }

    newClient.state.radius = DEFAULT_PLAYER_RADIUS;

    m_server->m_clients[clientId] = newClient;

    std::cout << "Client connected: " << clientAddress << ":" << clientPort 
              << " (ID: " << clientId << ")" << std::endl;

    if (m_server->m_map) {
        MapDataPacket mapPacket;
        mapPacket.mapJson = m_server->m_map->toJsonString();
        m_server->m_socket.sendTo(mapPacket.serialize(), clientAddress, clientPort);
    }

    GameStatePacket statePacket;
    statePacket.tick = m_server->m_currentTick;

    for (const auto& [id, client] : m_server->m_clients) {
        statePacket.players.push_back(client.state);
    }

    m_server->m_socket.sendTo(statePacket.serialize(), clientAddress, clientPort);
}

void NetworkManager::handlePlayerDisconnect(const std::string& clientAddress, int clientPort) {
    std::lock_guard<std::mutex> lock(m_server->m_clientsMutex);

    for (auto it = m_server->m_clients.begin(); it != m_server->m_clients.end(); ++it) {
        auto& client = it->second;
        if (client.address == clientAddress && client.port == clientPort) {
            m_server->m_playerInputs.erase(client.id);
            m_server->m_clients.erase(it);
            std::cout << "Client disconnected: " << clientAddress << ":" << clientPort << std::endl;
            return;
        }
    }
}

void NetworkManager::handlePlayerInput(const std::vector<uint8_t>& buffer, const std::string& clientAddress, int clientPort) {
    size_t offset = 1; 
    PlayerInput input = PlayerInput::deserialize(buffer, offset);

    std::lock_guard<std::mutex> lock(m_server->m_clientsMutex);

    for (auto& [id, client] : m_server->m_clients) {
        if (client.address == clientAddress && client.port == clientPort) {
            m_server->m_playerInputs[id] = input;
            client.lastPacketTime = std::chrono::steady_clock::now();
            return;
        }
    }
}

void NetworkManager::cleanupDisconnectedClients() {
    std::lock_guard<std::mutex> lock(m_server->m_clientsMutex);
    auto currentTime = std::chrono::steady_clock::now();

    for (auto it = m_server->m_clients.begin(); it != m_server->m_clients.end();) {
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - it->second.lastPacketTime).count() > 10) {
            std::cout << "Client timed out: " << it->second.address << ":" << it->second.port << std::endl;
            m_server->m_playerInputs.erase(it->first);
            it = m_server->m_clients.erase(it);
        } else {
            ++it;
        }
    }
}

void NetworkManager::broadcastGameState() {
    GameStatePacket statePacket;
    statePacket.tick = m_server->m_currentTick;

    for (const auto& [id, client] : m_server->m_clients) {
        statePacket.players.push_back(client.state);
    }

    auto serializedState = statePacket.serialize();

    for (const auto& [id, client] : m_server->m_clients) {
        m_server->m_socket.sendTo(serializedState, client.address, client.port);
    }
}

} // namespace evades 