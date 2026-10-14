#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
using namespace geode::prelude;

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include <util/ImageConverter.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

uint32_t ImageConverter::getRGBKey(cocos2d::ccColor4B col) {
    return (uint32_t(col.r) << 16) | (uint32_t(col.g) << 8) | uint32_t(col.b);
}

std::vector<std::vector<cocos2d::ccColor4B>> ImageConverter::getFlippedTile(const std::vector<std::vector<cocos2d::ccColor4B>>& fullImg, int x0, int y0, int tw, int th, bool flipX, bool flipY) {
    std::vector<std::vector<cocos2d::ccColor4B>> tile(th, std::vector<cocos2d::ccColor4B>(tw));
    for (int y = 0; y < th; y++) {
        for (int x = 0; x < tw; x++) {
            int srcX = flipX ? (x0 + tw - 1 - x) : (x0 + x);
            int srcY = flipY ? (y0 + th - 1 - y) : (y0 + y);
            tile[y][x] = fullImg[srcY][srcX];
        }
    }
    return tile;
}

std::vector<GDObject> ImageConverter::tileToGDObjects(const std::vector<std::vector<cocos2d::ccColor4B>>& tile, int zLayer, int startingZOrder, int maxLinked, std::unordered_map<uint32_t, int>& palette) {
    bool strat1 = false;
    
    int th = tile.size();
    int tw = tile[0].size();

    std::vector<GDObject> gdObjects; //resulting gd object list
    std::vector<int> idOrder; //list of insert indexes for objects in gdObjects (the insert index is linked to the gd object by having the same index in their respective lists)
    std::vector<std::vector<bool>> processed(th, std::vector<bool>(tw, false)); //list of pixels that have been checked and has an object covering that pixel
    std::vector<std::vector<int>> cov(th, std::vector<int>(tw, -1)); //list of pixels and what visible object covers that pixel (an object is represented by its index in the gdObjects list)

    //loop through each pixel in the tile
    for (int y = 0; y < th; y++) {
        for (int x = 0; x < tw; x++) {
            if (processed[y][x]) continue;

            cocos2d::ccColor4B baseCol = tile[y][x];
            if (baseCol.a != 255) {
                processed[y][x] = true;
                continue;
            }

            uint32_t baseRGB = getRGBKey(baseCol); //converts the pixels color into a unique int ID
            int rectWidth = 1;
            int rectHeight = 1;
            float minInsertID = -1; //the position in the list the object will be inserted at. this determines what objects it will be behind or infront of
            int revertX = 0;
            int revertY = 0;

            if (cov[y][x] > -1 && idOrder[cov[y][x]] > minInsertID) { //makes sure the current object will be in front of any other object occupying this pixel
                minInsertID = idOrder[cov[y][x]];
            }
            
            //expansion right
            bool canExpandRight = true;
            while (canExpandRight && x + rectWidth < tw) {
                bool useful = false;
                int nx = x + rectWidth;
                for (int i = 0; i < rectHeight; i++) {
                    cocos2d::ccColor4B p = tile[y + i][nx];
                    const uint32_t pixelRGBKey = getRGBKey(p);
                    if (p.a != 255 || (processed[y + i][nx] && pixelRGBKey != baseRGB)) { //cannot continue expanding if pixel is transparent or is a differently colored already processed pixel
                        canExpandRight = false;
                        if (cov[y + i][nx] > -1 && idOrder[cov[y + i][nx]] > minInsertID) //makes sure the current object will be in front of any other object occupying this pixel
                            minInsertID = idOrder[cov[y + i][nx]];
                        break;
                    }
                    //if pixel is not processed and the color is the same then expanding out over that pixel is useful
                    if (!processed[y + i][nx] && pixelRGBKey == baseRGB) useful = true;
                }
                if (canExpandRight) {
                    rectWidth++;
                    revertX = useful ? 0 : revertX + 1;
                }
            }

            if (strat1) {
                rectWidth -= revertX; //revert back to the last useful pixel
                revertX = 0;

                
                //expansion down
                bool canExpandDown = true;
                while (canExpandDown && y + rectHeight < th) {
                    bool useful = false;
                    int ny = y + rectHeight;
                    for (int i = 0; i < rectWidth; i++) {
                        cocos2d::ccColor4B p = tile[ny][x + i];
                        const uint32_t pixelRGBKey = getRGBKey(p);
                        if (p.a != 255 || (processed[ny][x + i] && pixelRGBKey != baseRGB)) {
                            canExpandDown = false;
                            if (cov[ny][x + i] > -1 && idOrder[cov[ny][x + i]] > minInsertID)
                                minInsertID = idOrder[cov[ny][x + i]];
                            break;
                        }
                        if (!processed[ny][x + i] && pixelRGBKey == baseRGB) useful = true;
                    }
                    if (canExpandDown) {
                        rectHeight++;
                        revertY = useful ? 0 : revertY + 1;
                    }
                }
                rectHeight -= revertY;
                revertY = 0;
            }
            else {
                //special expansion down (same as right but it reverts width if optimal)
                bool canExpandDown = true;
                while (canExpandDown && y + rectHeight < th) {
                    bool useful = false;
                    int ny = y + rectHeight;
                    for (int i = 0; i < rectWidth; i++) {
                        int nx = x + i;
                        int pixelsPastRevert = i - (rectWidth - revertX);
                        cocos2d::ccColor4B p = tile[ny][nx];
                        const uint32_t pixelRGBKey = getRGBKey(p);

                        if (p.a != 255 || (processed[ny][nx] && pixelRGBKey != baseRGB)) {
                            if (pixelsPastRevert >= 0) {
                                rectWidth -= revertX - pixelsPastRevert;
                                revertX = pixelsPastRevert;
                            }
                            else {
                                canExpandDown = false;
                            }
                            if (cov[ny][nx] > -1 && idOrder[cov[ny][nx]] > minInsertID)
                                minInsertID = idOrder[cov[ny][nx]];
                            break;
                        }
                        if (!processed[ny][nx] && pixelRGBKey == baseRGB) {
                            useful = true;
                            if (pixelsPastRevert >= 0) {
                                revertX -= pixelsPastRevert + 1;
                            }
                        }
                    }
                    if (canExpandDown) {
                        rectHeight++;
                        revertY = useful ? 0 : revertY + 1;
                    }
                }
                rectHeight -= revertY;
                revertY = 0;
                rectWidth -= revertX;
                revertX = 0;
            }
            
            //expansion down and right at the same time (works the same as expansion right and expansion down, but its just done at the same time)
            bool canExpandX = true;
            bool canExpandY = true;

            while (true) {
                bool grew = false;

                canExpandX = false;
                bool usefulCheckX = false;
                if (x + rectWidth < tw) {
                    canExpandX = true;
                    int nx = x + rectWidth;
                    for (int i = 0; i < rectHeight; i++) {
                        int ny = y + i;
                        cocos2d::ccColor4B p = tile[ny][nx];
                        const uint32_t pixelRGBKey = getRGBKey(p);
                        if (p.a != 255 || (processed[ny][nx] && pixelRGBKey != baseRGB)) {
                            canExpandX = false;
                            if (cov[ny][nx] > -1 && idOrder[cov[ny][nx]] > minInsertID)
                                minInsertID = idOrder[cov[ny][nx]];
                            break;
                        }
                        else if (pixelRGBKey == baseRGB) usefulCheckX = true;
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
                        const uint32_t pixelRGBKey = getRGBKey(p);
                        if (p.a != 255 || (processed[ny][nx] && pixelRGBKey != baseRGB)) {
                            canExpandY = false;
                            if (cov[ny][nx] > -1 && idOrder[cov[ny][nx]] > minInsertID) {
                                minInsertID = idOrder[cov[ny][nx]];
                            }
                            break;
                        }
                        else if (pixelRGBKey == baseRGB) {
                            usefulCheckY = true;
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

            int tempX = x;
            int tempY = y;

            int insertKey = idOrder.size();
            if (!idOrder.empty()) {

                //expansion left
                bool canExpandLeft = true;
                while (canExpandLeft && x > 0) {
                    int nx = x - 1;
                    //check every pixel in the rects height
                    for (int i = 0; i < rectHeight; i++) {
                        int ny = y + i;
                        cocos2d::ccColor4B p = tile[ny][nx];
                        const uint32_t pixelRGBKey = getRGBKey(p);

                        if (p.a != 255) {
                            canExpandLeft = false;
                        }
                        else if (processed[ny][nx] && pixelRGBKey != baseRGB) {
                            if (minInsertID >= idOrder[cov[ny][nx]]) {
                                canExpandLeft = false;
                            }
                            else if (idOrder[cov[ny][nx]] < insertKey) {
                                insertKey = idOrder[cov[ny][nx]];
                            }
                        }
                        else if (pixelRGBKey == baseRGB) {
                            if (cov[ny][nx] > -1 && idOrder[cov[ny][nx]] > minInsertID) {
                                minInsertID = idOrder[cov[ny][nx]];
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
                        const uint32_t pixelRGBKey = getRGBKey(p);

                        if (p.a != 255) {
                            canExpandUp = false;
                        }
                        else if (processed[ny][nx] && pixelRGBKey != baseRGB) {
                            if (minInsertID >= idOrder[cov[ny][nx]]) {
                                canExpandUp = false;
                            }
                            else if (idOrder[cov[ny][nx]] < insertKey) {
                                insertKey = idOrder[cov[ny][nx]];
                            }
                        }
                        else if (pixelRGBKey == baseRGB) {
                            if (cov[ny][nx] > -1 && idOrder[cov[ny][nx]] > minInsertID) {
                                minInsertID = idOrder[cov[ny][nx]];
                            }
                        }
                    }
                    if (canExpandUp) {
                        rectHeight++;
                        y--;
                    }
                }
            }


            //populate the cov and processed lists. goes through the pixels of the to-be placed object and???
            for (int dy = 0; dy < rectHeight; dy++) {
                for (int dx = 0; dx < rectWidth; dx++) {
                    cocos2d::ccColor4B p = tile[y + dy][x + dx];
                    //pixel is set to processed if the pixel has the same color as the object and the objects covering this pixel has a lower z-order
                    //if (p.a != 0 && getRGBKey(p) == baseRGB && (cov[y + dy][x + dx] < 0 || insertKey > idOrder[cov[y + dy][x + dx]])) {
                    if (getRGBKey(p) == baseRGB && (cov[y + dy][x + dx] < 0 || insertKey > idOrder[cov[y + dy][x + dx]])) {
                        processed[y + dy][x + dx] = true;
                    }
                    //updates coverage for the pixel if the current object has the highest z-order for this pixel
                    if (cov[y + dy][x + dx] < 0 || insertKey > idOrder[cov[y + dy][x + dx]]) {
                        cov[y + dy][x + dx] = idOrder.size(); //index of object that is at the top of this pixel
                    }
                }
            }

            int colorID = palette[baseRGB];
            gdObjects.emplace_back(x, y, rectWidth, rectHeight, colorID, zLayer, startingZOrder, maxLinked, idOrder.size());
            for (int& id : idOrder) if (id >= insertKey) id++;
            idOrder.push_back(insertKey);

            x = tempX;
            y = tempY;
        }
    }

    //sort the list of gd objects based on their insert index
    std::sort(gdObjects.begin(), gdObjects.end(), [&](const GDObject& a, const GDObject& b) {
        return idOrder[a.ID] < idOrder[b.ID];
    });

    return gdObjects;
}

std::vector<GDObject> ImageConverter::imgToGDObjects(const std::vector<std::vector<cocos2d::ccColor4B>>& image, std::unordered_map<uint32_t, int>& palette, int zLayer, int startingZOrder, int tileWidth, int tileHeight) {
    int imgWidth = image[0].size();
    int imgHeight = image.size();
    if (tileWidth == 0) tileWidth = imgWidth;
    if (tileHeight == 0) tileHeight = imgHeight;

    std::vector<GDObject> totalObj;
    int tilesX = std::ceil((double)imgWidth / tileWidth);
    int tilesY = std::ceil((double)imgHeight / tileHeight);
    int maxLinked = 0;

    //loop through each tile and convert it to gd objects
    for (int ty = 0; ty < tilesY; ty++) {
        for (int tx = 0; tx < tilesX; tx++) {
            int x0 = tx * tileWidth;
            int y0 = ty * tileHeight;
            int tw = std::min(tileWidth, imgWidth - x0);
            int th = std::min(tileHeight, imgHeight - y0);
            maxLinked++;

            // Process the 4 orientations to see which is best
            auto tile = getFlippedTile(image, x0, y0, tw, th, false, false);
            auto objsN = removeRedundantObjects(tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette), tw, th);

            tile = getFlippedTile(image, x0, y0, tw, th, false, true);
            auto objsY = removeRedundantObjects(tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette), tw, th);

            tile = getFlippedTile(image, x0, y0, tw, th, true, false);
            auto objsX = removeRedundantObjects(tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette), tw, th);

            tile = getFlippedTile(image, x0, y0, tw, th, true, true);
            auto objsXY = removeRedundantObjects(tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette), tw, th);

            std::vector<GDObject>* best = &objsN;
            bool usedFlipX = false;
            bool usedFlipY = false;
            
            if (objsY.size() < best->size()) { //objsY is best
                best = &objsY;
                usedFlipX = false;
                usedFlipY = true;
            }
            if (objsX.size() < best->size()) { //objsX is best
                best = &objsX;
                usedFlipX = true;
                usedFlipY = false;
            }
            if (objsXY.size() < best->size()) { //objsXY is best
                best = &objsXY;
                usedFlipX = true;
                usedFlipY = true;
            }

            //*best = removeRedundantObjects(*best, tw, th);
            shrinkObjects2(*best, tw, th);
            assignZOrder(*best);

            //unflip the choosen best tile and put its objects in totalObj
            for (auto& o : *best) {
                if (usedFlipY) o.y = th - o.y - o.height;
                if (usedFlipX) o.x = tw - o.x - o.width;

                o.x += x0;
                o.y += y0;
                totalObj.push_back(o);
            }
        }
    }
    return totalObj;
}

void ImageConverter::assignZOrder(std::vector<GDObject>& objects) {
    for (size_t i = 0; i < objects.size(); i++) {
        GDObject& curr = objects[i];
        int maxZ = curr.zOrder;
        for (size_t j = 0; j < i; j++) {
            GDObject& prev = objects[j];
            // same color can be on the same z-order
            if (prev.color == curr.color) {
                continue;
            }
            bool overlapX = curr.x < prev.x + prev.width && curr.x + curr.width > prev.x;
            bool overlapY = curr.y < prev.y + prev.height && curr.y + curr.height > prev.y;
            if (overlapX && overlapY) {
                maxZ = std::max(maxZ, prev.zOrder + 1);
                if (maxZ == 0) {
                    maxZ++;
                }
            }
        }
        curr.zOrder = maxZ;
    }
}

std::vector<GDObject> ImageConverter::removeRedundantObjects(const std::vector<GDObject>& objects, int width, int height) {
    std::vector<std::vector<int>> cov(width, std::vector<int>(height, 0));
    std::vector<GDObject> nonRedundant;

    //build coverage
    for (size_t i = 0; i < objects.size(); i++) {
        const GDObject& obj = objects[i];
        for (int y = obj.y; y < obj.y + obj.height; y++) {
            for (int x = obj.x; x < obj.x + obj.width; x++) {
                if (cov[x][y] == 0 || objects[std::abs(cov[x][y]) - 1].color != obj.color) {
                    cov[x][y] = i + 1;
                }
                else {
                    cov[x][y] = -(int)(i + 1);
                }

            }
        }
    }

    //remove objects
    for (size_t i = 0; i < objects.size(); i++) {
        const GDObject& obj = objects[i];
        bool visible = false;

        for (int y = obj.y; y < obj.y + obj.height && !visible; y++) {
            for (int x = obj.x; x < obj.x + obj.width; x++) {

                if (std::abs(cov[x][y]) - 1 == (int)i && cov[x][y] > 0) {
                    nonRedundant.push_back(obj);
                    visible = true;
                    break;
                }

            }
        }
    }
    return nonRedundant;
}

void ImageConverter::shrinkObjects2(std::vector<GDObject>& objects, int width, int height) {
    std::vector<std::vector<int>> cov(width, std::vector<int>(height));

    //fill cov
    for (size_t i = 0; i < objects.size(); i++) {
        const GDObject& obj = objects[i];
        for (int y = obj.y; y < obj.y + obj.height; y++) {
            for (int x = obj.x; x < obj.x + obj.width; x++) {
                cov[x][y] = i;
            }
        }
    }

    for (size_t i = 0; i < objects.size(); i++) {
        GDObject& obj = objects[i];
        bool canShrink = true;

        //shrink top
        while (canShrink && obj.height > 1) {
            const int y = obj.y;
            for (size_t x = obj.x; x < obj.x + obj.width; x++) {
                if (cov[x][y] == i) {
                    canShrink = false;
                }
            }
            if (canShrink) {
                obj.y++;
                obj.height--;
            }
        }
        canShrink = true;

        //shrink bottom
        while (canShrink && obj.height > 1) {
            const int y = obj.y + obj.height - 1;
            for (size_t x = obj.x; x < obj.x + obj.width; x++) {
                if (cov[x][y] == i) {
                    canShrink = false;
                }
            }
            if (canShrink) {
                obj.height--;
            }
        }
        canShrink = true;

        //shrink left
        while (canShrink && obj.width > 1) {
            const int x = obj.x;
            for (size_t y = obj.y; y < obj.y + obj.height; y++) {
                if (cov[x][y] == i) {
                    canShrink = false;
                }
            }
            if (canShrink) {
                obj.x++;
                obj.width--;
            }
        }
        canShrink = true;

        //shrink right
        while (canShrink && obj.width > 1) {
            const int x = obj.x + obj.width - 1;
            for (size_t y = obj.y; y < obj.y + obj.height; y++) {
                if (cov[x][y] == i) {
                    canShrink = false;
                }
            }
            if (canShrink) {
                obj.width--;
            }
        }
    }
}

void ImageConverter::shrinkObjects(std::vector<GDObject>& objects, int width, int height) {
    std::vector<std::vector<std::vector<int>>> cov(width, std::vector<std::vector<int>>(height));

    //fill cov
    for (size_t i = 0; i < objects.size(); i++) {
        const GDObject& obj = objects[i];
        for (int y = obj.y; y < obj.y + obj.height; y++) {
            for (int x = obj.x; x < obj.x + obj.width; x++) {
                cov[x][y].push_back(i);
            }
        }
    }

    //shrink objects
    for (size_t i = 0; i < objects.size(); i++) {
        GDObject& obj = objects[i];
        bool canShrink = true;

        // shrink top
        while (canShrink && obj.height > 1) {
            int y = obj.y;
            for (int x = obj.x; x < obj.x + obj.width; x++) {
                auto& list = cov[x][y];
                int size = list.size();
                int ind = list[size - 1];
                if (ind == (int)i && (size == 1 || objects[list[size - 2]].color != obj.color)) {
                    canShrink = false;
                    break;
                }
            }
            if (canShrink) {
                obj.y++;
                obj.height--;
                for (int x = obj.x; x < obj.x + obj.width; x++) {
                    auto& list = cov[x][y];
                    list.erase(std::find(list.begin(), list.end(), i));
                }
            }
        }

        // shrink bottom
        canShrink = true;
        while (canShrink && obj.height > 1) {
            int y = obj.y + obj.height - 1;
            for (int x = obj.x; x < obj.x + obj.width; x++) {
                auto& list = cov[x][y];
                int size = list.size();
                int ind = list[size - 1];
                if (ind == (int)i && (size == 1 || objects[list[size - 2]].color != obj.color)) {
                    canShrink = false;
                    break;
                }
            }
            if (canShrink) {
                obj.height--;
                for (int x = obj.x; x < obj.x + obj.width; x++) {
                    auto& list = cov[x][y];
                    list.erase(std::find(list.begin(), list.end(), i));
                }
            }
        }

        // shrink left
        canShrink = true;
        while (canShrink && obj.width > 1) {
            int x = obj.x;
            for (int y = obj.y; y < obj.y + obj.height; y++) {
                auto& list = cov[x][y];
                int size = list.size();
                int ind = list[size - 1];
                if (ind == (int)i && (size == 1 || objects[list[size - 2]].color != obj.color)) {
                    canShrink = false;
                    break;
                }
            }
            if (canShrink) {
                obj.x++;
                obj.width--;
                for (int y = obj.y; y < obj.y + obj.height; y++) {
                    auto& list = cov[x][y];
                    list.erase(std::find(list.begin(), list.end(), i));
                }
            }
        }

        // shrink right
        canShrink = true;
        while (canShrink && obj.width > 1) {
            int x = obj.x + obj.width - 1;
            for (int y = obj.y; y < obj.y + obj.height; y++) {
                auto& list = cov[x][y];
                int size = list.size();
                int ind = list[size - 1];
                if (ind == (int)i && (size == 1 || objects[list[size - 2]].color != obj.color)) {
                    canShrink = false;
                    break;
                }
            }
            if (canShrink) {
                obj.width--;
                for (int y = obj.y; y < obj.y + obj.height; y++) {
                    auto& list = cov[x][y];
                    list.erase(std::find(list.begin(), list.end(), i));
                }
            }
        }
    }
}

cocos2d::ccHSVValue ImageConverter::RGBtoHSV(const cocos2d::ccColor4B& color){
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;

    float max = (std::max)(r, (std::max)(g, b));
    float min = (std::min)(r, (std::min)(g, b));
    float del = max - min;

    float hue = 0.0f, sat = 0.0f, vibe = max;

    if (del > 0.0f) {
        sat = del / max;
        if (max == r) {
            hue = 60.0f * ((g - b) / del);
        }
        else if (max == g) {
            hue = 60.0f * (((b - r) / del) + 2.0f);
        }
        else {
            hue = 60.0f * (((r - g) / del) + 4.0f);
        }
    }
    if (hue < 0.0f) {
        hue += 360.0f;
    }
    hue = ((static_cast<int>(hue) + 180) % 360) - 180;
    // hue 0 is goofy
    if (hue == 0.0f) hue++;

    cocos2d::ccHSVValue hsv;
    hsv.h = hue;
    hsv.s = sat;
    hsv.v = vibe;

    return hsv;
}

void ImageConverter::run(const std::string& path, float scale, int startColorID, int startingZOrder, int zLayer, int tileWidth, int tileHeight, bool createColTrigs, bool hsvMode) {
    int width, height, channels;
    //bool hsvMode = true;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        FLAlertLayer::create("Error", "Failed to load image", "OK")->show();
        return;
    }

    auto ui = EditorUI::get();

    std::unordered_map<uint32_t, int> palette;
    std::unordered_map<uint32_t, cocos2d::ccHSVValue> hsv;
    std::vector<std::vector<cocos2d::ccColor4B>> image(height, std::vector<cocos2d::ccColor4B>(width));

    //get color palette or hsv values of image
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int i = (y * width + x) * 4;
            cocos2d::ccColor4B pixelCol = { data[i + 0], data[i + 1], data[i + 2], data[i + 3] };
            image[y][x] = pixelCol;

            if (pixelCol.a == 255) {
                uint32_t key = (pixelCol.r << 16) | (pixelCol.g << 8) | pixelCol.b;
                int colorIndex;
                auto it = palette.find(key);

                //check if color already exists
                if (it != palette.end()) {
                    colorIndex = it->second;
                }
                else {
                    colorIndex = palette.size() + 1;
                    palette[key] = colorIndex;
                    if (hsvMode) {
                        hsv[startColorID + colorIndex - 1] = ImageConverter::RGBtoHSV(pixelCol);
                        continue;
                    }

                    if (createColTrigs) {
                        auto colTrigger = static_cast<EffectGameObject*>(ui->m_editorLayer->createObject(899, ccp(-15, 90+colorIndex*15), false));
                        colTrigger->m_targetColor = startColorID + colorIndex - 1;
                        colTrigger->m_triggerTargetColor = { pixelCol.r, pixelCol.g, pixelCol.b };
                        colTrigger->m_duration = 0;
                        auto label = colTrigger->getChildByType<CCLabelBMFont>(0);
                        label->setString(fmt::to_string(colTrigger->m_targetColor).c_str());
                    }
                    else {
                        ui->m_editorLayer->m_levelSettings->m_effectManager->setColorAction(ColorAction::create({ pixelCol.r, pixelCol.g, pixelCol.b }, false, 0), startColorID + colorIndex - 1);
                    }

                }
            }
        }
    }
    stbi_image_free(data);

    //convert image to gd objects
    auto gdObjects = ImageConverter::imgToGDObjects(image, palette, zLayer, startingZOrder, tileWidth, tileHeight);
    bool zOrderPassedZero = false;

    //every object with the same linkID is put in the same list, each id has its own list of objects belonging to that group
    std::unordered_map<int, cocos2d::CCArray*> groupedObjects;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    CCPoint center = ui->m_editorLayer->m_objectLayer->convertToNodeSpace(ccp(winSize.width / 2.f, winSize.height / 2.f));
    center.x = (int)((center.x - width * 3.75f * scale) / 30) * 30.f;
    center.y = (int)((center.y + height * 3.75f * scale) / 30) * 30.f;

    //creates and places the gd objects in the level
    for (const auto& o : gdObjects) { //1 block is 30 units. 1 pixel is 1/4 of a block, so its 7.5 units.
        float centerX = center.x + ((o.x + o.width / 2.f) * 7.5f * scale); //180
        float centerY = center.y - ((o.y + o.height / 2.f) * 7.5f * scale); //720

        if (o.zOrder == 0) zOrderPassedZero = true;

        auto gameObject = ui->m_editorLayer->createObject(917, CCPoint(centerX, centerY), false);
        if (hsvMode) {
            auto& hsvv = gameObject->m_baseColor->m_hsv;
            hsvv = hsv[startColorID + o.color - 1];

            hsvv.absoluteSaturation = true;
            hsvv.absoluteBrightness = true;

            gameObject->m_baseColor->m_colorID = 1010;
        }
        else {
            gameObject->m_baseColor->m_colorID = startColorID + o.color - 1;
        }

        gameObject->m_zOrder = zOrderPassedZero ? o.zOrder + 1 : o.zOrder;
        gameObject->updateCustomScaleX(o.width * scale + 2.f/300.f); //+2 pixel width to remove graphical glitch lines
        gameObject->updateCustomScaleY(o.height * scale + 2.f / 300.f); //+2 pixel height to remove graphical glitch lines
        gameObject->setCustomZLayer(5);
        gameObject->m_editorLayer = o.zLayer;
        
        if (groupedObjects.find(o.linkID) == groupedObjects.end()) {
            groupedObjects[o.linkID] = cocos2d::CCArray::create();
            groupedObjects[o.linkID]->retain();
        }

        groupedObjects[o.linkID]->addObject(gameObject);
    }

    //link objects
    auto gameLayer = GameManager::get()->m_gameLayer;
    for (auto& [linkID, arr] : groupedObjects) {
        gameLayer->groupStickyObjects(arr);
        arr->release();
    }
}
