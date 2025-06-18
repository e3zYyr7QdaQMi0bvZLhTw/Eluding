#include "../include/PlayerManager.h"
#include "../include/AreaManager.h"
#include "../include/EntityManager.h"
#include "../include/Entities/ExpanderEnemy.h"
#include "../include/Entities/SilenceEnemy.h"

namespace evades {

PlayerManager::PlayerManager(GameServer* server) : m_server(server) {
}

void PlayerManager::downPlayer(Client& client) {
    if (!client.state.isDowned) {
        client.state.isDowned = true;
        client.state.downedTimer = 60; 
        client.downedTime = std::chrono::steady_clock::now();

        std::cout << "Player " << client.state.id << " downed for " << static_cast<int>(client.state.downedTimer) << " seconds" << std::endl;

        PlayerDownedPacket packet;
        packet.playerId = client.state.id;
        packet.isDown = 1;
        packet.remainingSeconds = client.state.downedTimer;

        m_server->m_socket.sendTo(packet.serialize(), client.address, client.port);
    }
}

void PlayerManager::updateDownedPlayers() {
    auto currentTime = std::chrono::steady_clock::now();

    for (auto& [clientId, client] : m_server->m_clients) {
        if (client.state.isDowned) {
            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - client.downedTime).count();

            int newRemainingTime = 60 - static_cast<int>(elapsedSeconds);

            if (newRemainingTime != client.state.downedTimer) {
                client.state.downedTimer = static_cast<uint8_t>(std::max(0, newRemainingTime));

                PlayerDownedPacket packet;
                packet.playerId = client.state.id;
                packet.isDown = 1;
                packet.remainingSeconds = client.state.downedTimer;

                m_server->m_socket.sendTo(packet.serialize(), client.address, client.port);
            }

            if (client.state.downedTimer <= 0) {
                respawnPlayer(client);
            }
        }
    }
}

void PlayerManager::respawnPlayer(Client& client) {
    std::cout << "Respawning player " << client.state.id << std::endl;

    client.state.isDowned = false;
    client.state.downedTimer = 0;
    client.expanderHits = 0; 
    client.state.radius = DEFAULT_PLAYER_RADIUS; 

    if (m_server->m_map) {
        Vector2 spawnPoint = m_server->m_map->findSpawnPoint();
        client.state.x = spawnPoint.x;
        client.state.y = spawnPoint.y;

        int newArea = m_server->m_areaManager->findClosestArea(client.state.x, client.state.y);

        if (newArea != client.currentArea) {
            client.currentArea = newArea;

            if (m_server->m_areasWithEnemies.find(client.currentArea) == m_server->m_areasWithEnemies.end()) {
                m_server->m_entityManager->spawnEnemiesForArea(client.currentArea);
            }
        }

        std::cout << "Player respawned at (" << client.state.x << ", " << client.state.y 
                  << ") in area: " << client.currentArea << std::endl;

        PlayerTeleportPacket teleport;
        teleport.playerId = client.state.id;
        teleport.x = client.state.x;
        teleport.y = client.state.y;

        m_server->m_socket.sendTo(teleport.serialize(), client.address, client.port);

        PlayerDownedPacket packet;
        packet.playerId = client.state.id;
        packet.isDown = 0;
        packet.remainingSeconds = 0;

        m_server->m_socket.sendTo(packet.serialize(), client.address, client.port);
    }
}

void PlayerManager::cursePlayer(Client& client) {
    if (!client.state.isCursed) {
        client.state.isCursed = true;
        client.state.cursedTimer = 1.5f; 
        client.cursedTime = std::chrono::steady_clock::now();

        std::cout << "Player " << client.state.id << " cursed for " 
                  << client.state.cursedTimer << " seconds" << std::endl;

        PlayerCursedPacket packet;
        packet.playerId = client.state.id;
        packet.isCursed = 1;
        packet.remainingSeconds = client.state.cursedTimer;

        m_server->m_socket.sendTo(packet.serialize(), client.address, client.port);
    }
}

