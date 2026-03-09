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
    return (col.r << 16) | (col.g << 8) | col.b;
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
    int th = tile.size();
    int tw = tile[0].size();

    std::vector<GDObject> gdObjects;
    std::vector<int> idOrder;
    std::vector<std::vector<bool>> processed(th, std::vector<bool>(tw, false));
    std::vector<std::vector<int>> cov(th, std::vector<int>(tw, -1));

    for (int y = 0; y < th; y++) {
        for (int x = 0; x < tw; x++) {
            if (processed[y][x]) continue;

            cocos2d::ccColor4B baseCol = tile[y][x];
            if (baseCol.a != 255) {
                processed[y][x] = true;
                continue;
            }

            uint32_t baseRGB = getRGBKey(baseCol);
            int rectWidth = 1;
            int rectHeight = 1;
            float minInsertID = -1;
            int revertCount = 0;

            if (cov[y][x] > -1 && idOrder[cov[y][x]] > minInsertID) {
                minInsertID = idOrder[cov[y][x]];
            }

            //expansion right
            bool canExpandRight = true;
            while (canExpandRight && x + rectWidth < tw) {
                bool useful = false;
                int nx = x + rectWidth;
                for (int i = 0; i < rectHeight; i++) {
                    cocos2d::ccColor4B p = tile[y + i][nx];
                    if (p.a != 255 || (processed[y + i][nx] && getRGBKey(p) != baseRGB)) {
                        canExpandRight = false;
                        if (cov[y + i][nx] > -1 && idOrder[cov[y + i][nx]] > minInsertID)
                            minInsertID = idOrder[cov[y + i][nx]];
                        break;
                    }
                    if (!processed[y + i][nx] && getRGBKey(p) == baseRGB) useful = true;
                }
                if (canExpandRight) {
                    rectWidth++;
                    revertCount = useful ? 0 : revertCount + 1;
                }
            }
            rectWidth -= revertCount;
            revertCount = 0;

            //expansion down
            bool canExpandDown = true;
            while (canExpandDown && y + rectHeight < th) {
                bool useful = false;
                int ny = y + rectHeight;
                for (int i = 0; i < rectWidth; i++) {
                    cocos2d::ccColor4B p = tile[ny][x + i];
                    if (p.a != 255 || (processed[ny][x + i] && getRGBKey(p) != baseRGB)) {
                        canExpandDown = false;
                        if (cov[ny][x + i] > -1 && idOrder[cov[ny][x + i]] > minInsertID)
                            minInsertID = idOrder[cov[ny][x + i]];
                        break;
                    }
                    if (!processed[ny][x + i] && getRGBKey(p) == baseRGB) useful = true;
                }
                if (canExpandDown) {
                    rectHeight++;
                    revertCount = useful ? 0 : revertCount + 1;
                }
            }
            rectHeight -= revertCount;
            revertCount = 0;

            //expansion down and right at the same time
            boolean canExpandX = true;
            boolean canExpandY = true;
            int revertX = 0;
            int revertY = 0;
            while (true) {
                boolean grew = false;

                canExpandX = false;
                boolean usefulCheckX = false;
                if (x + rectWidth < tw) {
                    canExpandX = true;
                    int nx = x + rectWidth;
                    for (int i = 0; i < rectHeight; i++) {
                        int ny = y + i;
                        cocos2d::ccColor4B p = tile[ny][nx];
                        if (p.a != 255 || (processed[ny][nx] && getRGBKey(p) != baseRGB)) {
                            canExpandX = false;
                            if (cov[ny][nx] > -1 && idOrder[cov[ny][nx]] > minInsertID)
                                minInsertID = idOrder[cov[ny][nx]];
                            break;
                        }
                        else if (getRGBKey(p) == baseRGB) usefulCheckX = true;
                    }
                    if (canExpandX) {
                        rectWidth++;
                        grew = true;
                    }
                }

                canExpandY = false;
                boolean usefulCheckY = false;
                if (y + rectHeight < th) {
                    canExpandY = true;
                    int ny = y + rectHeight;
                    for (int i = 0; i < rectWidth; i++) {
                        int nx = x + i;
                        cocos2d::ccColor4B p = tile[ny][nx];
                        if (p.a != 255 || (processed[ny][nx] && getRGBKey(p) != baseRGB)) {
                            canExpandY = false;
                            if (cov[ny][nx] > -1 && idOrder[cov[ny][nx]] > minInsertID) {
                                minInsertID = idOrder[cov[ny][nx]];
                            }
                            break;
                        }
                        else if (getRGBKey(p) == baseRGB) {
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

                        if (p.a != 255) {
                            canExpandLeft = false;
                        }
                        else if (processed[ny][nx] && getRGBKey(p) != baseRGB) {
                            if (minInsertID >= idOrder[cov[ny][nx]]) {
                                canExpandLeft = false;
                            }
                            else if (idOrder[cov[ny][nx]] < insertKey) {
                                insertKey = idOrder[cov[ny][nx]];
                            }
                        }
                        else if (getRGBKey(p) == baseRGB) {
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

                        if (p.a != 255) {
                            canExpandUp = false;
                        }
                        else if (processed[ny][nx] && getRGBKey(p) != baseRGB) {
                            if (minInsertID >= idOrder[cov[ny][nx]]) {
                                canExpandUp = false;
                            }
                            else if (idOrder[cov[ny][nx]] < insertKey) {
                                insertKey = idOrder[cov[ny][nx]];
                            }
                        }
                        else if (getRGBKey(p) == baseRGB) {
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


            // Registration
            for (int dy = 0; dy < rectHeight; dy++) {
                for (int dx = 0; dx < rectWidth; dx++) {
                    cocos2d::ccColor4B p = tile[y + dy][x + dx];
                    if (p.a != 0 && getRGBKey(p) == baseRGB && (cov[y + dy][x + dx] < 0 || insertKey > idOrder[cov[y + dy][x + dx]])) {
                        processed[y + dy][x + dx] = true;
                    }
                    if (cov[y + dy][x + dx] < 0 || insertKey > idOrder[cov[y + dy][x + dx]]) {
                        cov[y + dy][x + dx] = idOrder.size();
                    }
                }
            }

            int colorID = palette[baseRGB];
            gdObjects.emplace_back(x, y, rectWidth, rectHeight, colorID, zLayer, startingZOrder, maxLinked, idOrder.size());
            for (int& id : idOrder) if (id >= insertKey) id++;
            idOrder.push_back(insertKey);
        }
    }
    std::sort(gdObjects.begin(), gdObjects.end(), [&](const GDObject& a, const GDObject& b) {
        return idOrder[a.ID] < idOrder[b.ID];
    });
    return gdObjects;
}

// Call this from your convertImage function
std::vector<GDObject> ImageConverter::imgToGDObjects(const std::vector<std::vector<cocos2d::ccColor4B>>& image, std::unordered_map<uint32_t, int>& palette, int zLayer, int startingZOrder, int tileWidth, int tileHeight) {
    int imgWidth = image[0].size();
    int imgHeight = image.size();
    if (tileWidth == 0) tileWidth = imgWidth;
    if (tileHeight == 0) tileHeight = imgHeight;

    std::vector<GDObject> totalObj;
    int tilesX = std::ceil((double)imgWidth / tileWidth);
    int tilesY = std::ceil((double)imgHeight / tileHeight);
    int maxLinked = 0;

    for (int ty = 0; ty < tilesY; ty++) {
        for (int tx = 0; tx < tilesX; tx++) {
            int x0 = tx * tileWidth;
            int y0 = ty * tileHeight;
            int tw = std::min(tileWidth, imgWidth - x0);
            int th = std::min(tileHeight, imgHeight - y0);
            maxLinked++;

            // Process the 4 orientations to see which is best
            auto tile = getFlippedTile(image, x0, y0, tw, th, false, false);
            auto objsN = tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette);

            tile = getFlippedTile(image, x0, y0, tw, th, false, true);
            auto objsY = tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette);

            tile = getFlippedTile(image, x0, y0, tw, th, true, false);
            auto objsX = tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette);

            tile = getFlippedTile(image, x0, y0, tw, th, true, true);
            auto objsXY = tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette);

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

            *best = removeRedundantObjects(*best, tw, th);
            shrinkObjects(*best, tw, th);
            assignZOrder(*best);

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

void ImageConverter::run(std::string path, float scale, int startColorID, int startingZOrder, int zLayer, int tileWidth, int tileHeight) {
    int width, height, channels;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        FLAlertLayer::create("Error", "Failed to load image", "OK")->show();
        return;
    }

    auto ui = EditorUI::get();

    std::unordered_map<uint32_t, int> palette;
    std::vector<std::vector<cocos2d::ccColor4B>> image(height, std::vector<cocos2d::ccColor4B>(width));

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
                    ui->m_editorLayer->m_levelSettings->m_effectManager->setColorAction(ColorAction::create({ pixelCol.r, pixelCol.g, pixelCol.b }, false, 0), startColorID + colorIndex);
                }
            }
        }
    }
    stbi_image_free(data);

    auto gdObjects = ImageConverter::imgToGDObjects(image, palette, zLayer, startingZOrder, tileWidth, tileHeight);
    bool zOrderPassedZero = false;

    std::unordered_map<int, cocos2d::CCArray*> groupedObjects;

    for (const auto& o : gdObjects) {
        float centerX = 180.0f + (o.x * scale + ((o.width * scale) / 2.0f)) * 7.5f;
        float centerY = 720.0f - (o.y * scale + ((o.height * scale) / 2.0f)) * 7.5f;

        if (o.zOrder == 0) zOrderPassedZero = true;

        auto gameObject = ui->m_editorLayer->createObject(917, CCPoint(centerX, centerY), false);
        gameObject->m_baseColor->m_colorID = startColorID + o.color;

        gameObject->m_zOrder = zOrderPassedZero ? o.zOrder + 1 : o.zOrder;
        gameObject->updateCustomScaleX(o.width * scale);
        gameObject->updateCustomScaleY(o.height * scale);
        gameObject->setCustomZLayer(o.zLayer);

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
