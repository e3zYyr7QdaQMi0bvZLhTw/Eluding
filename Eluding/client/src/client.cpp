#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #include <shellapi.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#endif

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include "../../shared/include/protocol.h"
#include "../../shared/include/network.h"
#include "../../shared/include/game.h"
#include "../../shared/include/map.h"

#include "client_renderer.h"
#include "client_network.h"
#include "client_input.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>

using namespace evades;

constexpr float DEFAULT_ZOOM_FACTOR = 1.2f;

class GameClient {
public:
    GameClient(const std::string& serverAddress, int serverPort) 
        : m_running(true),
          m_localPlayerPos(0.0f, 0.0f),
          m_clientId(0),
          m_fps(0.0f),
          m_zoomFactor(DEFAULT_ZOOM_FACTOR),
          m_isPlayerDowned(false),
          m_downedTimer(0),
          m_isPlayerCursed(false),
          m_cursedTimer(0.0f),
          m_isFullscreen(false),
          m_showDebugInfo(false),
          m_serverTPS(240) { 

        sf::Image icon;
        bool iconLoaded = false;

        if (icon.loadFromFile("icon.ico")) {
            iconLoaded = true;
            std::cout << "Loaded icon.ico from current directory" << std::endl;
        } else if (icon.loadFromFile("./build_client/icon.ico")) {
            iconLoaded = true;
            std::cout << "Loaded icon.ico from build_client directory" << std::endl;
        } else if (icon.loadFromFile("../client/src/icon.ico")) {
            iconLoaded = true;
            std::cout << "Loaded icon.ico from client/src directory" << std::endl;
        } else {
            std::cout << "Failed to load icon.ico" << std::endl;
        }

        sf::ContextSettings settings;
        settings.antiAliasingLevel = 8;

        sf::VideoMode mode;
        mode.size = sf::Vector2u(CLIENT_WIDTH, CLIENT_HEIGHT);
        mode.bitsPerPixel = 32;
        m_window.create(mode, "Evades", sf::State::Windowed, settings);
        m_window.setVerticalSyncEnabled(false);
        m_window.setFramerateLimit(0);

        if (iconLoaded) {
            m_window.setIcon(icon);
        }

        m_renderer = std::make_unique<ClientRenderer>(m_window);
        m_network = std::make_unique<ClientNetwork>(serverAddress, serverPort);
        m_input = std::make_unique<ClientInput>();

        m_network->setGameStateCallback([this](const GameStatePacket& state) {
            handleGameState(state);
        });

        m_network->setMapDataCallback([this](const MapDataPacket& mapData) {
            handleMapData(mapData);
        });

        m_network->setTeleportCallback([this](const PlayerTeleportPacket& teleport) {
            handlePlayerTeleport(teleport);
        });

        m_network->setEnemyUpdateCallback([this](const EnemyUpdatePacket& packet) {
            handleEnemyUpdate(packet);
        });

        m_network->setPlayerDownedCallback([this](const PlayerDownedPacket& packet) {
            handlePlayerDowned(packet);
        });

        m_network->setPlayerCursedCallback([this](const PlayerCursedPacket& packet) {
            handlePlayerCursed(packet);
        });

        m_renderer->setZoomFactor(m_zoomFactor);
    }

