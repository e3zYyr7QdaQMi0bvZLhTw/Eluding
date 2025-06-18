#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <cstring>

namespace evades {

class GameMap; 

constexpr int DEFAULT_PORT = 12345;
constexpr float DEFAULT_PLAYER_RADIUS = 15.0f;
constexpr float DEFAULT_PLAYER_SPEED = 660.0f;

enum class MessageType : uint8_t {
    PlayerConnect = 0,
    PlayerDisconnect = 1,
    GameState = 2,
    PlayerInput = 3,
    Ping = 4,
    Pong = 5,
    MapData = 6,
    PlayerTeleport = 7,
    EnemyUpdate = 8,  
    PlayerDowned = 9,  
    PlayerCursed = 10,
    ResetPosition = 11
};

struct PlayerInput {
    bool moveUp = false;
    bool moveDown = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool isShiftPressed = false;

    bool isMouseControlEnabled = false;
    float mouseDirectionX = 0.0f;
    float mouseDirectionY = 0.0f;
    float mouseDistance = 0.0f;  

    bool isJoystickControlEnabled = false;
    float joystickDirectionX = 0.0f;
    float joystickDirectionY = 0.0f;
    float joystickDistance = 0.0f;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::PlayerInput));

        uint8_t inputBits = 0;
        if (moveUp) inputBits |= 1;
        if (moveDown) inputBits |= 2;
        if (moveLeft) inputBits |= 4;
        if (moveRight) inputBits |= 8;
        if (isMouseControlEnabled) inputBits |= 16;
        if (isShiftPressed) inputBits |= 32;
        if (isJoystickControlEnabled) inputBits |= 64;

        buffer.push_back(inputBits);

        if (isMouseControlEnabled) {
            buffer.resize(buffer.size() + sizeof(float) * 3);
            size_t offset = buffer.size() - (sizeof(float) * 3);

            memcpy(&buffer[offset], &mouseDirectionX, sizeof(float));
            offset += sizeof(float);

            memcpy(&buffer[offset], &mouseDirectionY, sizeof(float));
            offset += sizeof(float);

            memcpy(&buffer[offset], &mouseDistance, sizeof(float));
        }

        if (isJoystickControlEnabled) {
            buffer.resize(buffer.size() + sizeof(float) * 3);
            size_t offset = buffer.size() - (sizeof(float) * 3);

            memcpy(&buffer[offset], &joystickDirectionX, sizeof(float));
            offset += sizeof(float);

            memcpy(&buffer[offset], &joystickDirectionY, sizeof(float));
            offset += sizeof(float);

            memcpy(&buffer[offset], &joystickDistance, sizeof(float));
        }

        return buffer;
    }

    static PlayerInput deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
        PlayerInput input;
        uint8_t inputBits = buffer[offset++];

        input.moveUp = (inputBits & 1) != 0;
        input.moveDown = (inputBits & 2) != 0;
        input.moveLeft = (inputBits & 4) != 0;
        input.moveRight = (inputBits & 8) != 0;
        input.isMouseControlEnabled = (inputBits & 16) != 0;
        input.isShiftPressed = (inputBits & 32) != 0;
        input.isJoystickControlEnabled = (inputBits & 64) != 0;

        if (input.isMouseControlEnabled && offset + (sizeof(float) * 3) <= buffer.size()) {
            memcpy(&input.mouseDirectionX, &buffer[offset], sizeof(float));
            offset += sizeof(float);

            memcpy(&input.mouseDirectionY, &buffer[offset], sizeof(float));
            offset += sizeof(float);

            memcpy(&input.mouseDistance, &buffer[offset], sizeof(float));
            offset += sizeof(float);
        }

        if (input.isJoystickControlEnabled && offset + (sizeof(float) * 3) <= buffer.size()) {
            memcpy(&input.joystickDirectionX, &buffer[offset], sizeof(float));
            offset += sizeof(float);

            memcpy(&input.joystickDirectionY, &buffer[offset], sizeof(float));
            offset += sizeof(float);

            memcpy(&input.joystickDistance, &buffer[offset], sizeof(float));
            offset += sizeof(float);
        }

        return input;
    }
};

