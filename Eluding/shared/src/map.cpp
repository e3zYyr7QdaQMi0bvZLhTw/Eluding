#include "../include/map.h"
#include "../include/game.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace evades {

// Utility function to convert RapidJSON document to string
std::string jsonToString(const rapidjson::Document& document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return buffer.GetString();
}

ZoneType stringToZoneType(const std::string& typeStr) {
    if (typeStr == "safe") return ZoneType::Safe;
    if (typeStr == "active") return ZoneType::Active;
    if (typeStr == "exit") return ZoneType::Exit;
    if (typeStr == "teleport") return ZoneType::Teleport;
    return ZoneType::Blocked;
}

std::string zoneTypeToString(ZoneType type) {
    switch (type) {
        case ZoneType::Safe: return "safe";
        case ZoneType::Active: return "active";
        case ZoneType::Exit: return "exit";
        case ZoneType::Teleport: return "teleport";
        case ZoneType::Blocked: return "blocked";
        default: return "unknown";
    }
}
Spawner Spawner::fromJson(const rapidjson::Value& j) {
    Spawner spawner;
    spawner.radius = j.HasMember("radius") ? j["radius"].GetFloat() : 12.0f;
    
    if (j.HasMember("speed")) {
        if (!j["speed"].IsNull()) {
            spawner.speed = j["speed"].GetFloat() / 1.20f;
        } else {
            if (j.HasMember("min_speed") && j.HasMember("max_speed")) {
                spawner.min_speed = j["min_speed"].GetFloat() / 1.20f;
                spawner.max_speed = j["max_speed"].GetFloat() / 1.20f;
                spawner.speed = (spawner.min_speed + spawner.max_speed) / 2.0f;
            } else {
                spawner.speed = 4.0f; // 5.0f / 1.25f
            }
        }
    } else {
        spawner.speed = 4.0f; // 5.0f / 1.25f
    }
    
    if (j.HasMember("min_speed")) {
        spawner.min_speed = j["min_speed"].GetFloat() / 1.20f;
    }
    
    if (j.HasMember("max_speed")) {
        spawner.max_speed = j["max_speed"].GetFloat() / 1.20f;
    }
    
    if (j.HasMember("move_clockwise")) {
        spawner.moveClockwise = j["move_clockwise"].GetBool();
    }
    
    spawner.count = j.HasMember("count") ? j["count"].GetInt() : 10;

    if (j.HasMember("types") && j["types"].IsArray()) {
        for (rapidjson::SizeType i = 0; i < j["types"].Size(); i++) {
            spawner.enemyTypes.push_back(j["types"][i].GetString());
        }
    } else if (j.HasMember("enemyTypes") && j["enemyTypes"].IsArray()) {
        for (rapidjson::SizeType i = 0; i < j["enemyTypes"].Size(); i++) {
            spawner.enemyTypes.push_back(j["enemyTypes"][i].GetString());
        }
    }

    return spawner;
}

void Spawner::toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const {
    j.SetObject();
    j.AddMember("radius", radius, allocator);
    j.AddMember("speed", speed, allocator);
    
    if (min_speed > 0.0f) {
        j.AddMember("min_speed", min_speed, allocator);
    }
    
    if (max_speed > 0.0f) {
        j.AddMember("max_speed", max_speed, allocator);
    }
    
    j.AddMember("move_clockwise", moveClockwise, allocator);
    
    j.AddMember("count", count, allocator);
    
    rapidjson::Value types(rapidjson::kArrayType);
    for (const auto& type : enemyTypes) {
        rapidjson::Value typeValue;
        typeValue.SetString(type.c_str(), type.length(), allocator);
        types.PushBack(typeValue, allocator);
    }
    j.AddMember("types", types, allocator);
}

ZoneProperties ZoneProperties::fromJson(const rapidjson::Value& j) {
    ZoneProperties props;
    if (j.HasMember("minimum_speed")) {
        props.minimumSpeed = j["minimum_speed"].GetFloat();
    }
    return props;
}