    void run() {

        m_network->connectToServer();

        sf::Clock clock;
        bool isRunning = m_running; 

        while (m_running && m_window.isOpen()) {
            float deltaTime = clock.restart().asSeconds();
            m_fps = 1.0f / (deltaTime > 0 ? deltaTime : 0.001f);

            m_input->handleEvents(m_window, isRunning);
            m_running = isRunning; 

            if (m_input->shouldToggleFullscreen()) {
                toggleFullscreen();
                m_input->clearFullscreenToggleFlag();
            }

            if (m_input->shouldResetPosition()) {
                requestPositionReset();
                m_input->clearResetPositionFlag();
            }

            if (m_input->shouldToggleDebugInfo()) {
                m_showDebugInfo = !m_showDebugInfo;
                std::cout << "Debug info " << (m_showDebugInfo ? "enabled" : "disabled") << std::endl;
                m_input->clearDebugInfoToggleFlag();
            }

            m_input->updateMouseMovement(m_window, m_localPlayerPos.x, m_localPlayerPos.y, m_zoomFactor);

            m_input->updateJoystickInput(m_window, m_zoomFactor);

            if (m_input->hasWindowResized()) {
                m_renderer->updateViewForResize(m_input->getResizedWidth(), m_input->getResizedHeight());
                m_input->clearResizeFlag();
            }

            if (m_input->hasZoomChanged()) {
                m_zoomFactor = std::max(1.0f, m_zoomFactor + m_input->getZoomDelta());
                m_renderer->setZoomFactor(m_zoomFactor);
                std::cout << "Zoom factor: " << m_zoomFactor << std::endl;
            }
            else if (m_input->shouldResetZoom()) {
                m_zoomFactor = DEFAULT_ZOOM_FACTOR;
                m_renderer->setZoomFactor(m_zoomFactor);
                std::cout << "Zoom reset to default: " << m_zoomFactor << std::endl;
            }

            m_input->clearZoomFlags();

            m_network->processNetworkMessages();

            m_network->sendPlayerInput(m_input->getPlayerInput());

            render();

            m_network->checkConnection();
        }

        m_network->stop();
    }

private:
    void handleGameState(const GameStatePacket& statePacket) {
        std::lock_guard<std::mutex> lock(m_gameStateMutex);

        m_players.clear();
        for (const auto& playerState : statePacket.players) {
            m_players.push_back(playerState);

            if (m_clientId == 0 && m_network->getClientId() != 0) {
                m_clientId = m_network->getClientId();
            }

            if (playerState.id == m_clientId) {
                m_localPlayerPos = Vector2(playerState.x, playerState.y);
                m_isPlayerDowned = playerState.isDowned;
                m_downedTimer = playerState.downedTimer;
                m_isPlayerCursed = playerState.isCursed;
                m_cursedTimer = playerState.cursedTimer;
            }
        }
    }

    void handleMapData(const MapDataPacket& mapPacket) {
        std::cout << "Received map data from server" << std::endl;

        m_map = GameMap::loadFromJson(mapPacket.mapJson);

        if (m_map) {
            std::cout << "Map loaded: " << m_map->name << std::endl;
        } else {
            std::cerr << "Failed to parse map data" << std::endl;
        }
    }

    void handlePlayerTeleport(const PlayerTeleportPacket& teleport) {
        if (teleport.playerId == m_clientId) {
            std::cout << "Teleported to: " << teleport.x << ", " << teleport.y << std::endl;

            std::lock_guard<std::mutex> lock(m_gameStateMutex);
            m_localPlayerPos = Vector2(teleport.x, teleport.y);

            for (auto& player : m_players) {
                if (player.id == m_clientId) {
                    player.x = teleport.x;
                    player.y = teleport.y;
                    break;
                }
            }
        }
    }

    void handleEnemyUpdate(const EnemyUpdatePacket& packet) {
        std::lock_guard<std::mutex> lock(m_gameStateMutex);

        m_enemies = packet.enemies;
    }

    void handlePlayerDowned(const PlayerDownedPacket& packet) {

        if (packet.playerId == m_clientId) {
            std::lock_guard<std::mutex> lock(m_gameStateMutex);

            m_isPlayerDowned = (packet.isDown != 0);
            m_downedTimer = packet.remainingSeconds;

            std::cout << "Downed state changed: " << (m_isPlayerDowned ? "downed" : "active") 
                      << ", timer: " << static_cast<int>(m_downedTimer) << std::endl;
        }
    }

    void handlePlayerCursed(const PlayerCursedPacket& packet) {

        if (packet.playerId == m_clientId) {
            std::lock_guard<std::mutex> lock(m_gameStateMutex);

            m_isPlayerCursed = (packet.isCursed != 0);
            m_cursedTimer = packet.remainingSeconds;

            std::cout << "Cursed state changed: " << (m_isPlayerCursed ? "cursed" : "normal") 
                      << ", timer: " << m_cursedTimer << std::endl;
        }
    }

    void requestPositionReset() {
        if (m_clientId == 0) {
            std::cout << "Cannot reset position: client ID not yet assigned" << std::endl;
            return;
        }

        ResetPositionPacket packet;
        packet.playerId = m_clientId;

        std::vector<uint8_t> data = packet.serialize();
        m_network->sendPacket(data);

        std::cout << "Position reset request sent to server" << std::endl;
    }

    void render() {

        m_renderer->beginFrame();

        m_renderer->centerViewOn(m_localPlayerPos.x, m_localPlayerPos.y);

        m_renderer->applyView();

        std::lock_guard<std::mutex> lock(m_gameStateMutex);

        m_renderer->renderMap(m_map, m_localPlayerPos);

        m_renderer->renderEnemies(m_enemies);

        m_renderer->renderPlayers(m_players, m_clientId, m_isPlayerCursed, m_cursedTimer);

        m_renderer->renderDownedPlayerIndicators(m_players, m_clientId);

        m_renderer->renderUI(m_map, m_localPlayerPos, m_fps);

        if (m_showDebugInfo) {
            renderDebugInfo();
        }

        m_renderer->endFrame();
    }

