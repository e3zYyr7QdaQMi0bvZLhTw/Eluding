#include "../include/AreaManager.h"

namespace evades {

AreaManager::AreaManager(GameServer* server) : m_server(server) {
}

int AreaManager::findClosestArea(float x, float y) {
    if (!m_server->m_map) return 0;

    int closestArea = 0;
    float minDistance = std::numeric_limits<float>::max();

    for (size_t i = 0; i < m_server->m_map->areas.size(); ++i) {
        const auto& area = m_server->m_map->areas[i];

        float areaLeft = area.x;
        float areaTop = area.y;
        float areaRight = area.x + area.getWidth();
        float areaBottom = area.y + area.getHeight();

        float closestX = std::max(areaLeft, std::min(x, areaRight));
        float closestY = std::max(areaTop, std::min(y, areaBottom));

        float dx = x - closestX;
        float dy = y - closestY;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < minDistance) {
            minDistance = distance;
            closestArea = static_cast<int>(i);
        }

        if (x >= areaLeft && x <= areaRight && y >= areaTop && y <= areaBottom) {
            return static_cast<int>(i);
        }
    }

    return closestArea;
}

void AreaManager::loadMap(const std::string& mapFile) {
    std::cout << "Loading map: " << mapFile << std::endl;

    if (!std::filesystem::exists(mapFile)) {
        std::cerr << "Map file not found: " << mapFile << std::endl;
        return;
    }

    m_server->m_map = GameMap::loadFromFile(mapFile);

    if (!m_server->m_map) {
        std::cerr << "Failed to load map: " << mapFile << std::endl;
        return;
    }

    std::cout << "Map loaded successfully: " << m_server->m_map->name << std::endl;

    if (m_server->m_map) {
        std::cout << "Map has " << m_server->m_map->areas.size() << " areas:" << std::endl;
        for (size_t i = 0; i < m_server->m_map->areas.size(); i++) {
            const auto& area = m_server->m_map->areas[i];
            std::cout << "  Area " << i << ": pos=(" << area.x << "," << area.y 
                      << "), size=(" << area.getWidth() << "," << area.getHeight() << ")" << std::endl;

            std::cout << "    Zones: " << area.zones.size() << std::endl;
            for (size_t j = 0; j < area.zones.size(); j++) {
                const auto& zone = area.zones[j];
                std::string type;
                switch (zone.type) {
                    case ZoneType::Safe: type = "Safe"; break;
                    case ZoneType::Active: type = "Active"; break;
                    case ZoneType::Exit: type = "Exit"; break;
                    case ZoneType::Teleport: type = "Teleport"; break;
                    case ZoneType::Blocked: type = "Blocked"; break;
                    default: type = "Unknown"; break;
                }
                std::cout << "      Zone " << j << ": type=" << type << ", pos=(" 
                          << zone.x << "," << zone.y << "), size=(" 
                          << zone.width << "," << zone.height << ")" << std::endl;
                if (zone.type == ZoneType::Exit || zone.type == ZoneType::Teleport) {
                    std::cout << "        Translate: (" << zone.translate.x << "," 
                              << zone.translate.y << ")" << std::endl;
                }

                if (zone.type == ZoneType::Active && !zone.spawners.empty()) {
                    std::cout << "        Spawners: " << zone.spawners.size() << std::endl;
                    for (size_t k = 0; k < zone.spawners.size(); k++) {
                        const auto& spawner = zone.spawners[k];
                        std::cout << "          Spawner " << k << ": count=" << spawner.count
                                  << ", radius=" << spawner.radius << ", speed=" << spawner.speed << std::endl;
                        std::cout << "            Types: ";
                        for (const auto& type : spawner.enemyTypes) {
                            std::cout << type << " ";
                        }
                        std::cout << std::endl;
                    }
                }
            }
        }
    }
}

bool AreaManager::playerCircleIntersectsZone(float playerX, float playerY, float playerRadius, const MapArea* area, const MapZone* zone) const {
    if (!area || !zone) return false;

    float zoneLeft = area->x + zone->x;
    float zoneTop = area->y + zone->y;
    float zoneRight = zoneLeft + zone->width;
    float zoneBottom = zoneTop + zone->height;

    float closestX = std::max(zoneLeft, std::min(playerX, zoneRight));
    float closestY = std::max(zoneTop, std::min(playerY, zoneBottom));

    float distanceX = playerX - closestX;
    float distanceY = playerY - closestY;
    float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);

    return distanceSquared <= (playerRadius * playerRadius);
}

std::vector<std::pair<const MapArea*, const MapZone*>> AreaManager::getIntersectingZones(float x, float y, float radius) const {
    std::vector<std::pair<const MapArea*, const MapZone*>> result;

    if (!m_server->m_map) return result;

    for (const auto& area : m_server->m_map->areas) {
        for (const auto& zone : area.zones) {
            if (playerCircleIntersectsZone(x, y, radius, &area, &zone)) {
                result.push_back({&area, &zone});
            }
        }
    }

    return result;
}

} // namespace evades 