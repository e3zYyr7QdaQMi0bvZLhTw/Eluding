#pragma once

#include "../../shared/include/network.h"
#include "../../shared/include/protocol.h"
#include "../../shared/include/game.h"
#include "../../shared/include/map.h"

#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <random>
#include <algorithm> 
#include <filesystem>
#include <unordered_set>

namespace evades {

// Forward declarations
class Enemy;
class AreaManager;
class EntityManager;
class PlayerManager;
class NetworkManager;
struct GameMap;
struct MapArea;
struct MapZone;

struct Client {
    uint32_t id;
    PlayerState state;
    std::string address;
    int port;
    std::chrono::steady_clock::time_point lastPacketTime;
    int currentArea = 0; 
    std::chrono::steady_clock::time_point downedTime; 
    std::chrono::steady_clock::time_point cursedTime;
    bool isSlowed = false; 
    bool isSilenced = false; 
    int expanderHits = 0; 
    float prevMovementX = 0.0f; 
    float prevMovementY = 0.0f; 
};

class GameServer {
public:
    GameServer(int port);
    void run();
    void stop();

private:
    friend class AreaManager;
    friend class EntityManager;
    friend class PlayerManager;
    friend class NetworkManager;

    // Core server components
    void updateGameState(float deltaTime);
    void cleanupDisconnectedClients();

    int m_port;
    std::atomic<bool> m_running;
    UDPSocket m_socket;
    uint32_t m_nextClientId;
    uint32_t m_nextEnemyId; 
    uint32_t m_currentTick;

    std::mutex m_clientsMutex;
    std::unordered_map<uint32_t, Client> m_clients;
    std::unordered_map<uint32_t, std::unique_ptr<Enemy>> m_enemies; 
    std::unordered_map<uint32_t, PlayerInput> m_playerInputs;
    std::unordered_set<int> m_areasWithEnemies; 

    std::vector<uint8_t> m_enemySerializationBuffer;
    size_t m_serializeOffset = 0;

    std::mt19937 m_random;
    std::shared_ptr<GameMap> m_map;

    // Manager components
    std::unique_ptr<AreaManager> m_areaManager;
    std::unique_ptr<EntityManager> m_entityManager;
    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<NetworkManager> m_networkManager;
};

} // namespace evades 