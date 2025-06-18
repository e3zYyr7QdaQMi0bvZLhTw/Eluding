#pragma once

#include <SFML/Window.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "../../shared/include/protocol.h"

namespace evades {

class ClientInput {
public:

    ClientInput();

    void handleEvents(sf::RenderWindow& window, bool& running);

    void updateMouseMovement(sf::RenderWindow& window, float playerX, float playerY, float zoomFactor);

    void updateJoystickInput(sf::RenderWindow& window, float zoomFactor);
    
    bool hasZoomChanged() const { return m_zoomChanged; }
    float getZoomDelta() const { return m_zoomDelta; }
    bool shouldResetZoom() const { return m_resetZoom; }

    bool hasWindowResized() const { return m_windowResized; }
    unsigned int getResizedWidth() const { return m_resizedWidth; }
    unsigned int getResizedHeight() const { return m_resizedHeight; }

    bool shouldToggleFullscreen() const { return m_toggleFullscreen; }
    void clearFullscreenToggleFlag() { m_toggleFullscreen = false; }

    bool shouldResetPosition() const { return m_resetPosition; }
    void clearResetPositionFlag() { m_resetPosition = false; }

    bool shouldToggleDebugInfo() const { return m_toggleDebugInfo; }
    void clearDebugInfoToggleFlag() { m_toggleDebugInfo = false; }

    void clearZoomFlags() {
        m_zoomChanged = false;
        m_zoomDelta = 0.0f;
        m_resetZoom = false;
    }

    void clearResizeFlag() {
        m_windowResized = false;
    }

    const PlayerInput& getPlayerInput() const { return m_playerInput; }

private:

    PlayerInput m_playerInput;

    sf::Vector2f m_lastMousePos;
    float m_maxMouseMoveDistance; 

    bool m_zoomChanged;
    float m_zoomDelta;
    bool m_resetZoom;

    bool m_windowResized;
    unsigned int m_resizedWidth;
    unsigned int m_resizedHeight;
    
    bool m_toggleFullscreen;
    
    bool m_resetPosition;
    bool m_toggleDebugInfo;

    int m_activeJoystick;
    float m_joystickDeadzone;
    bool m_joystickEnabled;
};

} 