void ZoneProperties::toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const {
    j.SetObject();
    if (minimumSpeed > 0.0f) {
        j.AddMember("minimum_speed", minimumSpeed, allocator);
    }
}

Translation Translation::fromJson(const rapidjson::Value& j) {
    Translation trans;
    trans.x = j.HasMember("x") ? j["x"].GetFloat() : 0.0f;
    trans.y = j.HasMember("y") ? j["y"].GetFloat() : 0.0f;
    return trans;
}

void Translation::toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const {
    j.SetObject();
    j.AddMember("x", x, allocator);
    j.AddMember("y", y, allocator);
}

bool MapZone::containsPoint(float px, float py) const {
    return (px >= x && px < x + width && py >= y && py < y + height);
}

Color MapZone::getColor() const {
    switch (type) {
        case ZoneType::Safe: return getSafeZoneColor();
        case ZoneType::Active: return getActiveZoneColor();
        case ZoneType::Exit: return getExitZoneColor();
        case ZoneType::Teleport: return getTeleportZoneColor();
        default: return Color(0, 0, 0);
    }
}

Color MapZone::getGridColor() const {
    switch (type) {
        case ZoneType::Safe: return getSafeZoneGridColor();
        case ZoneType::Active: return getActiveZoneGridColor();
        case ZoneType::Exit: return getExitZoneGridColor();
        case ZoneType::Teleport: return getTeleportZoneGridColor();
        default: return Color(0, 0, 0);
    }
}

MapZone MapZone::fromJson(const rapidjson::Value& j, const MapZone* prevZone) {
    MapZone zone;

    if (j.HasMember("type")) {
        zone.type = stringToZoneType(j["type"].GetString());
    } else {
        zone.type = ZoneType::Blocked;
    }

    if (j.HasMember("x")) {
        if (j["x"].IsString()) {
            std::string xStr = j["x"].GetString();
            if (xStr == "last_right" && prevZone) {
                zone.x = prevZone->x + prevZone->width;
            } else {
                zone.x = 0.0f;
            }
        } else {
            zone.x = j["x"].GetFloat();
        }
    } else {
        zone.x = 0.0f;
    }

    if (j.HasMember("y")) {
        if (j["y"].IsString()) {
            std::string yStr = j["y"].GetString();
            if (yStr == "last_bottom" && prevZone) {
                zone.y = prevZone->y + prevZone->height;
            } else if (yStr == "last_y" && prevZone) {
                zone.y = prevZone->y;
            } else {
                zone.y = 0.0f;
            }
        } else {
            zone.y = j["y"].GetFloat();
        }
    } else {
        zone.y = 0.0f;
    }

    if (j.HasMember("width")) {
        if (j["width"].IsString()) {
            std::string widthStr = j["width"].GetString();
            if (widthStr == "last_width" && prevZone) {
                zone.width = prevZone->width;
            } else {
                zone.width = 0.0f;
            }
        } else {
            zone.width = j["width"].GetFloat();
        }
    } else {
        zone.width = 0.0f;
    }

    if (j.HasMember("height")) {
        if (j["height"].IsString()) {
            std::string heightStr = j["height"].GetString();
            if (heightStr == "last_height" && prevZone) {
                zone.height = prevZone->height;
            } else {
                zone.height = 0.0f;
            }
        } else {
            zone.height = j["height"].GetFloat();
        }
    } else {
        zone.height = 0.0f;
    }

    if (j.HasMember("properties") && j["properties"].IsObject()) {
        zone.properties = ZoneProperties::fromJson(j["properties"]);
    }

    if (j.HasMember("translate") && j["translate"].IsObject()) {
        zone.translate = Translation::fromJson(j["translate"]);
    }

    if (j.HasMember("spawner") && j["spawner"].IsArray()) {
        for (rapidjson::SizeType i = 0; i < j["spawner"].Size(); i++) {
            zone.spawners.push_back(Spawner::fromJson(j["spawner"][i]));
        }
    } else if (j.HasMember("spawners") && j["spawners"].IsArray()) {
        for (rapidjson::SizeType i = 0; i < j["spawners"].Size(); i++) {
            zone.spawners.push_back(Spawner::fromJson(j["spawners"][i]));
        }
    }

    return zone;
}