void PlayerManager::updateCursedPlayers() {
    auto currentTime = std::chrono::steady_clock::now();

    for (auto& [clientId, client] : m_server->m_clients) {
        if (client.state.isCursed) {
            auto elapsedDuration = currentTime - client.cursedTime;
            float elapsedSeconds = std::chrono::duration<float>(elapsedDuration).count();
            float newRemainingTime = 1.5f - elapsedSeconds;

            if (newRemainingTime <= 0.0f) {
                client.state.isCursed = false;
                client.state.cursedTimer = 0.0f;

                if (!client.state.isDowned) {
                    client.state.isDowned = true;
                    client.state.downedTimer = 60;
                    client.downedTime = std::chrono::steady_clock::now();

                    std::cout << "Player " << client.state.id << " became downed after curse expired" << std::endl;

                    PlayerDownedPacket downedPacket;
                    downedPacket.playerId = client.state.id;
                    downedPacket.isDown = 1;
                    downedPacket.remainingSeconds = client.state.downedTimer;

                    m_server->m_socket.sendTo(downedPacket.serialize(), client.address, client.port);
                }

            } else {
                client.state.cursedTimer = newRemainingTime;

                PlayerCursedPacket cursedPacket;
                cursedPacket.playerId = client.state.id;
                cursedPacket.isCursed = 1;
                cursedPacket.remainingSeconds = client.state.cursedTimer;

                m_server->m_socket.sendTo(cursedPacket.serialize(), client.address, client.port);
            }
        }
    }
}

void PlayerManager::teleportToFirstSafeZone(Client& client) {
    std::cout << "Teleporting cursed player " << client.state.id << " to first safe zone" << std::endl;

    client.state.isCursed = false;
    client.state.cursedTimer = 0;
    client.expanderHits = 0; 
    client.state.radius = DEFAULT_PLAYER_RADIUS; 

    if (m_server->m_map && !m_server->m_map->areas.empty()) {
        Vector2 spawnPoint = m_server->m_map->findSpawnPoint();
        client.state.x = spawnPoint.x;
        client.state.y = spawnPoint.y;

        int newArea = m_server->m_areaManager->findClosestArea(client.state.x, client.state.y);

        if (newArea != client.currentArea) {
            client.currentArea = newArea;

            if (m_server->m_areasWithEnemies.find(client.currentArea) == m_server->m_areasWithEnemies.end()) {
                m_server->m_entityManager->spawnEnemiesForArea(client.currentArea);
            }
        }

        std::cout << "Player teleported to (" << client.state.x << ", " << client.state.y 
                  << ") in area: " << client.currentArea << std::endl;

        PlayerTeleportPacket teleport;
        teleport.playerId = client.state.id;
        teleport.x = client.state.x;
        teleport.y = client.state.y;

        m_server->m_socket.sendTo(teleport.serialize(), client.address, client.port);

        PlayerCursedPacket packet;
        packet.playerId = client.state.id;
        packet.isCursed = 0;
        packet.remainingSeconds = 0;

        m_server->m_socket.sendTo(packet.serialize(), client.address, client.port);
    }
}

void PlayerManager::checkForPlayerSaving(const Client& activePlayer) {
    if (activePlayer.state.isDowned) {
        return;
    }

    if (activePlayer.isSilenced) {
        return;
    }

    for (auto& [otherId, otherClient] : m_server->m_clients) {
        if (otherId == activePlayer.id) {
            continue;
        }

        if (!otherClient.state.isDowned) {
            continue;
        }

        bool downedPlayerInSilenceAura = otherClient.isSilenced; 

        if (!downedPlayerInSilenceAura) {
            for (const auto& [enemyId, enemy] : m_server->m_enemies) {
                if (enemy->getType() == Enemy::Type::Silence) {
                    SilenceEnemy* silenceEnemy = dynamic_cast<SilenceEnemy*>(enemy.get());
                    if (silenceEnemy && silenceEnemy->isPlayerInAura(
                            otherClient.state.x, otherClient.state.y)) {
                        downedPlayerInSilenceAura = true;
                        break;
                    }
                }
            }
        }

        if (downedPlayerInSilenceAura) {
            continue;
        }

        float dx = activePlayer.state.x - otherClient.state.x;
        float dy = activePlayer.state.y - otherClient.state.y;
        float distanceSquared = dx * dx + dy * dy;

        float radiusSum = activePlayer.state.radius + otherClient.state.radius;
        if (distanceSquared <= (radiusSum * radiusSum)) {
            savePlayer(otherClient);
        }
    }
}

