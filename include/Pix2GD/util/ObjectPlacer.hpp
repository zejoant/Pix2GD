#pragma once

#include <Geode/Geode.hpp>
#include <util/GDObject.hpp>
#include <util/ObjectComparator.hpp>
#include <util/ObjectType.hpp>

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace geode::prelude;

class ObjectPlacer {
public:
    static ObjectComparator placePixel(const std::vector<std::vector<cocos2d::ccColor4B>>& tile, const std::vector<GDObject>& gdObjects, const std::vector<std::vector<bool>>& processed, const std::vector<std::vector<int>>& cov, const std::unordered_map<uint32_t, int>& palette, int y, int x, int th, int tw, const uint32_t baseRGB, int zLayer, int startingZOrder, int maxLinked, const int colorID);
    
    static ObjectComparator placeStair(const std::vector<std::vector<cocos2d::ccColor4B>>& tile, const std::vector<GDObject>& gdObjects, const std::vector<std::vector<bool>>& processed, const std::vector<std::vector<int>>& cov, const std::unordered_map<uint32_t, int>& palette, int y, int x, int th, int tw, const uint32_t baseRGB, int zLayer, int startingZOrder, int maxLinked, const int colorID);

};