void MapZone::toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const {
    j.SetObject();
    
    rapidjson::Value typeStr;
    std::string typeString = zoneTypeToString(type);
    typeStr.SetString(typeString.c_str(), typeString.length(), allocator);
    j.AddMember("type", typeStr, allocator);
    
    j.AddMember("x", x, allocator);
    j.AddMember("y", y, allocator);
    j.AddMember("width", width, allocator);
    j.AddMember("height", height, allocator);

    if (type == ZoneType::Safe && properties.minimumSpeed > 0.0f) {
        rapidjson::Value propsValue;
        properties.toJson(propsValue, allocator);
        j.AddMember("properties", propsValue, allocator);
    }

    if (type == ZoneType::Exit) {
        rapidjson::Value transValue;
        translate.toJson(transValue, allocator);
        j.AddMember("translate", transValue, allocator);
    }

    if (type == ZoneType::Active && !spawners.empty()) {
        rapidjson::Value spawnersJson(rapidjson::kArrayType);
        for (const auto& spawner : spawners) {
            rapidjson::Value spawnerValue;
            spawner.toJson(spawnerValue, allocator);
            spawnersJson.PushBack(spawnerValue, allocator);
        }
        j.AddMember("spawner", spawnersJson, allocator);
    }
}

float MapArea::getWidth() const {
    float maxWidth = 0.0f;
    for (const auto& zone : zones) {
        maxWidth = std::max(maxWidth, zone.x + zone.width);
    }
    return maxWidth;
}

float MapArea::getHeight() const {
    float maxHeight = 0.0f;
    for (const auto& zone : zones) {
        maxHeight = std::max(maxHeight, zone.y + zone.height);
    }
    return maxHeight;
}

const MapZone* MapArea::getZoneAt(float px, float py) const {
    float relative_px = px - x;
    float relative_py = py - y;

    for (const auto& zone : zones) {
        if (zone.containsPoint(relative_px, relative_py)) {
            return &zone;
        }
    }
    return nullptr;
}

MapArea MapArea::fromJson(const rapidjson::Value& j, const MapArea* prevArea) {
    MapArea area;

    if (j.HasMember("x")) {
        if (j["x"].IsString()) {
            std::string xStr = j["x"].GetString();
            if (xStr == "last_right" && prevArea) {
                area.x = prevArea->x + prevArea->getWidth();
            } else {
                area.x = 0.0f;
            }
        } else {
            area.x = j["x"].GetFloat();
        }
    } else {
        area.x = 0.0f;
    }

    if (j.HasMember("y")) {
        if (j["y"].IsString()) {
            std::string yStr = j["y"].GetString();
            if (yStr == "last_bottom" && prevArea) {
                area.y = prevArea->y + prevArea->getHeight();
            } else if (yStr == "last_y" && prevArea) {
                area.y = prevArea->y;
            }
        } else {
            area.y = j["y"].GetFloat();
        }
    } else {
        area.y = 0.0f;
    }

    if (j.HasMember("zones") && j["zones"].IsArray()) {
        const MapZone* prevZone = nullptr;
        for (rapidjson::SizeType i = 0; i < j["zones"].Size(); i++) {
            area.zones.push_back(MapZone::fromJson(j["zones"][i], prevZone));
            prevZone = &area.zones.back();
        }
    }

    return area;
}

void MapArea::toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const {
    j.SetObject();
    j.AddMember("x", x, allocator);
    j.AddMember("y", y, allocator);

    rapidjson::Value zonesJson(rapidjson::kArrayType);
    for (const auto& zone : zones) {
        rapidjson::Value zoneValue;
        zone.toJson(zoneValue, allocator);
        zonesJson.PushBack(zoneValue, allocator);
    }
    j.AddMember("zones", zonesJson, allocator);
}

