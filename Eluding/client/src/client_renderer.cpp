#include "client_renderer.h"
#include "../../shared/include/protocol.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>  

namespace evades {

sf::Color toSfColor(const Color& color) {
    return sf::Color(color.r, color.g, color.b, color.a);
}

ClientRenderer::ClientRenderer(sf::RenderWindow& window)
    : m_window(window), 
      m_zoomFactor(1.0f),
      m_uiScale(1.5f),
      m_map(nullptr),
      m_currentPlayerArea(nullptr) {

    m_gameView = m_window.getDefaultView();
    m_gameView.setSize(sf::Vector2f(m_gameView.getSize().x * m_zoomFactor, m_gameView.getSize().y * m_zoomFactor));

    m_enemyCircle.setPointCount(30);
    m_enemyOutline.setPointCount(30);
    m_enemyAura.setPointCount(30);
    m_enemyAura.setRadius(150.0f);
    m_enemyAura.setOrigin(sf::Vector2f(150.0f, 150.0f));

    m_playerCircle.setPointCount(60);

    m_horizontalLine.setFillColor(sf::Color(200, 200, 200));
    m_verticalLine.setFillColor(sf::Color(200, 200, 200));

    m_arrowShape.setPointCount(3);
    m_arrowShape.setPoint(0, sf::Vector2f(30.0f, 0));  
    m_arrowShape.setPoint(1, sf::Vector2f(0, 8.0f));   
    m_arrowShape.setPoint(2, sf::Vector2f(0, -8.0f));  
    m_arrowShape.setFillColor(sf::Color(255, 0, 0, 128));
    m_arrowShape.setOrigin(sf::Vector2f(15.0f, 0));

    loadFonts();
}

bool ClientRenderer::loadFonts() {
    try {

        if (m_font.openFromFile("arial.ttf")) {
            std::cout << "Font loaded successfully: arial.ttf" << std::endl;
        }

        else if (m_font.openFromFile("tahoma.ttf")) {
            std::cout << "Font loaded successfully: tahoma.ttf" << std::endl;
        }

        else if (m_font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
            std::cout << "Arial font loaded from Windows fonts" << std::endl;
        }
        else if (m_font.openFromFile("C:/Windows/Fonts/tahoma.ttf")) {
            std::cout << "Font loaded successfully from Windows fonts" << std::endl;
        }
        else {
            std::cerr << "Failed to load any font file" << std::endl;
            return false;
        }

        m_timerText = std::unique_ptr<sf::Text>(new sf::Text(m_font, sf::String(), 14));
        m_mapAreaText = std::unique_ptr<sf::Text>(new sf::Text(m_font, sf::String(), 32));

        m_timerText->setStyle(sf::Text::Bold);

        m_mapAreaText->setStyle(sf::Text::Bold);
        m_mapAreaText->setFillColor(sf::Color::White);
        m_mapAreaText->setOutlineColor(sf::Color(66, 90, 109));
        m_mapAreaText->setOutlineThickness(4.0f);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Font error: " << e.what() << std::endl;
        return false;
    }
}

void ClientRenderer::setZoomFactor(float zoomFactor) {
    m_zoomFactor = zoomFactor;

    sf::Vector2f center = m_gameView.getCenter();

    sf::Vector2f currentSize = m_gameView.getSize();
    float baseWidth = currentSize.x / m_zoomFactor;
    float baseHeight = currentSize.y / m_zoomFactor;

    m_gameView.setSize(sf::Vector2f(baseWidth * m_zoomFactor, baseHeight * m_zoomFactor));

    m_gameView.setCenter(center);
}

void ClientRenderer::centerViewOn(float x, float y) {
    m_gameView.setCenter(sf::Vector2f(x, y));

    sf::Vector2f center = m_gameView.getCenter();
    sf::Vector2f size = m_gameView.getSize();
    sf::Vector2f topLeft = center - size / 2.0f;
    m_viewBounds = sf::FloatRect(topLeft, size);
}

void ClientRenderer::updateViewForResize(unsigned int width, unsigned int height) {

    float windowRatio = static_cast<float>(width) / static_cast<float>(height);

    float gameRatio = static_cast<float>(CLIENT_WIDTH) / static_cast<float>(CLIENT_HEIGHT);

    float viewWidth, viewHeight;

    if (windowRatio > gameRatio) {

        viewHeight = CLIENT_HEIGHT;
        viewWidth = viewHeight * windowRatio;
    } else {

        viewWidth = CLIENT_WIDTH;
        viewHeight = viewWidth / windowRatio;
    }

    sf::Vector2f center = m_gameView.getCenter();

    viewWidth *= m_zoomFactor;
    viewHeight *= m_zoomFactor;

    m_gameView.setSize(sf::Vector2f(viewWidth, viewHeight));
    m_gameView.setCenter(center); 

    sf::FloatRect viewport({0.f, 0.f}, {1.f, 1.f});
    m_gameView.setViewport(viewport);

    m_uiScale = std::min(width / static_cast<float>(CLIENT_WIDTH), height / static_cast<float>(CLIENT_HEIGHT));

    std::cout << "Window resized to: " << width << "x" << height << std::endl;
    std::cout << "View size set to: " << viewWidth << "x" << viewHeight << std::endl;
}

void ClientRenderer::beginFrame(const sf::Color& bgColor) {
    m_window.clear(bgColor);

}

void ClientRenderer::applyView() {
    m_window.setView(m_gameView);
}

void ClientRenderer::endFrame() {
    m_window.display();
}

void ClientRenderer::renderGrid() {
    sf::View currentView = m_window.getView();
    sf::Vector2f center = currentView.getCenter();
    sf::Vector2f size = currentView.getSize();

    sf::Vector2f topLeft = center - size / 2.0f;
    sf::Vector2f bottomRight = center + size / 2.0f;

    m_viewBounds = sf::FloatRect(topLeft, bottomRight - topLeft);

    topLeft.x -= GRID_SIZE;
    topLeft.y -= GRID_SIZE;
    bottomRight.x += GRID_SIZE;
    bottomRight.y += GRID_SIZE;

    int startX = static_cast<int>(std::floor(topLeft.x / GRID_SIZE)) * GRID_SIZE;
    int startY = static_cast<int>(std::floor(topLeft.y / GRID_SIZE)) * GRID_SIZE;
    int endX = static_cast<int>(std::floor(bottomRight.x / GRID_SIZE)) * GRID_SIZE;
    int endY = static_cast<int>(std::floor(bottomRight.y / GRID_SIZE)) * GRID_SIZE;

    m_verticalLine.setSize(sf::Vector2f(2.0f, bottomRight.y - topLeft.y));

    for (int x = startX; x <= endX; x += GRID_SIZE) {
        m_verticalLine.setPosition(sf::Vector2f(static_cast<float>(x) - 1.0f, topLeft.y));
        m_window.draw(m_verticalLine);
    }

    m_horizontalLine.setSize(sf::Vector2f(bottomRight.x - topLeft.x, 2.0f));

    for (int y = startY; y <= endY; y += GRID_SIZE) {
        m_horizontalLine.setPosition(sf::Vector2f(topLeft.x, static_cast<float>(y) - 1.0f));
        m_window.draw(m_horizontalLine);
    }
}

void ClientRenderer::renderZoneGrid(float x, float y, float width, float height, const sf::Color& gridColor) {

    float startX = std::floor(x / GRID_SIZE) * GRID_SIZE;
    float startY = std::floor(y / GRID_SIZE) * GRID_SIZE;

    if (startX < x) startX += GRID_SIZE;
    if (startY < y) startY += GRID_SIZE;

    float endX = std::floor((x + width) / GRID_SIZE) * GRID_SIZE;
    float endY = std::floor((y + height) / GRID_SIZE) * GRID_SIZE;

    m_verticalLine.setSize(sf::Vector2f(2.0f, height));
    m_verticalLine.setFillColor(gridColor);

    for (float gridX = startX; gridX <= endX; gridX += GRID_SIZE) {
        m_verticalLine.setPosition(sf::Vector2f(gridX - 1.0f, y));
        m_window.draw(m_verticalLine);
    }

    m_horizontalLine.setSize(sf::Vector2f(width, 2.0f));
    m_horizontalLine.setFillColor(gridColor);

    for (float gridY = startY; gridY <= endY; gridY += GRID_SIZE) {
        m_horizontalLine.setPosition(sf::Vector2f(x, gridY - 1.0f));
        m_window.draw(m_horizontalLine);
    }
}

void ClientRenderer::renderMap(const std::shared_ptr<GameMap>& map, const Vector2& playerPos) {
    if (!map) {
        renderGrid();
        return;
    }

    sf::View currentView = m_window.getView();
    sf::Vector2f center = currentView.getCenter();
    sf::Vector2f size = currentView.getSize();

    sf::Vector2f topLeft = center - size / 2.0f;
    sf::Vector2f bottomRight = center + size / 2.0f;

    const MapArea* currentArea = map->getAreaAt(playerPos.x, playerPos.y);
    if (!currentArea) return;

    m_map = map;
    m_currentPlayerArea = currentArea;

    sf::FloatRect areaRect(
        sf::Vector2f(currentArea->x, currentArea->y),
        sf::Vector2f(currentArea->getWidth(), currentArea->getHeight())
    );

    sf::FloatRect viewRect(topLeft, bottomRight - topLeft);

    if (!viewRect.findIntersection(areaRect)) {
        return;
    }

    for (const auto& zone : currentArea->zones) {

        sf::FloatRect zoneRect(
            sf::Vector2f(currentArea->x + zone.x, currentArea->y + zone.y),
            sf::Vector2f(zone.width, zone.height)
        );

        if (!viewRect.findIntersection(zoneRect)) {
            continue;
        }

        sf::RectangleShape zoneShape(sf::Vector2f(zone.width, zone.height));
        zoneShape.setPosition(sf::Vector2f(currentArea->x + zone.x, currentArea->y + zone.y));
        zoneShape.setFillColor(toSfColor(zone.getColor()));
        m_window.draw(zoneShape);

        renderZoneGrid(currentArea->x + zone.x, currentArea->y + zone.y, 
                      zone.width, zone.height, toSfColor(zone.getGridColor()));
    }
}

void ClientRenderer::renderPlayers(const std::vector<PlayerState>& players, uint32_t clientId, bool isLocalPlayerCursed, float cursedTimer) {

    const MapArea* playerArea = nullptr;
    float playerX = 0, playerY = 0;

    for (const auto& player : players) {
        if (player.id == clientId) {
            playerX = player.x;
            playerY = player.y;
            if (m_map) {
                playerArea = m_map->getAreaAt(playerX, playerY);
            }
            break;
        }
    }

    for (const auto& player : players) {

        if (playerArea) {
            const MapArea* otherPlayerArea = m_map->getAreaAt(player.x, player.y);
            if (otherPlayerArea != playerArea) {
                continue; 
            }
        }

        if (!m_viewBounds.contains(sf::Vector2f(player.x, player.y))) {
            continue;
        }

        m_playerCircle.setRadius(player.radius);
        m_playerCircle.setOrigin(sf::Vector2f(player.radius, player.radius));
        m_playerCircle.setPosition(sf::Vector2f(player.x, player.y));

        sf::Color playerColor;

        if (player.id == clientId) {
            if (isLocalPlayerCursed) {
                float timeRatio = static_cast<float>(cursedTimer) / 0.5f; 
                playerColor = getCursedPlayerColor(timeRatio);
            }
            else {
                playerColor = sf::Color::Magenta;
            }
        } else {
            if (player.isCursed) {
                float timeRatio = static_cast<float>(player.cursedTimer) / 0.5f; 
                playerColor = getCursedPlayerColor(timeRatio);
            }
            else {
                playerColor = sf::Color::Magenta;   
            }
        }

        if (player.isDowned) {
            playerColor.a = 128; 
        }

        m_playerCircle.setFillColor(playerColor);
        m_window.draw(m_playerCircle);

        if (player.isDowned && player.downedTimer > 0) {
            std::string timerStr = std::to_string(static_cast<int>(player.downedTimer));

            m_timerText->setString(timerStr);
            m_timerText->setCharacterSize(static_cast<int>(14.0f * m_zoomFactor)); 
            m_timerText->setFillColor(sf::Color::Red); 

            sf::FloatRect textBounds = m_timerText->getLocalBounds();
            m_timerText->setPosition(sf::Vector2f(
                player.x - (textBounds.size.x / 2.0f + textBounds.position.x),
                player.y - (textBounds.size.y / 2.0f + textBounds.position.y)
            ));

            m_window.draw(*m_timerText);
        }

        if (player.isCursed && player.cursedTimer > 0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << player.cursedTimer;
            std::string timerStr = ss.str();

            m_timerText->setString(timerStr);
            m_timerText->setCharacterSize(static_cast<int>(10.0f * m_zoomFactor)); 
            m_timerText->setFillColor(sf::Color::White);
            m_timerText->setOutlineColor(sf::Color::Black);
            m_timerText->setOutlineThickness(3.0f);

            sf::FloatRect textBounds = m_timerText->getLocalBounds();
            m_timerText->setPosition(sf::Vector2f(
                player.x - (textBounds.size.x / 2.0f + textBounds.position.x),
                player.y - (textBounds.size.y / 2.0f + textBounds.position.y)
            ));

            m_window.draw(*m_timerText);
        }
    }
}
sf::Color ClientRenderer::getCursedPlayerColor(float timeRatio) const {

    timeRatio = std::max(0.0f, std::min(1.5f, timeRatio));

    sf::Color magenta(255, 0, 255);

    sf::Color cursedColor(0, 0, 0);

    uint8_t r = static_cast<uint8_t>(magenta.r * timeRatio + cursedColor.r * (1.0f - timeRatio));
    uint8_t g = static_cast<uint8_t>(magenta.g * timeRatio + cursedColor.g * (1.0f - timeRatio));
    uint8_t b = static_cast<uint8_t>(magenta.b * timeRatio + cursedColor.b * (1.0f - timeRatio));

    return sf::Color(r, g, b);
}

sf::Color ClientRenderer::getWaveringEnemyColor(float speed, float minSpeed, float maxSpeed, float changeProgress, bool isSpeedIncreasing) const {

    float nextSpeed = isSpeedIncreasing ? speed + 1.0f : speed - 1.0f;
    float currentSpeed = speed + (nextSpeed - speed) * changeProgress;

    sf::Color darkPurple(20, 10, 30);      
    sf::Color midPurple(86, 46, 117);      
    sf::Color lightPurple(240, 230, 255);  

    float normalizedSpeed = 30.0f * (currentSpeed - minSpeed) / (maxSpeed - minSpeed);

    if (normalizedSpeed <= 12.0f) {

        float ratio = normalizedSpeed / 12.0f;
        return sf::Color(
            static_cast<uint8_t>(darkPurple.r + (midPurple.r - darkPurple.r) * ratio),
            static_cast<uint8_t>(darkPurple.g + (midPurple.g - darkPurple.g) * ratio),
            static_cast<uint8_t>(darkPurple.b + (midPurple.b - darkPurple.b) * ratio)
        );
    } else {

        float ratio = std::min(1.0f, (normalizedSpeed - 12.0f) / 18.0f);
        return sf::Color(
            static_cast<uint8_t>(midPurple.r + (lightPurple.r - midPurple.r) * ratio),
            static_cast<uint8_t>(midPurple.g + (lightPurple.g - midPurple.g) * ratio),
            static_cast<uint8_t>(midPurple.b + (lightPurple.b - midPurple.b) * ratio)
        );
    }
}

void ClientRenderer::renderEnemies(const std::vector<EnemyState>& enemies) {
    if (!m_map || !m_currentPlayerArea) {
        return;
    }

    const MapArea* playerArea = m_currentPlayerArea;

    for (const auto& enemy : enemies) {

        const MapArea* enemyArea = m_map->getAreaAt(enemy.x, enemy.y);
        if (enemyArea != playerArea) {
            continue;
        }

        float maxRadius = std::max(enemy.radius, enemy.auraSize);
        if (!m_viewBounds.contains(sf::Vector2f(enemy.x - maxRadius, enemy.y - maxRadius)) && 
            !m_viewBounds.contains(sf::Vector2f(enemy.x + maxRadius, enemy.y + maxRadius))) {
            continue;
        }

        const float outlineWidth = 2.0f;

        m_enemyOutline.setRadius(enemy.radius);
        m_enemyOutline.setOrigin(sf::Vector2f(enemy.radius, enemy.radius));
        m_enemyOutline.setPosition(sf::Vector2f(enemy.x, enemy.y));

        sf::Color outlineColor = sf::Color::Black;
        if (enemy.isHarmless) {
            uint8_t alpha = static_cast<uint8_t>(255 - 205 * enemy.harmlessProgress);
            outlineColor.a = alpha;
        }
        m_enemyOutline.setFillColor(outlineColor);

        m_window.draw(m_enemyOutline);

        m_enemyCircle.setRadius(enemy.radius - outlineWidth);
        m_enemyCircle.setOrigin(sf::Vector2f(enemy.radius - outlineWidth, enemy.radius - outlineWidth));
        m_enemyCircle.setPosition(sf::Vector2f(enemy.x, enemy.y));

        sf::Color enemyColor;
        switch (enemy.type) {
            case 0: 
                enemyColor = sf::Color(128, 128, 128); 
                break;
            case 1: 
                enemyColor = sf::Color(87, 18, 31);    
                break;
            case 2: 
                enemyColor = sf::Color(22, 22, 22);    
                break;
            case 3: 
                enemyColor = sf::Color(255, 0, 0);

                if (enemy.auraSize > 0) {
                    m_enemyAura.setRadius(enemy.auraSize);
                    m_enemyAura.setOrigin(sf::Vector2f(enemy.auraSize, enemy.auraSize));
                    m_enemyAura.setPosition(sf::Vector2f(enemy.x, enemy.y));
                    m_enemyAura.setFillColor(sf::Color(255, 0, 0, 25));
                    m_window.draw(m_enemyAura);
                }
                break;
            case 4: 
                enemyColor = sf::Color(0, 0, 0);     
                break;
            case 5: 
                if (enemy.speed > 0.0f) {
                    enemyColor = getWaveringEnemyColor(
                        enemy.speed, 
                        enemy.minSpeed, 
                        enemy.maxSpeed,
                        enemy.changeProgress,
                        enemy.isSpeedIncreasing
                    );
                } else {
                    enemyColor = sf::Color(86, 46, 117);  
                }
                break;
            case 6: 
                enemyColor = sf::Color(255, 191, 124);   
                break;
            case 7: 
                enemyColor = sf::Color(66, 8, 8); 

                if (enemy.auraSize > 0) {
                    m_enemyAura.setRadius(enemy.auraSize);
                    m_enemyAura.setOrigin(sf::Vector2f(enemy.auraSize, enemy.auraSize));
                    m_enemyAura.setPosition(sf::Vector2f(enemy.x, enemy.y));
                    m_enemyAura.setFillColor(sf::Color(87, 16, 19, 25)); 
                    m_window.draw(m_enemyAura);
                }
                break;
            case 8: 
                enemyColor = sf::Color(160, 83, 83); 
                break;
            case 9: 
                enemyColor = sf::Color(160, 83, 83); 
                break;
            case 10: 
                enemyColor = sf::Color(0, 60, 102); 
                break;
            default:
                enemyColor = sf::Color(128, 128, 128);   
                break;
        }

        if (enemy.isHarmless) {
            uint8_t alpha = static_cast<uint8_t>(255 - 175 * enemy.harmlessProgress);
            enemyColor.a = alpha;
        }

        m_enemyCircle.setFillColor(enemyColor);
        m_window.draw(m_enemyCircle);
    }
}

void ClientRenderer::renderUI(const std::shared_ptr<GameMap>& map, const Vector2& playerPos, float fps) {

    sf::View uiView;
    uiView.setSize(sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
    uiView.setCenter(sf::Vector2f(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f));
    m_window.setView(uiView);

    if (map) {

        int currentAreaIndex = -1;
        const MapArea* currentArea = map->getAreaAt(playerPos.x, playerPos.y);

        if (currentArea) {
            for (size_t i = 0; i < map->areas.size(); i++) {
                if (&map->areas[i] == currentArea) {
                    currentAreaIndex = static_cast<int>(i);
                    break;
                }
            }
        }

        std::string displayText = map->name + ": Area " + std::to_string(currentAreaIndex + 1);

        m_mapAreaText->setString(displayText);
        int fontSize = static_cast<int>(32 * m_uiScale);
        m_mapAreaText->setCharacterSize(fontSize);

        sf::FloatRect textBounds = m_mapAreaText->getLocalBounds();
        float windowWidth = static_cast<float>(m_window.getSize().x);
        m_mapAreaText->setPosition(sf::Vector2f(
            (windowWidth - textBounds.size.x) / 2.0f,
            5.0f
        ));

        m_window.draw(*m_mapAreaText);
    }
}

void ClientRenderer::renderDownedTimer(uint8_t remainingSeconds) {

}

bool ClientRenderer::isPointInView(float x, float y) const {

    return m_viewBounds.contains(sf::Vector2f(x, y));
}

void ClientRenderer::renderDownedPlayerIndicators(const std::vector<PlayerState>& players, uint32_t clientId) {

    sf::Vector2f viewCenter = m_gameView.getCenter();
    sf::Vector2f viewSize = m_gameView.getSize();
    float viewLeft = viewCenter.x - viewSize.x / 2.0f;
    float viewRight = viewCenter.x + viewSize.x / 2.0f;
    float viewTop = viewCenter.y - viewSize.y / 2.0f;
    float viewBottom = viewCenter.y + viewSize.y / 2.0f;

    const float padding = 30.0f; 

    sf::Vector2f clientPos;
    bool foundClient = false;

    for (const auto& player : players) {
        if (player.id == clientId) {
            clientPos = sf::Vector2f(player.x, player.y);
            foundClient = true;
            break;
        }
    }

    if (!foundClient) return;

    for (const auto& player : players) {

        if (!player.isDowned || player.id == clientId) {
            continue;
        }

        if (m_map) {
            const MapArea* clientArea = m_map->getAreaAt(clientPos.x, clientPos.y);
            const MapArea* playerArea = m_map->getAreaAt(player.x, player.y);
            if (clientArea != playerArea) {
                continue;
            }
        }

        if (isPointInView(player.x, player.y)) {
            continue;  
        }

        sf::Vector2f direction(player.x - clientPos.x, player.y - clientPos.y);
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (distance < 1.0f) {
            continue;  
        }

        direction.x /= distance;
        direction.y /= distance;

        float intersectX, intersectY;

        if (std::abs(direction.x) > std::abs(direction.y)) {

            if (direction.x > 0) {
                intersectX = viewRight - padding;
            } else {
                intersectX = viewLeft + padding;
            }
            intersectY = clientPos.y + direction.y * (intersectX - clientPos.x) / direction.x;

            if (intersectY < viewTop + padding) {
                intersectY = viewTop + padding;
                intersectX = clientPos.x + direction.x * (intersectY - clientPos.y) / direction.y;
            } else if (intersectY > viewBottom - padding) {
                intersectY = viewBottom - padding;
                intersectX = clientPos.x + direction.x * (intersectY - clientPos.y) / direction.y;
            }
        } else {

            if (direction.y > 0) {
                intersectY = viewBottom - padding;
            } else {
                intersectY = viewTop + padding;
            }
            intersectX = clientPos.x + direction.x * (intersectY - clientPos.y) / direction.y;

            if (intersectX < viewLeft + padding) {
                intersectX = viewLeft + padding;
                intersectY = clientPos.y + direction.y * (intersectX - clientPos.x) / direction.x;
            } else if (intersectX > viewRight - padding) {
                intersectX = viewRight - padding;
                intersectY = clientPos.y + direction.y * (intersectX - clientPos.x) / direction.x;
            }
        }

        float angle = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;

        m_arrowShape.setPosition(sf::Vector2f(intersectX, intersectY));
        m_arrowShape.setRotation(sf::degrees(angle));

        m_window.draw(m_arrowShape);
    }
}

}