struct PlayerState {
    uint32_t id;
    float x;
    float y;
    float radius;
    bool isDowned = false;
    uint8_t downedTimer = 0;
    bool isCursed = false;
    float cursedTimer = 0.0f;

    void serialize(std::vector<uint8_t>& buffer) const {
        buffer.resize(buffer.size() + sizeof(id) + sizeof(x) + sizeof(y) + sizeof(radius) + sizeof(uint8_t) + sizeof(downedTimer) + sizeof(uint8_t) + sizeof(cursedTimer));
        size_t offset = buffer.size() - (sizeof(id) + sizeof(x) + sizeof(y) + sizeof(radius) + sizeof(uint8_t) + sizeof(downedTimer) + sizeof(uint8_t) + sizeof(cursedTimer));

        memcpy(&buffer[offset], &id, sizeof(id));
        offset += sizeof(id);

        memcpy(&buffer[offset], &x, sizeof(x));
        offset += sizeof(x);

        memcpy(&buffer[offset], &y, sizeof(y));
        offset += sizeof(y);

        memcpy(&buffer[offset], &radius, sizeof(radius));
        offset += sizeof(radius);

        uint8_t isDownedValue = isDowned ? 1 : 0;
        memcpy(&buffer[offset], &isDownedValue, sizeof(uint8_t));
        offset += sizeof(uint8_t);

        memcpy(&buffer[offset], &downedTimer, sizeof(downedTimer));
        offset += sizeof(downedTimer);

        uint8_t isCursedValue = isCursed ? 1 : 0;
        memcpy(&buffer[offset], &isCursedValue, sizeof(uint8_t));
        offset += sizeof(uint8_t);

        memcpy(&buffer[offset], &cursedTimer, sizeof(cursedTimer));
    }

    static PlayerState deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
        PlayerState state;

        memcpy(&state.id, &buffer[offset], sizeof(state.id));
        offset += sizeof(state.id);

        memcpy(&state.x, &buffer[offset], sizeof(state.x));
        offset += sizeof(state.x);

        memcpy(&state.y, &buffer[offset], sizeof(state.y));
        offset += sizeof(state.y);

        memcpy(&state.radius, &buffer[offset], sizeof(state.radius));
        offset += sizeof(state.radius);

        uint8_t isDownedValue;
        memcpy(&isDownedValue, &buffer[offset], sizeof(isDownedValue));
        state.isDowned = (isDownedValue != 0);
        offset += sizeof(uint8_t);

        memcpy(&state.downedTimer, &buffer[offset], sizeof(state.downedTimer));
        offset += sizeof(state.downedTimer);

        uint8_t isCursedValue;
        memcpy(&isCursedValue, &buffer[offset], sizeof(isCursedValue));
        state.isCursed = (isCursedValue != 0);
        offset += sizeof(uint8_t);

        memcpy(&state.cursedTimer, &buffer[offset], sizeof(state.cursedTimer));
        offset += sizeof(state.cursedTimer);

        return state;
    }
};

struct EnemyState {
    uint32_t id;
    float x;
    float y;
    float radius;
    uint8_t type;
    float speed = 0.0f;
    float minSpeed = 0.0f;
    float maxSpeed = 0.0f;
    float changeProgress = 0.0f;
    bool isSpeedIncreasing = true;
    bool isHarmless = false;
    float harmlessProgress = 0.0f;
    float auraSize = 0.0f;

