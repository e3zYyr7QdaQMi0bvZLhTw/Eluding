#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../../include/rapidjson/document.h"
#include "../../include/rapidjson/writer.h"
#include "../../include/rapidjson/stringbuffer.h"
#include "game.h"

namespace evades {

struct MapZone;
struct MapArea;
struct GameMap;

enum class ZoneType {
    Safe,
    Active,
    Exit,
    Teleport,
    Blocked
};

struct Spawner {
    float radius;
    float speed;
    float min_speed = 0.0f;
    float max_speed = 0.0f;
    int count;
    std::vector<std::string> enemyTypes;
    bool moveClockwise = true;

    void serialize(std::vector<uint8_t>& buffer) const;
    static Spawner deserialize(const std::vector<uint8_t>& buffer, size_t& offset);

    static Spawner fromJson(const rapidjson::Value& j);
    void toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const;
};

struct ZoneProperties {
    float minimumSpeed = 0.0f;

    static ZoneProperties fromJson(const rapidjson::Value& j);
    void toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const;
};

struct Translation {
    float x;
    float y;

    static Translation fromJson(const rapidjson::Value& j);
    void toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const;
};

struct Color {
    uint8_t r, g, b, a;
    
    Color(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, uint8_t alpha = 255) 
        : r(red), g(green), b(blue), a(alpha) {}
};

struct MapZone {
    ZoneType type;
    float x;
    float y;
    float width;
    float height;
    ZoneProperties properties;
    Translation translate;
    std::vector<Spawner> spawners;

    bool containsPoint(float px, float py) const;

    AABB getBoundaryAABB() const;

    static Color getSafeZoneColor() { return Color(154, 166, 170); } // #9aa6aa
    static Color getSafeZoneGridColor() { return Color(130, 142, 146); } // #828e92
    static Color getActiveZoneColor() { return Color(191, 203, 207); } // #bfcbcf
    static Color getActiveZoneGridColor() { return Color(165, 177, 181); } // #a5b1b5
    static Color getExitZoneColor() { return Color(191, 196, 118); } // #bfc476
    static Color getExitZoneGridColor() { return Color(174, 174, 60); } // #aeae3c
    static Color getTeleportZoneColor() { return Color(64, 126, 135); } // #407e87
    static Color getTeleportZoneGridColor() { return Color(40, 101, 115); } // #286573

    Color getColor() const;
    Color getGridColor() const;

    void serialize(std::vector<uint8_t>& buffer) const;
    static MapZone deserialize(const std::vector<uint8_t>& buffer, size_t& offset);

    static MapZone fromJson(const rapidjson::Value& j, const MapZone* prevZone = nullptr);
    void toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const;
};

struct MapArea {
    float x;
    float y;
    std::vector<MapZone> zones;

    float getWidth() const;
    float getHeight() const;
    const MapZone* getZoneAt(float px, float py) const;

    std::vector<AABB> getAllZoneAABBs() const;

    void serialize(std::vector<uint8_t>& buffer) const;
    static MapArea deserialize(const std::vector<uint8_t>& buffer, size_t& offset);

    static MapArea fromJson(const rapidjson::Value& j, const MapArea* prevArea = nullptr);
    void toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const;
};

struct MapProperties {
    Color backgroundColor;
    float friction;

    static MapProperties fromJson(const rapidjson::Value& j);
    void toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const;
};

struct GameMap {
    std::string name;
    MapProperties properties;
    std::vector<MapArea> areas;

    const MapArea* getAreaAt(float px, float py) const;
    const MapZone* getZoneAt(float px, float py) const;
    Vector2 findSpawnPoint() const; 

    std::vector<AABB> getAreaAABBs(float px, float py) const;

    bool resolveCollision(float& x, float& y, float radius, bool isEnemy = false) const;

    static std::shared_ptr<GameMap> loadFromFile(const std::string& filename);
    static std::shared_ptr<GameMap> loadFromJson(const std::string& jsonString);

    std::vector<uint8_t> serialize() const;
    static GameMap deserialize(const std::vector<uint8_t>& buffer);

    static GameMap fromJson(const rapidjson::Document& j);
    void toJson(rapidjson::Document& j) const;
    std::string toJsonString() const;
};

} 