MapProperties MapProperties::fromJson(const rapidjson::Value& j) {
    MapProperties props;

    if (j.HasMember("background_color") && j["background_color"].IsArray() && j["background_color"].Size() >= 3) {
        auto& colorArray = j["background_color"];
        uint8_t r = colorArray[0].GetUint();
        uint8_t g = colorArray[1].GetUint();
        uint8_t b = colorArray[2].GetUint();
        uint8_t a = (colorArray.Size() > 3) ? colorArray[3].GetUint() : 255;
        props.backgroundColor = Color(r, g, b, a);
    } else {
        props.backgroundColor = Color(100, 100, 100); 
    }

    if (j.HasMember("friction")) {
        props.friction = j["friction"].GetFloat();
    } else {
        props.friction = 1.0f; 
    }

    return props;
}

void MapProperties::toJson(rapidjson::Value& j, rapidjson::Document::AllocatorType& allocator) const {
    j.SetObject();
    
    rapidjson::Value colorArray(rapidjson::kArrayType);
    colorArray.PushBack(rapidjson::Value(backgroundColor.r), allocator);
    colorArray.PushBack(rapidjson::Value(backgroundColor.g), allocator);
    colorArray.PushBack(rapidjson::Value(backgroundColor.b), allocator);
    colorArray.PushBack(rapidjson::Value(backgroundColor.a), allocator);
    
    j.AddMember("background_color", colorArray, allocator);
    j.AddMember("friction", friction, allocator);
}

const MapArea* GameMap::getAreaAt(float px, float py) const {
    for (size_t i = 0; i < areas.size(); ++i) {
        const auto& area = areas[i];
        float areaLeft = area.x;
        float areaTop = area.y;
        float areaRight = area.x + area.getWidth();
        float areaBottom = area.y + area.getHeight();

        if (px >= areaLeft && py >= areaTop && px < areaRight && py < areaBottom) {
            return &area;
        }
    }

    if (areas.size() > 0) {
        size_t closestIdx = 0;
        float minDistance = std::numeric_limits<float>::max();

        for (size_t i = 0; i < areas.size(); ++i) {
            const auto& area = areas[i];
            float areaLeft = area.x;
            float areaTop = area.y;
            float areaRight = area.x + area.getWidth();
            float areaBottom = area.y + area.getHeight();

            float closestX = std::max(areaLeft, std::min(px, areaRight));
            float closestY = std::max(areaTop, std::min(py, areaBottom));

            float dx = px - closestX;
            float dy = py - closestY;
            float distSquared = dx*dx + dy*dy;

            if (distSquared < minDistance) {
                minDistance = distSquared;
                closestIdx = i;
            }
        }

        std::cerr << "Point (" << px << "," << py << ") not in any area. " 
                  << "Closest is area " << closestIdx << " at distance " << std::sqrt(minDistance) << std::endl;
        std::cerr << "Area " << closestIdx << " bounds: (" 
                  << areas[closestIdx].x << "," << areas[closestIdx].y << ") to ("
                  << areas[closestIdx].x + areas[closestIdx].getWidth() << "," 
                  << areas[closestIdx].y + areas[closestIdx].getHeight() << ")" << std::endl;
    }

    return nullptr;
}

const MapZone* GameMap::getZoneAt(float px, float py) const {
    const MapArea* area = getAreaAt(px, py);
    if (area) {
        const MapZone* zone = area->getZoneAt(px, py);
        if (!zone) {
            std::cerr << "Point (" << px << "," << py << ") is in area at ("
                     << area->x << "," << area->y << ") but not in any zone." << std::endl;
        }
        return zone;
    }
    return nullptr;
}

Vector2 GameMap::findSpawnPoint() const {
    if (!areas.empty()) {
        const auto& firstArea = areas[30];
        for (const auto& zone : firstArea.zones) {
            if (zone.type == ZoneType::Safe) {
                return Vector2(
                    firstArea.x + zone.x + zone.width / 2.0f,
                    firstArea.y + zone.y + zone.height / 2.0f
                );
            }
        }
    }

    return Vector2(100.0f, 100.0f);
}

std::shared_ptr<GameMap> GameMap::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open map file: " << filename << std::endl;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromJson(buffer.str());
}