    void serialize(std::vector<uint8_t>& buffer) const {
        buffer.resize(buffer.size() + sizeof(id) + sizeof(x) + sizeof(y) + sizeof(radius) + sizeof(type) + 
                     sizeof(speed) + sizeof(minSpeed) + sizeof(maxSpeed) + sizeof(changeProgress) + sizeof(uint8_t) +
                     sizeof(uint8_t) + sizeof(harmlessProgress) + sizeof(auraSize));
        size_t offset = buffer.size() - (sizeof(id) + sizeof(x) + sizeof(y) + sizeof(radius) + sizeof(type) + 
                     sizeof(speed) + sizeof(minSpeed) + sizeof(maxSpeed) + sizeof(changeProgress) + sizeof(uint8_t) +
                     sizeof(uint8_t) + sizeof(harmlessProgress) + sizeof(auraSize));

        memcpy(&buffer[offset], &id, sizeof(id));
        offset += sizeof(id);

        memcpy(&buffer[offset], &x, sizeof(x));
        offset += sizeof(x);

        memcpy(&buffer[offset], &y, sizeof(y));
        offset += sizeof(y);

        memcpy(&buffer[offset], &radius, sizeof(radius));
        offset += sizeof(radius);

        memcpy(&buffer[offset], &type, sizeof(type));
        offset += sizeof(type);
        
        memcpy(&buffer[offset], &speed, sizeof(speed));
        offset += sizeof(speed);
        
        memcpy(&buffer[offset], &minSpeed, sizeof(minSpeed));
        offset += sizeof(minSpeed);
        
        memcpy(&buffer[offset], &maxSpeed, sizeof(maxSpeed));
        offset += sizeof(maxSpeed);
        
        memcpy(&buffer[offset], &changeProgress, sizeof(changeProgress));
        offset += sizeof(changeProgress);
        
        uint8_t dirFlag = isSpeedIncreasing ? 1 : 0;
        memcpy(&buffer[offset], &dirFlag, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        
        uint8_t harmlessFlag = isHarmless ? 1 : 0;
        memcpy(&buffer[offset], &harmlessFlag, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        
        memcpy(&buffer[offset], &harmlessProgress, sizeof(harmlessProgress));
        offset += sizeof(harmlessProgress);
        
        memcpy(&buffer[offset], &auraSize, sizeof(auraSize));
    }

    static EnemyState deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
        EnemyState state;

        memcpy(&state.id, &buffer[offset], sizeof(state.id));
        offset += sizeof(state.id);

        memcpy(&state.x, &buffer[offset], sizeof(state.x));
        offset += sizeof(state.x);

        memcpy(&state.y, &buffer[offset], sizeof(state.y));
        offset += sizeof(state.y);

        memcpy(&state.radius, &buffer[offset], sizeof(state.radius));
        offset += sizeof(state.radius);

        memcpy(&state.type, &buffer[offset], sizeof(state.type));
        offset += sizeof(state.type);
        
        if (offset + sizeof(state.speed) <= buffer.size()) {
            memcpy(&state.speed, &buffer[offset], sizeof(state.speed));
            offset += sizeof(state.speed);
            
            memcpy(&state.minSpeed, &buffer[offset], sizeof(state.minSpeed));
            offset += sizeof(state.minSpeed);
            
            memcpy(&state.maxSpeed, &buffer[offset], sizeof(state.maxSpeed));
            offset += sizeof(state.maxSpeed);
            
            memcpy(&state.changeProgress, &buffer[offset], sizeof(state.changeProgress));
            offset += sizeof(state.changeProgress);
            
            uint8_t dirFlag = 0;
            memcpy(&dirFlag, &buffer[offset], sizeof(uint8_t));
            offset += sizeof(uint8_t);
            state.isSpeedIncreasing = (dirFlag != 0);
            
            if (offset + sizeof(uint8_t) <= buffer.size()) {
                uint8_t harmlessFlag = 0;
                memcpy(&harmlessFlag, &buffer[offset], sizeof(uint8_t));
                offset += sizeof(uint8_t);
                state.isHarmless = (harmlessFlag != 0);
                
                if (offset + sizeof(state.harmlessProgress) <= buffer.size()) {
                    memcpy(&state.harmlessProgress, &buffer[offset], sizeof(state.harmlessProgress));
                    offset += sizeof(state.harmlessProgress);
                    
                    if (offset + sizeof(state.auraSize) <= buffer.size()) {
                        memcpy(&state.auraSize, &buffer[offset], sizeof(state.auraSize));
                        offset += sizeof(state.auraSize);
                    }
                }
            }
        }

        return state;
    }
};

struct GameStatePacket {
    uint32_t tick;
    std::vector<PlayerState> players;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::GameState));

        buffer.resize(buffer.size() + sizeof(tick) + sizeof(uint16_t));
        size_t offset = 1;

        memcpy(&buffer[offset], &tick, sizeof(tick));
        offset += sizeof(tick);

        uint16_t playerCount = static_cast<uint16_t>(players.size());
        memcpy(&buffer[offset], &playerCount, sizeof(playerCount));
        offset += sizeof(playerCount);

        for (const auto& player : players) {
            player.serialize(buffer);
        }

        return buffer;
    }

    static GameStatePacket deserialize(const std::vector<uint8_t>& buffer) {
        GameStatePacket packet;
        size_t offset = 1; 

        memcpy(&packet.tick, &buffer[offset], sizeof(packet.tick));
        offset += sizeof(packet.tick);

        uint16_t playerCount;
        memcpy(&playerCount, &buffer[offset], sizeof(playerCount));
        offset += sizeof(playerCount);

        packet.players.resize(playerCount);
        for (uint16_t i = 0; i < playerCount; ++i) {
            packet.players[i] = PlayerState::deserialize(buffer, offset);
        }

        return packet;
    }
};

