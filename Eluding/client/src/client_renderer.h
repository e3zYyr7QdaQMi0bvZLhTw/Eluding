#pragma once

#include <SFML/Graphics.hpp>
#include "../../shared/include/game.h"
#include "../../shared/include/map.h"
#include "../../shared/include/protocol.h"

namespace evades {

sf::Color toSfColor(const Color& color);

class ClientRenderer {
public:
    ClientRenderer(sf::RenderWindow& window);

    void setZoomFactor(float zoomFactor);
    void updateViewForResize(unsigned int width, unsigned int height);
    void centerViewOn(float x, float y);
    void applyView();

    void beginFrame(const sf::Color& bgColor = sf::Color(51, 51, 51));
    void endFrame();

    void renderMap(const std::shared_ptr<GameMap>& map, const Vector2& playerPos);
    void renderPlayers(const std::vector<PlayerState>& players, uint32_t clientId, bool isLocalPlayerCursed = false, float cursedTimer = 0.0f);
    void renderEnemies(const std::vector<EnemyState>& enemies);
    void renderUI(const std::shared_ptr<GameMap>& map, const Vector2& playerPos, float fps);
    void renderDownedTimer(uint8_t remainingSeconds);
    void renderDownedPlayerIndicators(const std::vector<PlayerState>& players, uint32_t clientId);

    float getUiScale() const { return m_uiScale; }
    void setUiScale(float scale) { m_uiScale = scale; }

    bool loadFonts();

    const sf::Font& getFont() const { return m_font; }

private:
    void renderGrid();
    void renderZoneGrid(float x, float y, float width, float height, const sf::Color& gridColor);
    bool isPointInView(float x, float y) const;

    sf::Color getCursedPlayerColor(float timeRatio) const;
    sf::Color getWaveringEnemyColor(float speed, float minSpeed, float maxSpeed, float changeProgress, bool isSpeedIncreasing) const;

    sf::RenderWindow& m_window;
    sf::Font m_font;
    sf::View m_gameView;
    float m_zoomFactor;
    float m_uiScale;
    std::shared_ptr<GameMap> m_map;
    const MapArea* m_currentPlayerArea;

    sf::FloatRect m_viewBounds;

    sf::CircleShape m_enemyCircle;
    sf::CircleShape m_enemyOutline;
    sf::CircleShape m_enemyAura;
    sf::CircleShape m_playerCircle;

    sf::RectangleShape m_horizontalLine;
    sf::RectangleShape m_verticalLine;

    sf::ConvexShape m_arrowShape;

    std::unique_ptr<sf::Text> m_timerText;
    std::unique_ptr<sf::Text> m_mapAreaText;
};

}