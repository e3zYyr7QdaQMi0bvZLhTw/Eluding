#!/bin/bash

set -e

echo "Building server..."

mkdir -p build
cd build

g++ -std=c++17 -O2 -DSERVER_BUILD \
    -I../server/include \
    -I../server/include/Entities \
    -I../shared/include \
    -o server_game \
    ../server/src/GameServer.cpp \
    ../shared/src/map.cpp \
    ../server/src/AreaManager.cpp \
    ../server/src/EntityManager.cpp \
    ../server/src/PlayerManager.cpp \
    ../server/src/NetworkManager.cpp \
    ../server/src/Entities/Entity.cpp \
    ../server/src/Entities/NormalEnemy.cpp \
    ../server/src/Entities/CursedEnemy.cpp \
    ../server/src/Entities/WallEnemy.cpp \
    ../server/src/Entities/SlowingEnemy.cpp \
    ../server/src/Entities/ImmuneEnemy.cpp \
    ../server/src/Entities/WaveringEnemy.cpp \
    ../server/src/Entities/ExpanderEnemy.cpp \
    ../server/src/Entities/SilenceEnemy.cpp \
    ../server/src/Entities/SniperEnemy.cpp \
    ../server/src/Entities/SniperBullet.cpp \
    ../server/src/Entities/DasherEnemy.cpp \
    -lpthread
    
# Create maps directory and copy map files
mkdir -p maps
cp ../maps/*.json maps/ 2>/dev/null || true

echo "Server built successfully! Run ./server_game to start the server." 