#include <Geode/Geode.hpp>

#include <util/ObjectPlacer.hpp>
#include <util/ImageConverter.hpp>

using namespace geode::prelude;

ObjectComparator ObjectPlacer::placePixel(const std::vector<std::vector<cocos2d::ccColor4B>>& tile, const std::vector<GDObject>& gdObjects, const std::vector<std::vector<bool>>& processed, const std::vector<std::vector<int>>& cov, const std::unordered_map<uint32_t, int>& palette, int y, int x, int th, int tw, const uint32_t baseRGB, int zLayer, int startingZOrder, int maxLinked, const int colorID) {

    //const uint32_t baseRGB = ImageConverter::getRGBKey(baseCol); //converts the pixel's (and in turn, the object's) color into a unique int ID
    int rectWidth = 1;
    int rectHeight = 1;
    int minInsertID = -1; //the position in the list the object will be inserted at. this determines what objects it will be behind or infront of
    int revertX = 0;
    int revertY = 0;

    if (cov[y][x] > -1 && gdObjects[cov[y][x]].ID > minInsertID) { //makes sure the current object will be in front of any other object occupying this pixel
        minInsertID = gdObjects[cov[y][x]].ID;
    }

    //expansion right
    bool canExpandRight = true;
    int tempMinInsertID = minInsertID;
    while (canExpandRight && x + rectWidth < tw) {
        bool useful = false;
        const int nx = x + rectWidth;
        for (int i = 0; i < rectHeight; i++) {
            const int ny = y + i;
            const cocos2d::ccColor4B p = tile[ny][nx];
            const uint32_t pixelRGBKey = ImageConverter::getRGBKey(p);
            if (p.a != 255 || (processed[ny][nx] && pixelRGBKey != baseRGB)) { //cannot continue expanding if pixel is transparent or is a differently colored already processed pixel
                canExpandRight = false;
                break;
            }
            //if pixel is not processed and the color is the same then expanding out over that pixel is useful
            if (!processed[ny][nx] && pixelRGBKey == baseRGB) useful = true;

            if (cov[ny][nx] > -1) {
                tempMinInsertID = std::max(tempMinInsertID, gdObjects[cov[ny][nx]].ID);
            }
        }
        if (canExpandRight) {
            if (useful) {
                minInsertID = tempMinInsertID;
            }
            rectWidth++;
            revertX = useful ? 0 : revertX + 1;
        }
    }

    //special expansion down (same as right but it reverts width if optimal)
    bool canExpandDown = true;
    tempMinInsertID = minInsertID;
    while (canExpandDown && y + rectHeight < th) {
        bool useful = false;
        const int ny = y + rectHeight;
        for (int i = 0; i < rectWidth; i++) {
            const int nx = x + i;
            const int pixelsPastRevert = i - (rectWidth - revertX);
            const cocos2d::ccColor4B p = tile[ny][nx];
            const uint32_t pixelRGBKey = ImageConverter::getRGBKey(p);

            if (p.a != 255 || (processed[ny][nx] && pixelRGBKey != baseRGB)) {
                if (pixelsPastRevert >= 0) {
                    rectWidth -= revertX - pixelsPastRevert;
                    revertX = pixelsPastRevert;
                }
                else {
                    canExpandDown = false;
                }
                break;
            }
            if (!processed[ny][nx] && pixelRGBKey == baseRGB) {
                useful = true;
                if (pixelsPastRevert >= 0) {
                    revertX -= pixelsPastRevert + 1;
                }
            }
            if (cov[ny][nx] > -1)
                tempMinInsertID = std::max(tempMinInsertID, gdObjects[cov[ny][nx]].ID);
        }
        if (canExpandDown) {
            if (useful)
                minInsertID = tempMinInsertID;
            rectHeight++;
            revertY = useful ? 0 : revertY + 1;
        }
    }
    rectHeight -= revertY;
    revertY = 0;
    rectWidth -= revertX;
    revertX = 0;

    //expansion down and right at the same time (works the same as expansion right and expansion down, but its just done at the same time)
    bool canExpandX = true;
    bool canExpandY = true;
    tempMinInsertID = minInsertID;
    while (true) {
        bool grew = false;

        canExpandX = false;
        bool usefulCheckX = false;
        if (x + rectWidth < tw) {
            canExpandX = true;
            const int nx = x + rectWidth;
            for (int i = 0; i < rectHeight; i++) {
                const int ny = y + i;
                const cocos2d::ccColor4B p = tile[ny][nx];
                const uint32_t pixelRGBKey = ImageConverter::getRGBKey(p);
                if (p.a != 255 || (processed[ny][nx] && pixelRGBKey != baseRGB)) {
                    canExpandX = false;
                    if (cov[ny][nx] > -1 && gdObjects[cov[ny][nx]].ID > minInsertID)
                        minInsertID = gdObjects[cov[ny][nx]].ID;
                    break;
                }
                if (pixelRGBKey == baseRGB) usefulCheckX = true;

                if (cov[ny][nx] > -1) {
                    tempMinInsertID = std::max(tempMinInsertID, gdObjects[cov[ny][nx]].ID);
                }
            }
            if (canExpandX) {
                rectWidth++;
                grew = true;
            }
        }

        canExpandY = false;
        bool usefulCheckY = false;
        if (y + rectHeight < th) {
            canExpandY = true;
            int ny = y + rectHeight;
            for (int i = 0; i < rectWidth; i++) {
                int nx = x + i;
                cocos2d::ccColor4B p = tile[ny][nx];
                const uint32_t pixelRGBKey = ImageConverter::getRGBKey(p);
                if (p.a != 255 || (processed[ny][nx] && pixelRGBKey != baseRGB)) {
                    canExpandY = false;
                    if (cov[ny][nx] > -1 && gdObjects[cov[ny][nx]].ID > minInsertID)
                        minInsertID = gdObjects[cov[ny][nx]].ID;
                    break;
                }
                if (pixelRGBKey == baseRGB) usefulCheckY = true;

                if (cov[ny][nx] > -1) {
                    tempMinInsertID = std::max(tempMinInsertID, gdObjects[cov[ny][nx]].ID);
                }
            }
            if (canExpandY) {
                rectHeight++;
                grew = true;
            }
        }

        // Stop if nothing expanded
        if (!grew) {
            break;
        }

        if (canExpandX) {
            if (usefulCheckX) {
                revertX = 0;
                revertY = 0;
            }
            else revertX++;
        }
        if (canExpandY) {
            if (usefulCheckY) {
                revertX = 0;
                revertY = 0;
            }
            else revertY++;
        }

    }
    rectHeight -= revertY;
    rectWidth -= revertX;

    int insertKey = gdObjects.size();
    if (!gdObjects.empty()) {

        //expansion left
        bool canExpandLeft = true;
        while (canExpandLeft && x > 0) {
            int nx = x - 1;
            //check every pixel in the rects height
            for (int i = 0; i < rectHeight; i++) {
                int ny = y + i;
                cocos2d::ccColor4B p = tile[ny][nx];
                const uint32_t pixelRGBKey = ImageConverter::getRGBKey(p);

                if (p.a != 255) {
                    canExpandLeft = false;
                }
                else if (processed[ny][nx] && pixelRGBKey != baseRGB) {
                    if (minInsertID >= gdObjects[cov[ny][nx]].ID) {
                        canExpandLeft = false;
                    }
                    else if (gdObjects[cov[ny][nx]].ID < insertKey) {
                        insertKey = gdObjects[cov[ny][nx]].ID;
                    }
                }
            }
            if (canExpandLeft) {
                rectWidth++;
                x--;
            }
        }

        //expansion up
        bool canExpandUp = true;
        while (canExpandUp && y > 0) {
            int ny = y - 1;
            for (int i = 0; i < rectWidth; i++) {
                int nx = x + i;
                cocos2d::ccColor4B p = tile[ny][nx];
                const uint32_t pixelRGBKey = ImageConverter::getRGBKey(p);

                if (p.a != 255) {
                    canExpandUp = false;
                }
                else if (processed[ny][nx] && pixelRGBKey != baseRGB) {
                    if (minInsertID >= gdObjects[cov[ny][nx]].ID) {
                        canExpandUp = false;
                    }
                    else if (gdObjects[cov[ny][nx]].ID < insertKey) {
                        insertKey = gdObjects[cov[ny][nx]].ID;
                    }
                }
            }
            if (canExpandUp) {
                rectHeight++;
                y--;
            }
        }
    }

    int pixelsSaved = 0;
    for (int dy = 0; dy < rectHeight; dy++) {
        int ny = y + dy;
        for (int dx = 0; dx < rectWidth; dx++) {
            int nx = x + dx;
            cocos2d::ccColor4B p = tile[ny][nx];
            const uint32_t pixelRGBKey = ImageConverter::getRGBKey(p);
            if (pixelRGBKey == baseRGB && !processed[ny][nx]) {
                pixelsSaved++;
            }
        }
    }

    GDObject newObj = GDObject(x, y, rectWidth, rectHeight, colorID, zLayer, startingZOrder, maxLinked, insertKey);
    return ObjectComparator(newObj, pixelsSaved);
}