void PlayerManager::savePlayer(Client& client) {
    std::cout << "Player " << client.state.id << " was saved by another player" << std::endl;

    client.state.isDowned = false;
    client.state.downedTimer = 0;
    client.expanderHits = 0; 
    client.state.radius = DEFAULT_PLAYER_RADIUS; 

    PlayerDownedPacket packet;
    packet.playerId = client.state.id;
    packet.isDown = 0;
    packet.remainingSeconds = 0;

    m_server->m_socket.sendTo(packet.serialize(), client.address, client.port);
}

void PlayerManager::handleExpanderCollision(Client& client, Enemy* enemy) {
    if (client.state.isDowned) {
        return;
    }

    enemy->makeHarmless(1.5f);

    client.expanderHits++;

    if (client.expanderHits >= ExpanderEnemy::MAX_HITS) {
        client.expanderHits = 0;
        downPlayer(client);
        std::cout << "Player " << client.state.id << " downed after " 
                  << ExpanderEnemy::MAX_HITS << " expander hits" << std::endl;
    } else {
        client.state.radius += ExpanderEnemy::RADIUS_INCREASE;
        std::cout << "Player " << client.state.id << " radius increased to " 
                  << client.state.radius << " after " << client.expanderHits 
                  << " expander hits" << std::endl;
    }
}

void PlayerManager::handleResetPosition(const std::vector<uint8_t>& buffer, const std::string& clientAddress, int clientPort) {
    ResetPositionPacket packet = ResetPositionPacket::deserialize(buffer);
    std::lock_guard<std::mutex> lock(m_server->m_clientsMutex);

    for (auto& [id, client] : m_server->m_clients) {
        if (client.address == clientAddress && client.port == clientPort && client.id == packet.playerId) {
            if (m_server->m_map) {
                Vector2 spawnPoint = m_server->m_map->findSpawnPoint();

                client.state.x = spawnPoint.x;
                client.state.y = spawnPoint.y;
                client.state.isDowned = false;
                client.state.downedTimer = 0;
                client.state.isCursed = false;
                client.state.cursedTimer = 0.0f;
                client.expanderHits = 0;
                client.state.radius = DEFAULT_PLAYER_RADIUS;

                std::cout << "Resetting player " << client.id << " to spawn point: (" 
                          << spawnPoint.x << "," << spawnPoint.y << ")" << std::endl;

                PlayerTeleportPacket teleport;
                teleport.playerId = client.id;
                teleport.x = spawnPoint.x;
                teleport.y = spawnPoint.y;
                m_server->m_socket.sendTo(teleport.serialize(), client.address, client.port);

                PlayerDownedPacket downedPacket;
                downedPacket.playerId = client.id;
                downedPacket.isDown = 0;
                downedPacket.remainingSeconds = 0;
                m_server->m_socket.sendTo(downedPacket.serialize(), client.address, client.port);

                PlayerCursedPacket cursedPacket;
                cursedPacket.playerId = client.id;
                cursedPacket.isCursed = 0;
                cursedPacket.remainingSeconds = 0;
                m_server->m_socket.sendTo(cursedPacket.serialize(), client.address, client.port);

                int newArea = m_server->m_areaManager->findClosestArea(spawnPoint.x, spawnPoint.y);
                if (newArea != client.currentArea) {
                    client.currentArea = newArea;

                    if (m_server->m_areasWithEnemies.find(client.currentArea) == m_server->m_areasWithEnemies.end()) {
                        m_server->m_entityManager->spawnEnemiesForArea(client.currentArea);
                    }
                }
            }
            return;
        }
    }
}

} // namespace evades 