    void renderDebugInfo() {

        std::stringstream debugText;
        debugText << std::fixed << std::setprecision(1)
                  << "FPS: " << m_fps << std::endl
                  << "TPS: " << m_serverTPS;

        sf::Text debugInfoText(m_renderer->getFont(), debugText.str());
        debugInfoText.setCharacterSize(static_cast<unsigned int>(16 * m_renderer->getUiScale()));
        debugInfoText.setFillColor(sf::Color::White);
        debugInfoText.setOutlineColor(sf::Color::Black);
        debugInfoText.setOutlineThickness(1.0f);

        float padding = 10.0f * m_renderer->getUiScale();

        debugInfoText.setPosition(
            sf::Vector2f(m_window.getSize().x - debugInfoText.getLocalBounds().size.x - padding,
                        padding)
        );

        sf::View currentView = m_window.getView();
        sf::View uiView;
        uiView.setSize(sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
        uiView.setCenter(sf::Vector2f(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f));
        m_window.setView(uiView);

        m_window.draw(debugInfoText);

        m_window.setView(currentView);
    }

    void toggleFullscreen() {
        m_isFullscreen = !m_isFullscreen;
        sf::ContextSettings settings;
        settings.antiAliasingLevel = 8;

        sf::Image icon;
        bool iconLoaded = false;

        if (icon.loadFromFile("icon.ico")) {
            iconLoaded = true;
        } else if (icon.loadFromFile("./build_client/icon.ico")) {
            iconLoaded = true;
        } else if (icon.loadFromFile("../client/src/icon.ico")) {
            iconLoaded = true;
        }

        if (m_isFullscreen) {

            sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
            m_window.create(desktopMode, "Evades", sf::State::Fullscreen, settings);
            std::cout << "Switched to fullscreen mode: " << desktopMode.size.x << "x" << desktopMode.size.y << std::endl;
        } else {
            sf::VideoMode windowedMode;
            windowedMode.size = sf::Vector2u(CLIENT_WIDTH, CLIENT_HEIGHT);
            windowedMode.bitsPerPixel = 32;
            m_window.create(windowedMode, "Evades", sf::State::Windowed, settings);
            std::cout << "Switched to windowed mode: " << CLIENT_WIDTH << "x" << CLIENT_HEIGHT << std::endl;
        }

        if (iconLoaded) {
            m_window.setIcon(icon);
        }

        m_window.setVerticalSyncEnabled(false);
        m_window.setFramerateLimit(0);
        m_window.setMouseCursorVisible(true);  
        m_renderer->loadFonts(); 

        m_renderer->updateViewForResize(m_window.getSize().x, m_window.getSize().y);
    }

    sf::RenderWindow m_window;

    std::unique_ptr<ClientRenderer> m_renderer;
    std::unique_ptr<ClientNetwork> m_network;
    std::unique_ptr<ClientInput> m_input;

    std::atomic<bool> m_running;
    Vector2 m_localPlayerPos;
    uint32_t m_clientId;
    float m_fps;
    float m_zoomFactor;
    std::shared_ptr<GameMap> m_map;
    bool m_isFullscreen;
    bool m_showDebugInfo;
    int m_serverTPS;

    std::mutex m_gameStateMutex;
    std::vector<PlayerState> m_players;

    std::vector<EnemyState> m_enemies;

    bool m_isPlayerDowned;
    uint8_t m_downedTimer;

    bool m_isPlayerCursed;
    float m_cursedTimer;
};

#ifdef _WIN32

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {

    int argc = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        std::string arg(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, &arg[0], size_needed, NULL, NULL);
        arg.resize(strlen(arg.c_str())); 
        args.push_back(arg);
    }
    LocalFree(argvW);

    std::string serverAddress = "34.118.34.128";
    int serverPort = DEFAULT_PORT;

    if (argc > 1) {
        serverAddress = args[1];
    }

    if (argc > 2) {
        serverPort = std::atoi(args[2].c_str());
    }

    try {
        GameClient client(serverAddress, serverPort);
        client.run();
    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return 0;
}
#else

int main(int argc, char** argv) {
    std::string serverAddress = "34.118.34.128";
    int serverPort = DEFAULT_PORT;

    if (argc > 1) {
        serverAddress = argv[1];
    }

    if (argc > 2) {
        serverPort = std::atoi(argv[2]);
    }

    try {
        GameClient client(serverAddress, serverPort);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#endif