ObjectComparator ObjectPlacer::placeStair(const std::vector<std::vector<cocos2d::ccColor4B>>& tile, const std::vector<GDObject>& gdObjects, const std::vector<std::vector<bool>>& processed, const std::vector<std::vector<int>>& cov, const std::unordered_map<uint32_t, int>& palette, int y, int x, int th, int tw, const uint32_t baseRGB, int zLayer, int startingZOrder, int maxLinked, const int colorID) {
    
    if (y < 0 || x < 0 || y + 3 > th || x + 3 > tw) {
        return ObjectComparator(
            GDObject(x, y, 1, 1, palette.at(baseRGB), zLayer, startingZOrder, maxLinked, 0, ObjectType::STAIR, 0),
            -1
        );
    }
    
    
    
    int minInsertID = -1;
    int rotation = 0;
    int insertKey = gdObjects.size();

    if (cov[y][x] > -1 && gdObjects[cov[y][x]].ID > minInsertID) { //makes sure the current object will be in front of any other object occupying this pixel
        minInsertID = gdObjects[cov[y][x]].ID;
    }

    int pixelsSaved = 0;
    bool canPlace = true;
    for (uint8_t dy = 0; dy < 3 && canPlace; dy++) {
        int ny = y + dy;
        for (uint8_t dx = 0; dx < 3-dy; dx++) {
            int nx = x + dx;
            cocos2d::ccColor4B p = tile[ny][nx];
            const uint32_t pixelRGBKey = ImageConverter::getRGBKey(p);

            if (p.a != 255) {
                canPlace = false;
                break;
            }


            if (processed[ny][nx] && pixelRGBKey != baseRGB) {
                if (minInsertID >= gdObjects[cov[ny][nx]].ID) {
                    canPlace = false;
                    break;
                }
                else if (insertKey > gdObjects[cov[ny][nx]].ID) {
                    insertKey = gdObjects[cov[ny][nx]].ID;
                }
            }

            if (!processed[ny][nx] && pixelRGBKey == baseRGB) {
                pixelsSaved++;
            }

        }
    }

    if (!canPlace) {
        pixelsSaved = -1;
    }

    GDObject newObj = GDObject(x, y, 1, 1, colorID, zLayer, startingZOrder, maxLinked, insertKey, ObjectType::STAIR, rotation);
    return ObjectComparator(newObj, pixelsSaved);
}