std::shared_ptr<GameMap> GameMap::loadFromJson(const std::string& jsonString) {
    try {
        rapidjson::Document doc;
        doc.Parse(jsonString.c_str());
        
        if (doc.HasParseError()) {
            std::cerr << "Failed to parse map JSON: ParseError at offset " 
                      << doc.GetErrorOffset() << std::endl;
            return nullptr;
        }
        
        auto map = std::make_shared<GameMap>(fromJson(doc));
        return map;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse map JSON: " << e.what() << std::endl;
        return nullptr;
    }
}

// Serializes the map to a JSON string
std::string GameMap::toJsonString() const {
    rapidjson::Document doc;
    toJson(doc);
    return jsonToString(doc);
}

GameMap GameMap::fromJson(const rapidjson::Document& j) {
    GameMap map;

    if (j.HasMember("name")) {
        map.name = j["name"].GetString();
    } else {
        map.name = "Unnamed Map";
    }

    if (j.HasMember("properties") && j["properties"].IsObject()) {
        map.properties = MapProperties::fromJson(j["properties"]);
    }

    if (j.HasMember("areas") && j["areas"].IsArray()) {
        const MapArea* prevArea = nullptr;
        for (rapidjson::SizeType i = 0; i < j["areas"].Size(); i++) {
            map.areas.push_back(MapArea::fromJson(j["areas"][i], prevArea));
            prevArea = &map.areas.back();
        }
    }

    return map;
}

void GameMap::toJson(rapidjson::Document& j) const {
    j.SetObject();
    
    rapidjson::Document::AllocatorType& allocator = j.GetAllocator();
    
    rapidjson::Value nameValue;
    nameValue.SetString(name.c_str(), name.length(), allocator);
    j.AddMember("name", nameValue, allocator);
    
    rapidjson::Value propsValue;
    properties.toJson(propsValue, allocator);
    j.AddMember("properties", propsValue, allocator);

    rapidjson::Value areasJson(rapidjson::kArrayType);
    for (const auto& area : areas) {
        rapidjson::Value areaValue;
        area.toJson(areaValue, allocator);
        areasJson.PushBack(areaValue, allocator);
    }
    j.AddMember("areas", areasJson, allocator);
}

AABB MapZone::getBoundaryAABB() const {
    return AABB::fromPositionAndSize(x, y, width, height);
}

std::vector<AABB> MapArea::getAllZoneAABBs() const {
    std::vector<AABB> allBoundaries;
    allBoundaries.reserve(zones.size());

    for (size_t i = 0; i < zones.size(); i++) {
        allBoundaries.push_back(zones[i].getBoundaryAABB());
    }

    return allBoundaries;
}

std::vector<AABB> GameMap::getAreaAABBs(float px, float py) const {
    const MapArea* area = getAreaAt(px, py);
    if (area) {
        return area->getAllZoneAABBs();
    }
    return {};
}

