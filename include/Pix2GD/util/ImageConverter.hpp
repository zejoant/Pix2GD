#pragma once

#include <Geode/Geode.hpp>
using namespace geode::prelude;

struct GDObject {
    int x, y, width, height;
    int color, zLayer, zOrder;
    int linkID, ID;

    GDObject(int x, int y, int w, int h, int c, int zl, int zo, int lid, int id)
        : x(x), y(y), width(w), height(h), color(c), zLayer(zl), zOrder(zo), linkID(lid), ID(id) {
    }
};

class ImageConverter {
public:
    static uint32_t getRGBKey(cocos2d::ccColor4B col);

    static std::vector<std::vector<cocos2d::ccColor4B>> getFlippedTile(const std::vector<std::vector<cocos2d::ccColor4B>>& fullImg, int x0, int y0, int tw, int th, bool flipX, bool flipY);

    static std::vector<GDObject> tileToGDObjects(const std::vector<std::vector<cocos2d::ccColor4B>>& tile, int zLayer, int startingZOrder, int maxLinked, std::unordered_map<uint32_t, int>& palette);

    static std::vector<GDObject> imgToGDObjects(const std::vector<std::vector<cocos2d::ccColor4B>>& image, std::unordered_map<uint32_t, int>& palette, int zLayer, int startingZOrder, int tileWidth = 0, int tileHeight = 0);

    static void assignZOrder(std::vector<GDObject>&);

    static std::vector<GDObject> removeRedundantObjects(const std::vector<GDObject>& objects, int width, int height);

    static void shrinkObjects(std::vector<GDObject>& objects, int width, int height);

    static void run(const std::string& path, float scale, int startColorID, int startingZOrder, int zLayer, int tileWidth, int tileHeight, bool createColTrigs);

};