#include "client_input.h"
#include <iostream>
#include <cmath>

namespace evades {

ClientInput::ClientInput()
    : m_zoomChanged(false),
      m_zoomDelta(0.0f),
      m_resetZoom(false),
      m_windowResized(false),
      m_resizedWidth(0),
      m_resizedHeight(0),
      m_toggleFullscreen(false),
      m_resetPosition(false),
      m_toggleDebugInfo(false),
      m_maxMouseMoveDistance(150.0f),
      m_activeJoystick(-1),
      m_joystickDeadzone(2.0f),
      m_joystickEnabled(false) { 

    m_playerInput.moveUp = false;
    m_playerInput.moveDown = false;
    m_playerInput.moveLeft = false;
    m_playerInput.moveRight = false;
    m_playerInput.isMouseControlEnabled = false;
    m_playerInput.mouseDirectionX = 0.0f;
    m_playerInput.mouseDirectionY = 0.0f;
    m_playerInput.mouseDistance = 0.0f;
    m_playerInput.isJoystickControlEnabled = false;
    m_playerInput.joystickDirectionX = 0.0f;
    m_playerInput.joystickDirectionY = 0.0f;
    m_playerInput.joystickDistance = 0.0f;

    m_lastMousePos = sf::Vector2f(0.0f, 0.0f);

    sf::Joystick::update();
    for (int i = 0; i < 8; i++) {
        if (sf::Joystick::isConnected(i)) {
            m_activeJoystick = i;
            m_joystickEnabled = true;
            std::cout << "Joystick " << i << " detected and active" << std::endl;

            auto ident = sf::Joystick::getIdentification(i);
            std::cout << "  Name: " << ident.name.toAnsiString() << std::endl;
            std::cout << "  Buttons: " << sf::Joystick::getButtonCount(i) << std::endl;
            std::cout << "  Has X axis: " << sf::Joystick::hasAxis(i, sf::Joystick::Axis::X) << std::endl;
            std::cout << "  Has Y axis: " << sf::Joystick::hasAxis(i, sf::Joystick::Axis::Y) << std::endl;
            break;
        }
    }
}

void ClientInput::handleEvents(sf::RenderWindow& window, bool& running) {
    m_zoomChanged = false;
    m_zoomDelta = 0.0f;
    m_resetZoom = false;
    m_windowResized = false;
    m_toggleFullscreen = false;
    m_resetPosition = false;
    m_toggleDebugInfo = false;

    while (auto event = window.pollEvent()) {
        if (const auto* closed = event->getIf<sf::Event::Closed>()) {
            std::cout << "Window close event received" << std::endl;
            window.close();
            running = false;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                std::cout << "Escape key pressed, exiting" << std::endl;
                window.close();
                running = false;
            }
            else if (keyPressed->code == sf::Keyboard::Key::Equal || 
                     keyPressed->code == sf::Keyboard::Key::Add) {
                m_zoomChanged = true;
                m_zoomDelta = -0.1f; 
            }
            else if (keyPressed->code == sf::Keyboard::Key::Hyphen || 
                     keyPressed->code == sf::Keyboard::Key::Subtract) {
                m_zoomChanged = true;
                m_zoomDelta = 0.1f; 
            }
            else if (keyPressed->code == sf::Keyboard::Key::Num0 || 
                     keyPressed->code == sf::Keyboard::Key::Numpad0) {
                m_resetZoom = true; 
            }
            else if (keyPressed->code == sf::Keyboard::Key::F11) {
                m_toggleFullscreen = true;
                std::cout << "Fullscreen toggle requested" << std::endl;
            }
            else if (keyPressed->code == sf::Keyboard::Key::R) {
                m_resetPosition = true;
                std::cout << "Position reset requested" << std::endl;
            }
            else if (keyPressed->code == sf::Keyboard::Key::T) {
                m_toggleDebugInfo = true;
                std::cout << "Debug info toggle requested" << std::endl;
            }
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button == sf::Mouse::Button::Left) {
                m_playerInput.isMouseControlEnabled = !m_playerInput.isMouseControlEnabled;

                if (m_playerInput.isMouseControlEnabled) {
                    m_playerInput.isJoystickControlEnabled = false;
                }

                std::cout << "Mouse control " 
                        << (m_playerInput.isMouseControlEnabled ? "enabled" : "disabled") 
                        << std::endl;
            }
        }

        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
            m_lastMousePos = window.mapPixelToCoords(mouseMoved->position);
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            m_windowResized = true;
            m_resizedWidth = resized->size.x;
            m_resizedHeight = resized->size.y;
        }

        if (const auto* joystickConnected = event->getIf<sf::Event::JoystickConnected>()) {
            m_activeJoystick = joystickConnected->joystickId;
            m_joystickEnabled = true;
            std::cout << "Joystick " << joystickConnected->joystickId << " connected" << std::endl;
        }

        if (const auto* joystickDisconnected = event->getIf<sf::Event::JoystickDisconnected>()) {
            if (m_activeJoystick == joystickDisconnected->joystickId) {
                m_joystickEnabled = false;
                m_playerInput.isJoystickControlEnabled = false;
                std::cout << "Active joystick disconnected" << std::endl;

                for (int i = 0; i < 8; i++) {
                    if (sf::Joystick::isConnected(i)) {
                        m_activeJoystick = i;
                        m_joystickEnabled = true;
                        std::cout << "Switched to joystick " << i << std::endl;
                        break;
                    }
                }
            }
        }

        if (const auto* joystickButtonPressed = event->getIf<sf::Event::JoystickButtonPressed>()) {
            if (joystickButtonPressed->joystickId == m_activeJoystick) {
                /*
                if (joystickButtonPressed->button == 0) { // Cross
                    m_playerInput.isShiftPressed = true;
                    std::cout << "Shift pressed via joystick" << std::endl;
                }
                */

                if (joystickButtonPressed->button == 9) { // Options
                    m_playerInput.isJoystickControlEnabled = !m_playerInput.isJoystickControlEnabled;

                    if (m_playerInput.isJoystickControlEnabled) {
                        m_playerInput.isMouseControlEnabled = false;
                    }

                    std::cout << "Joystick control " 
                            << (m_playerInput.isJoystickControlEnabled ? "enabled" : "disabled") 
                            << std::endl;
                }

                if (joystickButtonPressed->button == 3) { // Triangle
                    m_resetPosition = true;
                    std::cout << "Position reset requested via joystick" << std::endl;
                }
                if (joystickButtonPressed->button == 4) { // Left Bumper
                    m_joystickDeadzone += 1.0f;
                    std::cout << "Increment Joystick Deadzone via joystick" << std::endl;
                }
                if (joystickButtonPressed->button == 5) { // Right Bumper
                    m_joystickDeadzone -= 1.0f;
                    std::cout << "Decrement Joystick Deadzone via joystick" << std::endl;
                }
            }
        }
    }

    if (window.hasFocus() && !m_playerInput.isMouseControlEnabled && !m_playerInput.isJoystickControlEnabled) {
        m_playerInput.moveUp = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);

        m_playerInput.moveDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || 
                                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);

        m_playerInput.moveLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || 
                                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);

        m_playerInput.moveRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || 
                                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);
    }

    m_playerInput.isShiftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || 
                                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);

    if (!window.hasFocus()) {
        m_playerInput.moveUp = false;
        m_playerInput.moveDown = false;
        m_playerInput.moveLeft = false;
        m_playerInput.moveRight = false;
        m_playerInput.isShiftPressed = false;
    }

    if (!window.hasFocus() && m_playerInput.isMouseControlEnabled) {
        m_playerInput.mouseDirectionX = 0.0f;
        m_playerInput.mouseDirectionY = 0.0f;
        m_playerInput.mouseDistance = 0.0f;
    }

    if (!window.hasFocus() && m_playerInput.isJoystickControlEnabled) {
        m_playerInput.joystickDirectionX = 0.0f;
        m_playerInput.joystickDirectionY = 0.0f;
        m_playerInput.joystickDistance = 0.0f;
    }
}