struct MapDataPacket {
    std::string mapJson;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::MapData));

        uint32_t jsonLength = static_cast<uint32_t>(mapJson.length());
        buffer.resize(buffer.size() + sizeof(jsonLength));
        memcpy(&buffer[1], &jsonLength, sizeof(jsonLength));

        buffer.insert(buffer.end(), mapJson.begin(), mapJson.end());

        return buffer;
    }

    static MapDataPacket deserialize(const std::vector<uint8_t>& buffer) {
        MapDataPacket packet;
        size_t offset = 1; 

        uint32_t jsonLength;
        memcpy(&jsonLength, &buffer[offset], sizeof(jsonLength));
        offset += sizeof(jsonLength);

        packet.mapJson.assign(
            reinterpret_cast<const char*>(&buffer[offset]), 
            jsonLength
        );

        return packet;
    }
};

struct PlayerTeleportPacket {
    uint32_t playerId;
    float x;
    float y;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::PlayerTeleport));

        buffer.resize(buffer.size() + sizeof(playerId) + sizeof(x) + sizeof(y));
        size_t offset = 1;

        memcpy(&buffer[offset], &playerId, sizeof(playerId));
        offset += sizeof(playerId);

        memcpy(&buffer[offset], &x, sizeof(x));
        offset += sizeof(x);

        memcpy(&buffer[offset], &y, sizeof(y));

        return buffer;
    }

    static PlayerTeleportPacket deserialize(const std::vector<uint8_t>& buffer) {
        PlayerTeleportPacket packet;
        size_t offset = 1; 

        memcpy(&packet.playerId, &buffer[offset], sizeof(packet.playerId));
        offset += sizeof(packet.playerId);

        memcpy(&packet.x, &buffer[offset], sizeof(packet.x));
        offset += sizeof(packet.x);

        memcpy(&packet.y, &buffer[offset], sizeof(packet.y));

        return packet;
    }
};

struct ConnectPacket {

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::PlayerConnect));
        return buffer;
    }
};

struct DisconnectPacket {

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::PlayerDisconnect));
        return buffer;
    }

    static DisconnectPacket deserialize(const std::vector<uint8_t>& buffer) {
        return DisconnectPacket();
    }
};