bool GameMap::resolveCollision(float& x, float& y, float radius, bool isEnemy) const {
    float originalX = x;
    float originalY = y;

    const MapArea* area = getAreaAt(x, y);

    if (!area) {
        float minDistance = std::numeric_limits<float>::max();
        const MapArea* closestArea = nullptr;
        float closestX = x, closestY = y;

        for (const auto& checkArea : areas) {
            float areaLeft = checkArea.x;
            float areaTop = checkArea.y;
            float areaRight = checkArea.x + checkArea.getWidth();
            float areaBottom = checkArea.y + checkArea.getHeight();

            float nearestX = std::max(areaLeft, std::min(x, areaRight));
            float nearestY = std::max(areaTop, std::min(y, areaBottom));

            float dx = x - nearestX;
            float dy = y - nearestY;
            float distance = std::sqrt(dx*dx + dy*dy);

            if (distance < minDistance) {
                minDistance = distance;
                closestArea = &checkArea;
                closestX = nearestX;
                closestY = nearestY;
            }
        }

        if (closestArea) {
            for (const auto& zone : closestArea->zones) {
                if (zone.type == ZoneType::Safe) {
                    x = closestArea->x + zone.x + zone.width/2.0f;
                    y = closestArea->y + zone.y + zone.height/2.0f;
                    return true;
                }
            }

            for (const auto& zone : closestArea->zones) {
                if (zone.type != ZoneType::Blocked) {
                    x = closestArea->x + zone.x + zone.width/2.0f;
                    y = closestArea->y + zone.y + zone.height/2.0f;
                    return true;
                }
            }
        }

        return false;
    }

    float areaLeft = area->x;
    float areaTop = area->y;
    float areaRight = area->x + area->getWidth();
    float areaBottom = area->y + area->getHeight();

    float adjustedLeft = areaLeft + radius;
    float adjustedTop = areaTop + radius;
    float adjustedRight = areaRight - radius;
    float adjustedBottom = areaBottom - radius;

    bool outsideAreaBounds = false;

    if (x < adjustedLeft) {
        x = adjustedLeft;
        outsideAreaBounds = true;
    }
    if (x > adjustedRight) {
        x = adjustedRight;
        outsideAreaBounds = true;
    }
    if (y < adjustedTop) {
        y = adjustedTop;
        outsideAreaBounds = true;
    }
    if (y > adjustedBottom) {
        y = adjustedBottom;
        outsideAreaBounds = true;
    }

    if (outsideAreaBounds) {
        return true;
    }

    if (isEnemy) {
        for (const auto& zone : area->zones) {
            if (zone.type != ZoneType::Safe && zone.type != ZoneType::Exit) {
                continue;  
            }

            float zoneLeft = area->x + zone.x;
            float zoneTop = area->y + zone.y;
            float zoneRight = zoneLeft + zone.width;
            float zoneBottom = zoneTop + zone.height;

            float expandedLeft = zoneLeft - radius;
            float expandedTop = zoneTop - radius;
            float expandedRight = zoneRight + radius;
            float expandedBottom = zoneBottom + radius;

            if (x >= expandedLeft && x <= expandedRight && y >= expandedTop && y <= expandedBottom) {
                float closestX = std::max(zoneLeft, std::min(x, zoneRight));
                float closestY = std::max(zoneTop, std::min(y, zoneBottom));

                float dx = x - closestX;
                float dy = y - closestY;
                float distance = std::sqrt(dx*dx + dy*dy);

                if (distance > 0) {
                    float pushX = dx / distance * (radius + 0.1f);  
                    float pushY = dy / distance * (radius + 0.1f);

                    x = closestX + pushX;
                    y = closestY + pushY;

                    x = std::max(adjustedLeft, std::min(x, adjustedRight));
                    y = std::max(adjustedTop, std::min(y, adjustedBottom));

                    return true;
                }
            }
        }
    } 
    else {
        for (const auto& zone : area->zones) {
            if (zone.type != ZoneType::Blocked) {
                continue;  
            }

            float zoneLeft = area->x + zone.x;
            float zoneTop = area->y + zone.y;
            float zoneRight = zoneLeft + zone.width;
            float zoneBottom = zoneTop + zone.height;

            float expandedLeft = zoneLeft - radius;
            float expandedTop = zoneTop - radius;
            float expandedRight = zoneRight + radius;
            float expandedBottom = zoneBottom + radius;

            if (x >= expandedLeft && x <= expandedRight && y >= expandedTop && y <= expandedBottom) {
                float closestX = std::max(zoneLeft, std::min(x, zoneRight));
                float closestY = std::max(zoneTop, std::min(y, zoneBottom));

                float dx = x - closestX;
                float dy = y - closestY;
                float distance = std::sqrt(dx*dx + dy*dy);

                if (distance > 0) {
                    float pushX = dx / distance * (radius + 0.1f);  
                    float pushY = dy / distance * (radius + 0.1f);

                    x = closestX + pushX;
                    y = closestY + pushY;

                    x = std::max(adjustedLeft, std::min(x, adjustedRight));
                    y = std::max(adjustedTop, std::min(y, adjustedBottom));

                    return true;
                }
            }
        }
    }

    return (x != originalX || y != originalY);
}

} 