void ClientInput::updateMouseMovement(sf::RenderWindow& window, float playerX, float playerY, float zoomFactor) {
    if (!m_playerInput.isMouseControlEnabled || !window.hasFocus()) {
        return;
    }
    sf::Vector2i mouseScreenPos = sf::Mouse::getPosition(window);
    sf::Vector2i windowCenter(window.getSize().x / 2, window.getSize().y / 2);

    float dirX = static_cast<float>(mouseScreenPos.x - windowCenter.x);
    float dirY = static_cast<float>(mouseScreenPos.y - windowCenter.y);
    float distance = std::sqrt(dirX * dirX + dirY * dirY);

    if (distance > 0.0f) {
    float invDistance = 1.0f / distance;
    float normalizedDirX = dirX * invDistance;
    float normalizedDirY = dirY * invDistance;

        m_playerInput.mouseDirectionX = normalizedDirX;
        m_playerInput.mouseDirectionY = normalizedDirY;
        float adjustedMaxDistance = m_maxMouseMoveDistance * zoomFactor;
        float distanceFactor = std::min(distance / adjustedMaxDistance, 1.0f);

        m_playerInput.mouseDistance = distanceFactor;
    } else {
        m_playerInput.mouseDirectionX = 0.0f;
        m_playerInput.mouseDirectionY = 0.0f;
        m_playerInput.mouseDistance = 0.0f;
    }
}
void ClientInput::updateJoystickInput(sf::RenderWindow& window, float zoomFactor) { 
    if (!m_joystickEnabled || !m_playerInput.isJoystickControlEnabled || m_activeJoystick < 0 ) {
        return;
    }

    sf::Joystick::update();

    if (!sf::Joystick::isConnected(m_activeJoystick)) {
        m_joystickEnabled = false;
        m_playerInput.isJoystickControlEnabled = false;
        return;
    }

    sf::Vector2i windowCenter(window.getSize().x / 2, window.getSize().y / 2);
    float joyX = 0.0f;
    float joyY = 0.0f;

    if (sf::Joystick::hasAxis(m_activeJoystick, sf::Joystick::Axis::X)) {
        joyX = static_cast<float>(((sf::Joystick::getAxisPosition(m_activeJoystick, sf::Joystick::Axis::X) /100.0f) * (window.getSize().x / 4)));
        //joyX = sf::Joystick::getAxisPosition(m_activeJoystick, sf::Joystick::Axis::X);
    }

    if (sf::Joystick::hasAxis(m_activeJoystick, sf::Joystick::Axis::Y)) {
        joyY = static_cast<float>(((sf::Joystick::getAxisPosition(m_activeJoystick, sf::Joystick::Axis::Y) / 100.0f) * (window.getSize().y / 4)));
        //joyY = sf::Joystick::getAxisPosition(m_activeJoystick, sf::Joystick::Axis::Y);
    }

    if (std::abs(joyX) < m_joystickDeadzone && std::abs(joyY) < m_joystickDeadzone) {
        joyX = 0.0f;
        joyY = 0.0f;
    } 
    
    float magnitude = std::sqrt(joyX * joyX + joyY * joyY);

    if (magnitude < 0.01f) {
        m_playerInput.joystickDirectionX = 0.0f;
        m_playerInput.joystickDirectionY = 0.0f;
        //m_playerInput.joystickDistance = 0.0f; // redundant as well
    } else {
        float normX = joyX / magnitude;
        float normY = joyY / magnitude;
        m_playerInput.joystickDirectionX = normX;
        m_playerInput.joystickDirectionY = normY;
    }
    // Yo when you get back, run it. Maybe I fixed it
    // Also the conversion from sfml joystick to jsgamepad joystick is: sfml/100 = js gamepadAPI (what evades uses)

    float adjustedMax = m_maxMouseMoveDistance * zoomFactor;

    float distanceFactor = std::min(magnitude / adjustedMax, 1.0f);
    m_playerInput.joystickDistance = distanceFactor;

    if (sf::Joystick::isButtonPressed(m_activeJoystick, 1)) { // Cross
        m_playerInput.isShiftPressed = true; 
        std::cout << "Shift pressed via joystick button" << std::endl;
    }

    /* Left / Right Triggers
    if (sf::Joystick::hasAxis(m_activeJoystick, sf::Joystick::Axis::Z) ||
        sf::Joystick::hasAxis(m_activeJoystick, sf::Joystick::Axis::R)) {

        float triggerValue = 0.0f;

        if (sf::Joystick::hasAxis(m_activeJoystick, sf::Joystick::Axis::Z)) {
            triggerValue = sf::Joystick::getAxisPosition(m_activeJoystick, sf::Joystick::Axis::Z);
        }

        if (sf::Joystick::hasAxis(m_activeJoystick, sf::Joystick::Axis::R)) {
            triggerValue = std::max(triggerValue, sf::Joystick::getAxisPosition(m_activeJoystick, sf::Joystick::Axis::R));
        }

        //m_playerInput.isShiftPressed = (triggerValue > 50.0f); 
    }
    */
}

}