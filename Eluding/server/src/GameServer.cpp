#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#include "../include/GameServer.h"
#include "../include/AreaManager.h"
#include "../include/EntityManager.h"
#include "../include/PlayerManager.h"
#include "../include/NetworkManager.h"
#include "../include/Entities/SlowingEnemy.h"

namespace evades {

GameServer::GameServer(int port) : m_port(port), m_running(true), m_nextClientId(1), m_nextEnemyId(1), m_currentTick(0) {
    try {
        m_socket.bind(port);
        m_socket.setNonBlocking(true);
        std::cout << "Server started on port " << port << std::endl;
        std::cout << "Server running at 240 TPS" << std::endl;

        m_areaManager = std::make_unique<AreaManager>(this);
        m_entityManager = std::make_unique<EntityManager>(this);
        m_playerManager = std::make_unique<PlayerManager>(this);
        m_networkManager = std::make_unique<NetworkManager>(this);

        m_areaManager->loadMap("maps/first_map.json");

        if (m_map) {
            m_entityManager->spawnEnemiesForArea(0);
        }

        m_random.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        m_running = false;
    }
}

void GameServer::run() {
    auto lastUpdateTime = std::chrono::steady_clock::now();
    auto lastClientCleanupTime = lastUpdateTime;
    auto lastEnemySpawnTime = lastUpdateTime;

    const float targetFrameTime = 1.0f / 240.0f;

    while (m_running) {
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();

        m_networkManager->processNetworkMessages();

        if (deltaTime >= targetFrameTime) {
            updateGameState(deltaTime);
            lastUpdateTime = currentTime;
        }

        if (std::chrono::duration<float>(currentTime - lastClientCleanupTime).count() >= 5.0f) {
            m_networkManager->cleanupDisconnectedClients();
            lastClientCleanupTime = currentTime;
        }

        if (std::chrono::duration<float>(currentTime - lastEnemySpawnTime).count() >= 5.0f) {
            m_entityManager->checkAndSpawnEnemies();
            lastEnemySpawnTime = currentTime;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(100)); 
    }
}

void GameServer::stop() {
    m_running = false;
}

void GameServer::updateGameState(float deltaTime) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    for (auto& [id, client] : m_clients) {
        client.isSilenced = false;

        if (client.state.isDowned) {
            continue;
        }

        Vector2 movement;

        if (m_playerInputs.count(id)) {
            const auto& input = m_playerInputs[id];

            const float DEFAULT_SLIDE_FRICTION = 0.75f;
            const float BASE_TPS = 240.0f;

            float speed = DEFAULT_PLAYER_SPEED;
            float friction = 1.0f;
            float slideFriction = DEFAULT_SLIDE_FRICTION;


            client.isSlowed = false;

            for (const auto& [enemyId, enemy] : m_enemies) {
                SlowingEnemy* slowingEnemy = dynamic_cast<SlowingEnemy*>(enemy.get());
                if (slowingEnemy && slowingEnemy->isPlayerInAura(client.state.x, client.state.y)) {
                    client.isSlowed = true;
                    break;
                }
            }

            if (client.isSlowed) {
                speed *= SlowingEnemy::SLOW_FACTOR;
            }

            if (input.isShiftPressed) {
                speed *= 0.5f;
            }

            if (m_map) {
                const MapZone* zone = m_map->getZoneAt(client.state.x, client.state.y);
                if (zone) {
                    if (zone->type == ZoneType::Safe && zone->properties.minimumSpeed > 0) {
                        speed = std::max(speed, zone->properties.minimumSpeed);
                    }
                    friction = m_map->properties.friction;

                    if (client.state.isCursed && zone->type == ZoneType::Safe) {
                        client.state.isCursed = false;
                        client.state.cursedTimer = 0;

                        PlayerCursedPacket packet;
                        packet.playerId = client.id;
                        packet.isCursed = 0;
                        packet.remainingSeconds = 0;

                        m_socket.sendTo(packet.serialize(), client.address, client.port);
                        std::cout << "Player " << client.id << " entered safe zone and was cured from curse" << std::endl;
                    }

                    if (zone->type == ZoneType::Safe && client.expanderHits > 0 && client.state.radius > DEFAULT_PLAYER_RADIUS) {
                        client.state.radius = DEFAULT_PLAYER_RADIUS;
                        client.expanderHits = 0;
                        std::cout << "Player " << client.id << " entered safe zone, radius reset to normal" << std::endl;
                    }
                }
            }

            float frictionFactor = 1.0f - slideFriction;
            float slideX = client.prevMovementX * frictionFactor;
            float slideY = client.prevMovementY * frictionFactor;

            float baseDistance = (speed / BASE_TPS) * friction;
            float dX = 0.0f;
            float dY = 0.0f;
            bool hasInput = false;

            if (input.isMouseControlEnabled) {
                float mouseDistance = input.mouseDistance;
                if (mouseDistance > 0.001f) {
                    hasInput = true;
                    const float distanceFactor = mouseDistance;
                    float finalDistance = baseDistance * distanceFactor;

                    dX = finalDistance * input.mouseDirectionX;
                    dY = finalDistance * input.mouseDirectionY;
                }
            }
            else if (input.isJoystickControlEnabled) {
                float joystickDistance = input.joystickDistance;
                if (joystickDistance > 0.001f) {
                    hasInput = true;
                    const float distanceFactor = joystickDistance;
                    float finalDistance = baseDistance * distanceFactor;

                    dX = finalDistance * input.joystickDirectionX;
                    dY = finalDistance * input.joystickDirectionY;
                }
            }
            else {
                Vector2 direction(0.0f, 0.0f);

                if (input.moveUp) {
                    direction.y -= 1.0f;
                }
                if (input.moveDown) {
                    direction.y += 1.0f;
                }
                if (input.moveLeft) {
                    direction.x -= 1.0f;
                }
                if (input.moveRight) {
                    direction.x += 1.0f;
                }

                if (direction.x != 0.0f || direction.y != 0.0f) {
                    hasInput = true;
                    direction.normalize();

                    if (direction.x != 0.0f && direction.y != 0.0f) {
                        direction *= 1.414f;
                    }

                    dX = baseDistance * direction.x;
                    dY = baseDistance * direction.y;
                }
            }

            if (!hasInput) {
                dX = slideX;
                dY = slideY;
            } else {
                dX += slideX;
                dY += slideY;
            }

            if (std::fabs(dX) < 0.001f) dX = 0.0f;
            if (std::fabs(dY) < 0.001f) dY = 0.0f;

            movement.x = dX;
            movement.y = dY;

            client.prevMovementX = dX;
            client.prevMovementY = dY;
        }

        float newX = client.state.x + movement.x;
        float newY = client.state.y + movement.y;

        if (m_map) {
            const MapZone* currentZone = m_map->getZoneAt(client.state.x, client.state.y);

            if (!currentZone) {
                std::cout << "Warning: Player " << id << " is not in any zone before movement" << std::endl;
            }

            const MapZone* newPosZone = m_map->getZoneAt(newX, newY);

            if ((movement.x != 0.0f || movement.y != 0.0f) && !newPosZone) {
                std::cout << "Warning: New position (" << newX << "," << newY 
                          << ") is not in any zone" << std::endl;

                int closestArea = m_areaManager->findClosestArea(newX, newY);
                std::cout << "Closest area to new position: " << closestArea << std::endl;
            }

            if ((newPosZone && newPosZone->type != ZoneType::Blocked) || !newPosZone) {
                client.state.x = newX;
                client.state.y = newY;

                bool collided = m_map->resolveCollision(client.state.x, client.state.y, client.state.radius, false);

                if (collided) {
                    std::cout << "Player " << id << " collided, position adjusted to ("
                              << client.state.x << "," << client.state.y << ")" << std::endl;
                }

                const MapZone* finalZone = m_map->getZoneAt(client.state.x, client.state.y);
                bool isInExitZone = false;

                if (finalZone && (finalZone->type == ZoneType::Exit || finalZone->type == ZoneType::Teleport) && 
                    currentZone && currentZone->type != ZoneType::Exit && currentZone->type != ZoneType::Teleport) {
                    isInExitZone = true;
                } 
                else if (currentZone && currentZone->type != ZoneType::Exit && currentZone->type != ZoneType::Teleport) {
                    std::vector<std::pair<const MapArea*, const MapZone*>> intersectingZones = 
                        m_areaManager->getIntersectingZones(client.state.x, client.state.y, client.state.radius);

                    for (const auto& [area, zone] : intersectingZones) {
                        if (zone->type == ZoneType::Exit || zone->type == ZoneType::Teleport) {
                            finalZone = zone;
                            isInExitZone = true;
                            std::cout << "Player " << client.id << " is touching an exit zone with their radius" << std::endl;
                            break;
                        }
                    }
                }

                if (isInExitZone) {
                    float originalX = client.state.x;
                    float originalY = client.state.y;

                    float translateX = finalZone->translate.x;
                    float translateY = finalZone->translate.y;

                    client.state.x += translateX;
                    client.state.y += translateY;

                    const float safeOffset = client.state.radius * 1.1f;
                    if (translateX != 0) {
                        client.state.x += (translateX > 0) ? safeOffset : -safeOffset;
                    }
                    if (translateY != 0) {
                        client.state.y += (translateY > 0) ? safeOffset : -safeOffset;
                    }

                    if (translateX == 0 && translateY == 0) {
                        client.state.x += safeOffset;
                    }

                    int newArea = m_areaManager->findClosestArea(client.state.x, client.state.y);

                    if (newArea != client.currentArea) {
                        client.currentArea = newArea;
                        std::cout << "Player teleported from (" << originalX << "," << originalY << ") to (" 
                              << client.state.x << "," << client.state.y << ") in area: " << client.currentArea << std::endl;

                        if (m_areasWithEnemies.find(client.currentArea) == m_areasWithEnemies.end()) {
                            m_entityManager->spawnEnemiesForArea(client.currentArea);
                        }
                    } else {
                        std::cout << "Player teleported from (" << originalX << "," << originalY << ") to (" 
                              << client.state.x << "," << client.state.y << ") in same area: " << client.currentArea << std::endl;
                    }

                    if (m_playerInputs.count(client.id)) {
                        m_playerInputs[client.id] = PlayerInput();
                    }

                    PlayerTeleportPacket teleport;
                    teleport.playerId = client.id;
                    teleport.x = client.state.x;
                    teleport.y = client.state.y;

                    m_socket.sendTo(teleport.serialize(), client.address, client.port);
                }

                m_playerManager->checkForPlayerSaving(client);
            }
        } else {
            client.state.x = std::max(0.0f, std::min(newX, static_cast<float>(WORLD_WIDTH)));
            client.state.y = std::max(0.0f, std::min(newY, static_cast<float>(WORLD_HEIGHT)));

            m_playerManager->checkForPlayerSaving(client);
        }
    }

    m_entityManager->updateEnemies(deltaTime);
    m_playerManager->updateDownedPlayers();
    m_playerManager->updateCursedPlayers();

    m_networkManager->broadcastGameState();
    m_entityManager->updateSniperEnemies(deltaTime);
    m_entityManager->broadcastEnemyState();

    m_currentTick++;
}

} // namespace evades

int main(int argc, char** argv) {
    int port = evades::DEFAULT_PORT;

    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    try {
        evades::GameServer server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 