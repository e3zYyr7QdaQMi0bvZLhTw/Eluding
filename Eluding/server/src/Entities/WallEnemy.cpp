#include "WallEnemy.h"
#include <iostream>
#include <random>

namespace evades {

WallEnemy::WallEnemy(float x, float y, float radius, float speed)
    : Enemy(x, y, radius, speed, Enemy::Type::Wall) {

    m_wallDirection = WallDirection::Right; 
    m_moveClockwise = true; 

    setVelocityFromWallDirection();
}

void WallEnemy::updateBehavior(float deltaTime, const std::shared_ptr<GameMap>& map) {
    if (!map) return;

    setVelocityFromWallDirection();

    const MapZone* currentZone = map->getZoneAt(m_position.x, m_position.y);
    if (!currentZone) return;

    const MapArea* area = map->getAreaAt(m_position.x, m_position.y);
    if (!area) return;

    float zoneLeft = area->x + currentZone->x;
    float zoneTop = area->y + currentZone->y;
    float zoneRight = zoneLeft + currentZone->width;
    float zoneBottom = zoneTop + currentZone->height;

    float nextX = m_position.x + m_velocity.x * deltaTime;
    float nextY = m_position.y + m_velocity.y * deltaTime;

    bool hitWall = false;

    switch (m_wallDirection) {
        case WallDirection::Up: 
            if (nextY - m_radius <= zoneTop) {
                m_position.y = zoneTop + m_radius;
                hitWall = true;
            }
            break;

        case WallDirection::Right: 
            if (nextX + m_radius >= zoneRight) {
                m_position.x = zoneRight - m_radius;
                hitWall = true;
            }
            break;

        case WallDirection::Down: 
            if (nextY + m_radius >= zoneBottom) {
                m_position.y = zoneBottom - m_radius;
                hitWall = true;
            }
            break;

        case WallDirection::Left: 
            if (nextX - m_radius <= zoneLeft) {
                m_position.x = zoneLeft + m_radius;
                hitWall = true;
            }
            break;
    }

    if (hitWall) {
        m_wallDirection = rotateDirection(m_wallDirection, m_moveClockwise);
        setVelocityFromWallDirection();
    }
}

WallEnemy::WallDirection WallEnemy::rotateDirection(WallDirection direction, bool clockwise) {
    switch (direction) {
        case WallDirection::Up:
            return clockwise ? WallDirection::Left : WallDirection::Right;
        case WallDirection::Right:
            return clockwise ? WallDirection::Up : WallDirection::Down;
        case WallDirection::Down:
            return clockwise ? WallDirection::Right : WallDirection::Left;
        case WallDirection::Left:
            return clockwise ? WallDirection::Down : WallDirection::Up;
        default:
            return WallDirection::Right;
    }
}

void WallEnemy::setVelocityFromWallDirection() {
    switch (m_wallDirection) {
        case WallDirection::Up:
            m_velocity = Vector2(0, -m_speed);
            break;
        case WallDirection::Down:
            m_velocity = Vector2(0, m_speed);
            break;
        case WallDirection::Right:
            m_velocity = Vector2(m_speed, 0);
            break;
        case WallDirection::Left:
            m_velocity = Vector2(-m_speed, 0);
            break;
    }
}

std::unique_ptr<WallEnemy> WallEnemy::create(
    float zoneX, float zoneY, float zoneWidth, float zoneHeight, 
    float speed, float radius, int wallIndex, int totalCount,
    int initialSide, bool moveClockwise) {

    float actualRadius = radius;

    if (initialSide < 0 || initialSide > 3) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> sideDist(0, 3);
        initialSide = sideDist(gen);
    }

    float perimeter = 2 * ((zoneWidth - actualRadius * 2) + (zoneHeight - actualRadius * 2));

    float distance = wallIndex * perimeter / totalCount;

    float x = 0, y = 0;
    WallDirection initialDirection;

    switch (initialSide) {
        case 0: 
            x = zoneWidth / 2 + zoneX;
            y = zoneY + actualRadius;
            initialDirection = WallDirection::Up; 
            break;
        case 1: 
            x = zoneX + zoneWidth - actualRadius;
            y = zoneHeight / 2 + zoneY;
            initialDirection = WallDirection::Right; 
            break;
        case 2: 
            x = zoneWidth / 2 + zoneX;
            y = zoneY + zoneHeight - actualRadius;
            initialDirection = WallDirection::Down; 
            break;
        case 3: 
            x = zoneX + actualRadius;
            y = zoneHeight / 2 + zoneY;
            initialDirection = WallDirection::Left; 
            break;
    }

    switch (initialSide) {
        case 0: 
            initialDirection = moveClockwise ? WallDirection::Left : WallDirection::Right;
            break;
        case 1: 
            initialDirection = moveClockwise ? WallDirection::Up : WallDirection::Down;
            break;
        case 2: 
            initialDirection = moveClockwise ? WallDirection::Right : WallDirection::Left;
            break;
        case 3: 
            initialDirection = moveClockwise ? WallDirection::Down : WallDirection::Up;
            break;
    }

    int antiCrash = 0;
    WallDirection currentDirection = initialDirection;

    while (distance > 0) {

        if (antiCrash > 1000) {
            std::cerr << "Potential infinite loop when positioning wall enemy" << std::endl;
            break;
        }
        antiCrash++;

        switch (currentDirection) {
            case WallDirection::Up: 
                {
                    float moveDistance = std::min(distance, y - (zoneY + actualRadius));
                    y -= moveDistance;
                    distance -= moveDistance;

                    if (y <= zoneY + actualRadius) {
                        y = zoneY + actualRadius;
                        currentDirection = rotateDirection(currentDirection, moveClockwise);
                    } else {
                        break;
                    }
                }
                break;

            case WallDirection::Right: 
                {
                    float moveDistance = std::min(distance, (zoneX + zoneWidth - actualRadius) - x);
                    x += moveDistance;
                    distance -= moveDistance;

                    if (x >= zoneX + zoneWidth - actualRadius) {
                        x = zoneX + zoneWidth - actualRadius;
                        currentDirection = rotateDirection(currentDirection, moveClockwise);
                    } else {
                        break;
                    }
                }
                break;

            case WallDirection::Down: 
                {
                    float moveDistance = std::min(distance, (zoneY + zoneHeight - actualRadius) - y);
                    y += moveDistance;
                    distance -= moveDistance;

                    if (y >= zoneY + zoneHeight - actualRadius) {
                        y = zoneY + zoneHeight - actualRadius;
                        currentDirection = rotateDirection(currentDirection, moveClockwise);
                    } else {
                        break;
                    }
                }
                break;

            case WallDirection::Left: 
                {
                    float moveDistance = std::min(distance, x - (zoneX + actualRadius));
                    x -= moveDistance;
                    distance -= moveDistance;

                    if (x <= zoneX + actualRadius) {
                        x = zoneX + actualRadius;
                        currentDirection = rotateDirection(currentDirection, moveClockwise);
                    } else {
                        break;
                    }
                }
                break;
        }
    }

    auto enemy = std::make_unique<WallEnemy>(x, y, actualRadius, speed);

    enemy->m_wallDirection = currentDirection;
    enemy->m_moveClockwise = moveClockwise;

    enemy->setVelocityFromWallDirection();

    return enemy;
}

} 
