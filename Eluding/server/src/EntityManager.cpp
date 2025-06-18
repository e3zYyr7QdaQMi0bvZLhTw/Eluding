#include "../include/EntityManager.h"
#include "../include/PlayerManager.h"
#include "../include/Entities/SlowingEnemy.h"
#include "../include/Entities/SilenceEnemy.h"
#include "../include/Entities/ExpanderEnemy.h"
#include "../include/Entities/WaveringEnemy.h"
#include "../include/Entities/SniperEnemy.h"
#include "../include/Entities/SniperBullet.h"
#include "../include/Entities/DasherEnemy.h"
#include "../include/Entities/ImmuneEnemy.h"

namespace evades {

EntityManager::EntityManager(GameServer* server) : m_server(server) {
}

void EntityManager::spawnEnemiesForArea(int areaIndex) {
    if (!m_server->m_map || areaIndex < 0 || areaIndex >= static_cast<int>(m_server->m_map->areas.size())) return;

    std::cout << "Spawning enemies for area " << areaIndex << "..." << std::endl;

    const auto& area = m_server->m_map->areas[areaIndex];

    for (const auto& zone : area.zones) {
        if (zone.type == ZoneType::Active) {
            for (const auto& spawner : zone.spawners) {
                spawnEnemiesFromSpawner(area, zone, spawner);
            }
        }
    }

    m_server->m_areasWithEnemies.insert(areaIndex);

    std::cout << "Spawned enemies for area " << areaIndex << ", total enemies: " << m_server->m_enemies.size() << std::endl;
}

void EntityManager::despawnEnemiesInArea(int areaIndex) {
    if (!m_server->m_map || areaIndex < 0 || areaIndex >= static_cast<int>(m_server->m_map->areas.size())) return;

    std::cout << "Despawning enemies in area " << areaIndex << "..." << std::endl;

    const auto& area = m_server->m_map->areas[areaIndex];

    int removedCount = 0;

    for (auto it = m_server->m_enemies.begin(); it != m_server->m_enemies.end();) {
        auto& enemy = it->second;

        float enemyX = enemy->getX();
        float enemyY = enemy->getY();

        const MapArea* enemyArea = m_server->m_map->getAreaAt(enemyX, enemyY);

        if (enemyArea && enemyArea == &area) {
            it = m_server->m_enemies.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }

    m_server->m_areasWithEnemies.erase(areaIndex);

    std::cout << "Despawned " << removedCount << " enemies from area " << areaIndex << std::endl;
}

void EntityManager::checkAndSpawnEnemies() {
    if (!m_server->m_map) return;

    std::unordered_set<int> areasWithPlayers;

    for (const auto& [id, client] : m_server->m_clients) {
        areasWithPlayers.insert(client.currentArea);
    }

    for (int areaIndex : areasWithPlayers) {
        if (m_server->m_areasWithEnemies.find(areaIndex) == m_server->m_areasWithEnemies.end()) {
            spawnEnemiesForArea(areaIndex);
        }
    }

    std::vector<int> areasToCleanup;
    for (int areaIndex : m_server->m_areasWithEnemies) {
        if (areasWithPlayers.find(areaIndex) == areasWithPlayers.end()) {
            areasToCleanup.push_back(areaIndex);
        }
    }

    for (int areaIndex : areasToCleanup) {
        despawnEnemiesInArea(areaIndex);
    }

    for (int areaIndex : areasWithPlayers) {
        if (m_server->m_areasWithEnemies.find(areaIndex) != m_server->m_areasWithEnemies.end()) {
            int enemiesInArea = 0;
            const auto& area = m_server->m_map->areas[areaIndex];

            for (const auto& [id, enemy] : m_server->m_enemies) {
                float enemyX = enemy->getX();
                float enemyY = enemy->getY();
                const MapArea* enemyArea = m_server->m_map->getAreaAt(enemyX, enemyY);
                if (enemyArea && enemyArea == &area) {
                    enemiesInArea++;
                }
            }

            int maxEnemiesInArea = 0;
            for (const auto& zone : area.zones) {
                if (zone.type == ZoneType::Active) {
                    for (const auto& spawner : zone.spawners) {
                        maxEnemiesInArea += spawner.count;
                    }
                }
            }

            if (enemiesInArea < maxEnemiesInArea) {
                spawnEnemiesFromSpawner(area, area.zones[0], area.zones[0].spawners[0]);
            }
        }
    }
}

void EntityManager::spawnEnemiesFromSpawner(const MapArea& area, const MapZone& zone, const Spawner& spawner) {
    float zoneX = area.x + zone.x;
    float zoneY = area.y + zone.y;
    int count = spawner.count;

    bool hasWallEnemies = false;
    int wallEnemyCount = 0;

    for (const auto& typeStr : spawner.enemyTypes) {
        if (Enemy::stringToEnemyType(typeStr) == Enemy::Type::Wall) {
            hasWallEnemies = true;
            wallEnemyCount = count; 
            break;
        }
    }

    int initialSide = -1;
    bool moveClockwise = spawner.moveClockwise; 

    if (hasWallEnemies) {
        std::uniform_int_distribution<int> sideDist(0, 3);
        initialSide = sideDist(m_server->m_random);
    }

    int wallIndex = 0;

    bool useRandomSpeed = (spawner.min_speed > 0.0f && spawner.max_speed > 0.0f);
    std::uniform_real_distribution<float> speedDist(spawner.min_speed, spawner.max_speed);

    for (int i = 0; i < count; i++) {
        if (!spawner.enemyTypes.empty()) {
            std::uniform_int_distribution<int> typeDist(0, spawner.enemyTypes.size() - 1);
            int typeIndex = typeDist(m_server->m_random);

            Enemy::Type enemyType = Enemy::stringToEnemyType(spawner.enemyTypes[typeIndex]);
            std::unique_ptr<Enemy> enemy;

            float enemySpeed = spawner.speed;
            if (useRandomSpeed) {
                enemySpeed = speedDist(m_server->m_random);
            }

            if (enemyType == Enemy::Type::Wall) {
                enemy = Enemy::createWallEnemy(
                    zoneX, zoneY, zone.width, zone.height,
                    enemySpeed, spawner.radius, 
                    wallIndex, wallEnemyCount, initialSide, moveClockwise);

                wallIndex++;
            } else if (enemyType == Enemy::Type::Wavering) {
                float minSpeed = spawner.min_speed > 0.0f ? spawner.min_speed : enemySpeed / 2.0f;
                float maxSpeed = spawner.max_speed > 0.0f ? spawner.max_speed : enemySpeed * 2.0f;

                std::uniform_real_distribution<float> xDist(zoneX + spawner.radius, zoneX + zone.width - spawner.radius);
                std::uniform_real_distribution<float> yDist(zoneY + spawner.radius, zoneY + zone.height - spawner.radius);

                float x = xDist(m_server->m_random);
                float y = yDist(m_server->m_random);

                enemy = Enemy::createWaveringEnemy(x, y, spawner.radius, enemySpeed, minSpeed, maxSpeed, 0.25f);
            } else {
                std::uniform_real_distribution<float> xDist(zoneX + spawner.radius, zoneX + zone.width - spawner.radius);
                std::uniform_real_distribution<float> yDist(zoneY + spawner.radius, zoneY + zone.height - spawner.radius);

                float x = xDist(m_server->m_random);
                float y = yDist(m_server->m_random);
                enemy = Enemy::createEnemy(enemyType, x, y, spawner.radius, enemySpeed);
            }

            enemy->serialize(m_server->m_enemySerializationBuffer); 
            EnemyState state = EnemyState::deserialize(m_server->m_enemySerializationBuffer, m_server->m_serializeOffset);
            m_server->m_enemySerializationBuffer.clear();
            m_server->m_serializeOffset = 0;

            if (enemyType == Enemy::Type::Wavering) {
                WaveringEnemy* waveringEnemy = dynamic_cast<WaveringEnemy*>(enemy.get());
                if (waveringEnemy) {
                    state.speed = waveringEnemy->getCurrentSpeed();
                    state.minSpeed = waveringEnemy->getMinSpeed();
                    state.maxSpeed = waveringEnemy->getMaxSpeed();
                    state.changeProgress = waveringEnemy->getChangeProgress();
                    state.isSpeedIncreasing = waveringEnemy->isSpeedIncreasing();
                }
            }

            uint32_t enemyId = m_server->m_nextEnemyId++;

            m_server->m_enemies[enemyId] = std::move(enemy);
            std::cout << "Spawned enemy with ID " << enemyId << " type " << static_cast<int>(enemyType) 
                      << " speed=" << enemySpeed << " radius=" << spawner.radius << std::endl;
        }
    }
}

void EntityManager::updateEnemies(float deltaTime) {
    for (auto it = m_server->m_enemies.begin(); it != m_server->m_enemies.end();) {
        auto& enemy = it->second;

        enemy->update(deltaTime, m_server->m_map);

        bool shouldRemove = false;

        SniperBullet* bullet = dynamic_cast<SniperBullet*>(enemy.get());
        if (bullet && bullet->shouldRemove()) {
            shouldRemove = true;
        }

        if (m_server->m_map) {
            const MapZone* zone = m_server->m_map->getZoneAt(enemy->getX(), enemy->getY());
            if (!zone || zone->type == ZoneType::Blocked) {
                shouldRemove = true;
            }

            if (bullet && zone && zone->type == ZoneType::Safe) {
                shouldRemove = true;
            }
        } 
        else {
            if (enemy->getX() < 0 || enemy->getX() > WORLD_WIDTH || 
                enemy->getY() < 0 || enemy->getY() > WORLD_HEIGHT) {
                shouldRemove = true;
            }
        }

        if (shouldRemove) {
            it = m_server->m_enemies.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& [enemyId, enemy] : m_server->m_enemies) {
        for (auto& [clientId, client] : m_server->m_clients) {
            if (enemy->getType() == Enemy::Type::Silence) {
                SilenceEnemy* silenceEnemy = dynamic_cast<SilenceEnemy*>(enemy.get());
                if (silenceEnemy) {
                    if (silenceEnemy->isPlayerInAura(client.state.x, client.state.y)) {
                        client.isSilenced = true;
                        silenceEnemy->updateAuraSize(deltaTime, true);
                    }
                }
            }

            if (client.state.isDowned) continue;

            Entity playerEntity(client.state.x, client.state.y, client.state.radius, 0.0f);

            if (enemy->isCollidingWith(playerEntity)) {
                if (enemy->isHarmless()) {
                    continue;
                }

                Enemy::Type enemyType = enemy->getType();

                if (enemyType == Enemy::Type::Normal) {
                    m_server->m_playerManager->downPlayer(client);
                }
                else if (enemyType == Enemy::Type::Cursed) {
                    m_server->m_playerManager->cursePlayer(client);
                    enemy->makeHarmless(1.5f);
                }
                else if (enemyType == Enemy::Type::Wall) {
                    m_server->m_playerManager->downPlayer(client);
                }
                else if (enemyType == Enemy::Type::Slowing) {
                    m_server->m_playerManager->downPlayer(client);
                }
                else if (enemyType == Enemy::Type::Immune) {
                    m_server->m_playerManager->downPlayer(client);
                }
                else if (enemyType == Enemy::Type::Wavering) {
                    m_server->m_playerManager->downPlayer(client);
                }
                else if (enemyType == Enemy::Type::Expander) {
                    m_server->m_playerManager->handleExpanderCollision(client, enemy.get());
                }
                else if (enemyType == Enemy::Type::Silence) {
                    m_server->m_playerManager->downPlayer(client);
                }
                else if (enemyType == Enemy::Type::SniperBullet) {
                    m_server->m_playerManager->downPlayer(client);
                    enemy->makeHarmless(0.1f);
                }
                else if (enemyType == Enemy::Type::Sniper) {
                    m_server->m_playerManager->downPlayer(client);
                }
                else if (enemyType == Enemy::Type::Dasher) {
                    m_server->m_playerManager->downPlayer(client);
                }
                break;
            }
        }
    }
}

void EntityManager::updateSniperEnemies(float deltaTime) {
    if (m_server->m_clients.empty()) return; 

    std::vector<PlayerState> playerStates;
    for (const auto& [clientId, client] : m_server->m_clients) {
        playerStates.push_back(client.state);
    }

    for (auto it = m_server->m_enemies.begin(); it != m_server->m_enemies.end(); ++it) {
        auto& enemy = it->second;
        SniperEnemy* sniperEnemy = dynamic_cast<SniperEnemy*>(enemy.get());

        if (sniperEnemy && sniperEnemy->canShoot()) {
            const MapArea* area = m_server->m_map->getAreaAt(sniperEnemy->getX(), sniperEnemy->getY());
            const MapZone* zone = m_server->m_map->getZoneAt(sniperEnemy->getX(), sniperEnemy->getY());

            if (!area || !zone || zone->type != ZoneType::Active) {
                continue;
            }

            const PlayerState* target = sniperEnemy->findClosestPlayer(playerStates, zone, area, m_server->m_map);

            if (target) {
                float dx = target->x - sniperEnemy->getX();
                float dy = target->y - sniperEnemy->getY();
                float angle = std::atan2(dy, dx);

                float bulletRadius = sniperEnemy->getRadius() / 2.0f;
                float bulletSpeed = sniperEnemy->getSpeed() * 2.5f;

                std::unique_ptr<Enemy> bullet = Enemy::createSniperBullet(
                    sniperEnemy->getX(), sniperEnemy->getY(), 
                    bulletRadius, bulletSpeed, angle
                );

                m_server->m_enemies[m_server->m_nextEnemyId++] = std::move(bullet);
                sniperEnemy->resetShotTimer();
            }
        }
    }
}

void EntityManager::broadcastEnemyState() {
    EnemyUpdatePacket packet;

    for (const auto& [id, enemy] : m_server->m_enemies) {
        EnemyState state;
        state.id = id;
        state.x = enemy->getX();
        state.y = enemy->getY();
        state.radius = enemy->getRadius();
        state.type = static_cast<uint8_t>(enemy->getType());

        if (enemy->getType() == Enemy::Type::Wavering) {
            WaveringEnemy* waveringEnemy = dynamic_cast<WaveringEnemy*>(enemy.get());
            if (waveringEnemy) {
                state.speed = waveringEnemy->getCurrentSpeed();
                state.minSpeed = waveringEnemy->getMinSpeed();
                state.maxSpeed = waveringEnemy->getMaxSpeed();
                state.changeProgress = waveringEnemy->getChangeProgress();
                state.isSpeedIncreasing = waveringEnemy->isSpeedIncreasing();
            }
        }

        if (enemy->getType() == Enemy::Type::Silence) {
            SilenceEnemy* silenceEnemy = dynamic_cast<SilenceEnemy*>(enemy.get());
            if (silenceEnemy) {
                state.auraSize = silenceEnemy->getAuraSize();
            }
        }

        if (enemy->getType() == Enemy::Type::Slowing) {
            SlowingEnemy* slowingEnemy = dynamic_cast<SlowingEnemy*>(enemy.get());
            if (slowingEnemy) {
                state.auraSize = SlowingEnemy::AURA_RADIUS;
            }
        }

        state.isHarmless = enemy->isHarmless();
        state.harmlessProgress = enemy->getHarmlessProgress();

        packet.enemies.push_back(state);
    }

    if (!packet.enemies.empty()) {
        auto serializedPacket = packet.serialize();

        for (const auto& [id, client] : m_server->m_clients) {
            m_server->m_socket.sendTo(serializedPacket, client.address, client.port);
        }
    }
}

} // namespace evades 