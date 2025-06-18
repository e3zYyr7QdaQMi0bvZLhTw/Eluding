#include "SniperEnemy.h"
#include <random>
#include <cmath>
#include <limits>

namespace evades {

SniperEnemy::SniperEnemy(float x, float y, float radius, float speed)
    : Enemy(x, y, radius, speed, Enemy::Type::Sniper) {

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, RELEASE_TIME * 0.75f); 
    m_timeSinceLastShot = dist(gen);

    m_bulletRadius = radius / 2.0f;
    m_bulletSpeed = speed * 2.0f;  
}

const PlayerState* SniperEnemy::findClosestPlayer(const std::vector<PlayerState>& players, 
                                               const MapZone* activeZone, 
                                               const MapArea* area,
                                               const std::shared_ptr<GameMap>& map) const {
    if (!activeZone || !area) return nullptr;

    float detectionDistanceSquared = DETECTION_DISTANCE * DETECTION_DISTANCE;
    float closestDistance = std::numeric_limits<float>::max();
    const PlayerState* closestPlayer = nullptr;

    float zoneLeft = area->x + activeZone->x;
    float zoneTop = area->y + activeZone->y;
    float zoneRight = zoneLeft + activeZone->width;
    float zoneBottom = zoneTop + activeZone->height;

    for (const auto& player : players) {

        if (player.isDowned) continue;

        if (player.x >= zoneLeft && player.x <= zoneRight && 
            player.y >= zoneTop && player.y <= zoneBottom) {

            float dx = player.x - m_position.x;
            float dy = player.y - m_position.y;
            float distSquared = dx * dx + dy * dy;

            if (distSquared < detectionDistanceSquared && distSquared < closestDistance) {

                const MapZone* playerZone = map->getZoneAt(player.x, player.y);
                if (playerZone && playerZone->type != ZoneType::Safe) {
                    closestDistance = distSquared;
                    closestPlayer = &player;
                }
            }
        }
    }

    return closestPlayer;
}

void SniperEnemy::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {

    m_timeSinceLastShot += deltaTime;

    if (m_velocity.x == 0 && m_velocity.y == 0) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        m_velocity = getRandomDirection(gen) * m_speed;
    }
}

} 
