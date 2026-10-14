#pragma once

#include <Geode/Geode.hpp>
#include <util/GDObject.hpp>
#include <util/ObjectComparator.hpp>
#include <util/PixelPosition.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

struct TileVariant {
    std::vector<GDObject> objects;
    bool flipX;
    bool flipY;
    int rotation;
};

class ImageConverter {
private:

    static std::vector<std::vector<cocos2d::ccColor4B>> getFlippedTile(const std::vector<std::vector<cocos2d::ccColor4B>>& fullImg, int x0, int y0, int tw, int th, bool flipX, bool flipY, int rotation);

    static void undoFlippedTile(TileVariant& tile, int x0, int y0, int tw, int th);

    static std::vector<GDObject> tileToGDObjects(const std::vector<std::vector<cocos2d::ccColor4B>>& tile, int zLayer, int startingZOrder, int maxLinked, const std::unordered_map<uint32_t, int>& palette);

    static std::vector<GDObject> imgToGDObjects(const std::vector<std::vector<cocos2d::ccColor4B>>& image, std::unordered_map<uint32_t, int>& palette, int zLayer, int startingZOrder, int tileWidth = 0, int tileHeight = 0);

    static void assignZOrder2(std::vector<GDObject>& objects);

    static void assignZOrder(std::vector<GDObject>&);

    static std::vector<GDObject> removeRedundantObjects(const std::vector<GDObject>& objects, int width, int height);

    static void shrinkObjects2(std::vector<GDObject>& objects, int width, int height);

    static void shrinkObjects(std::vector<GDObject>& objects, int width, int height);

    static cocos2d::ccHSVValue RGBtoHSV(const cocos2d::ccColor4B& color);
public:
    static uint32_t getRGBKey(cocos2d::ccColor4B col);

    static void run(const std::string& path, float scale, int startColorID, int startingZOrder, int zLayer, int tileWidth, int tileHeight, bool createColTrigs, bool hsvMode);
};