struct EnemyUpdatePacket {
    std::vector<EnemyState> enemies;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::EnemyUpdate));

        uint16_t enemyCount = static_cast<uint16_t>(enemies.size());
        buffer.resize(buffer.size() + sizeof(enemyCount));
        memcpy(&buffer[1], &enemyCount, sizeof(enemyCount));

        for (const auto& enemy : enemies) {
            enemy.serialize(buffer);
        }

        return buffer;
    }

    static EnemyUpdatePacket deserialize(const std::vector<uint8_t>& buffer) {
        EnemyUpdatePacket packet;
        size_t offset = 1; 

        uint16_t enemyCount;
        memcpy(&enemyCount, &buffer[offset], sizeof(enemyCount));
        offset += sizeof(enemyCount);

        for (uint16_t i = 0; i < enemyCount; ++i) {
            packet.enemies.push_back(EnemyState::deserialize(buffer, offset));
        }

        return packet;
    }
};

struct PlayerDownedPacket {
    uint32_t playerId;
    uint8_t isDown;
    uint8_t remainingSeconds;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::PlayerDowned));

        buffer.resize(buffer.size() + sizeof(playerId) + sizeof(isDown) + sizeof(remainingSeconds));
        size_t offset = 1;

        memcpy(&buffer[offset], &playerId, sizeof(playerId));
        offset += sizeof(playerId);

        memcpy(&buffer[offset], &isDown, sizeof(isDown));
        offset += sizeof(isDown);

        memcpy(&buffer[offset], &remainingSeconds, sizeof(remainingSeconds));

        return buffer;
    }

    static PlayerDownedPacket deserialize(const std::vector<uint8_t>& buffer) {
        PlayerDownedPacket packet;
        size_t offset = 1;

        memcpy(&packet.playerId, &buffer[offset], sizeof(packet.playerId));
        offset += sizeof(packet.playerId);

        memcpy(&packet.isDown, &buffer[offset], sizeof(packet.isDown));
        offset += sizeof(packet.isDown);

        memcpy(&packet.remainingSeconds, &buffer[offset], sizeof(packet.remainingSeconds));

        return packet;
    }
};

struct PlayerCursedPacket {
    uint32_t playerId;
    uint8_t isCursed;
    float remainingSeconds;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::PlayerCursed));

        buffer.resize(buffer.size() + sizeof(playerId) + sizeof(isCursed) + sizeof(remainingSeconds));
        size_t offset = 1;

        memcpy(&buffer[offset], &playerId, sizeof(playerId));
        offset += sizeof(playerId);

        memcpy(&buffer[offset], &isCursed, sizeof(isCursed));
        offset += sizeof(isCursed);

        memcpy(&buffer[offset], &remainingSeconds, sizeof(remainingSeconds));

        return buffer;
    }

    static PlayerCursedPacket deserialize(const std::vector<uint8_t>& buffer) {
        PlayerCursedPacket packet;
        size_t offset = 1;

        memcpy(&packet.playerId, &buffer[offset], sizeof(packet.playerId));
        offset += sizeof(packet.playerId);

        memcpy(&packet.isCursed, &buffer[offset], sizeof(packet.isCursed));
        offset += sizeof(packet.isCursed);

        memcpy(&packet.remainingSeconds, &buffer[offset], sizeof(packet.remainingSeconds));

        return packet;
    }
};

struct ResetPositionPacket {
    uint32_t playerId;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.push_back(static_cast<uint8_t>(MessageType::ResetPosition));
        
        buffer.resize(buffer.size() + sizeof(playerId));
        size_t offset = 1;
        
        memcpy(&buffer[offset], &playerId, sizeof(playerId));
        
        return buffer;
    }
    
    static ResetPositionPacket deserialize(const std::vector<uint8_t>& buffer) {
        ResetPositionPacket packet;
        size_t offset = 1;
        
        memcpy(&packet.playerId, &buffer[offset], sizeof(packet.playerId));
        
        return packet;
    }
};

} 