#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <util/ImageConverter.hpp>
#include <util/ObjectPlacer.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace geode::prelude;

uint32_t ImageConverter::getRGBKey(cocos2d::ccColor4B col) {
    return (uint32_t(col.r) << 16) | (uint32_t(col.g) << 8) | uint32_t(col.b);
}

std::vector<std::vector<cocos2d::ccColor4B>> ImageConverter::getFlippedTile(const std::vector<std::vector<cocos2d::ccColor4B>>& fullImg, int x0, int y0, int tw, int th, bool flipX, bool flipY, int rotation) {
    int outWidth = rotation != 0 ? th : tw;
    int outHeight = rotation != 0 ? tw : th;

    std::vector<std::vector<cocos2d::ccColor4B>>tile(outHeight, std::vector<cocos2d::ccColor4B>(outWidth));

    for (int y = 0; y < outHeight; y++) {
        for (int x = 0; x < outWidth; x++) {

            // Coordinates in the rotated image.
            int rx = flipX ? outWidth - 1 - x : x;
            int ry = flipY ? outHeight - 1 - y : y;

            int srcX;
            int srcY;

            switch (rotation) {
                case 0:
                    srcX = rx;
                    srcY = ry;
                    break;

                case 1: // 90° clockwise
                    srcX = ry;
                    srcY = th - 1 - rx;
                    break;

                case -1: // 90° counter-clockwise
                    srcX = tw - 1 - ry;
                    srcY = rx;
                    break;

                default:
                    srcX = rx;
                    srcY = ry;
                    break;
            }

            tile[y][x] = fullImg[y0 + srcY][x0 + srcX];
        }
    }

    return tile;
}

void ImageConverter::undoFlippedTile(TileVariant& tile, int x0, int y0, int tw, int th) {
    int rotatedWidth = tile.rotation != 0 ? th : tw;
    int rotatedHeight = tile.rotation != 0 ? tw : th;

    for (auto& o : tile.objects) {
        // Undo flips in the rotated coordinate system.
        if (tile.flipX) {
            o.x = rotatedWidth - o.x - o.width;
            o.flipX = true;
        }
        if (tile.flipY) {
            o.y = rotatedHeight - o.y - o.height;
            o.flipY = true;
        }

        // Undo rotation.
        if (tile.rotation == 1) { // from 90 deg to 0 deg
            int oldX = o.x;
            int oldY = o.y;

            o.x = oldY;
            o.y = th - oldX - o.width;
            o.rotation--;
        }
        else if (tile.rotation == -1) { // from -90 deg to 0 deg
            int oldX = o.x;
            int oldY = o.y;

            o.x = tw - oldY - o.height;
            o.y = oldX;
            o.rotation++;
        }

        o.x += x0;
        o.y += y0;
    }
}

std::vector<GDObject> ImageConverter::tileToGDObjects(const std::vector<std::vector<cocos2d::ccColor4B>>& tile, int zLayer, int startingZOrder, int maxLinked, const std::unordered_map<uint32_t, int>& palette) {
    int th = tile.size();
    int tw = tile[0].size();

    std::vector<GDObject> gdObjects; //resulting gd object list
    std::vector<std::vector<bool>> processed(th, std::vector<bool>(tw, false)); //list of pixels that matches the original image (these will never be considered again)
    std::vector<std::vector<int>> cov(th, std::vector<int>(tw, -1)); //list of what visible object covers what pixels (an object is represented by its index in the gdObjects list)

    //loop through each pixel in the tile
    for (int y = 0; y < th; y++) {
        for (int x = 0; x < tw; x++) {
            if (processed[y][x]) continue;

            cocos2d::ccColor4B baseCol = tile[y][x];
            if (baseCol.a != 255) {
                processed[y][x] = true;
                continue;
            }

            const uint32_t baseRGB = getRGBKey(baseCol); //converts the pixel's (and in turn, the object's) color into a unique int ID
            const int colorID = palette.at(baseRGB);
            constexpr size_t objectsTriedCount = 2;

            /*auto makeTask = [&](auto placeObject) {
                return async::runtime().spawnBlocking<ObjectComparator>(
                    [&, placeObject] {
                        return placeObject(tile, gdObjects, processed, cov, palette, y, x, th, tw, baseRGB, zLayer, startingZOrder, maxLinked, colorID);
                    }
                );
            };

            //try placing all object types in parallel
            std::array<arc::BlockingTaskHandle<ObjectComparator>, objectsTriedCount> tasks = {
                makeTask(ObjectPlacer::placePixel),
                makeTask(ObjectPlacer::placeStair)
            };

            // Wait for all tasks to finish
            std::array<ObjectComparator, objectsTriedCount> contenders = {
                tasks[0].blockOn(),
                tasks[1].blockOn()
            };*/
            

            std::array<ObjectComparator, objectsTriedCount> contenders = {
                ObjectPlacer::placePixel(tile, gdObjects, processed, cov, palette, y, x, th, tw, baseRGB, zLayer, startingZOrder, maxLinked, colorID),
                ObjectPlacer::placeStair(tile, gdObjects, processed, cov, palette, y, x, th, tw, baseRGB, zLayer, startingZOrder, maxLinked, colorID)
            };

            GDObject bestObjectTried = std::max_element(contenders.begin(), contenders.end(),
                [](const ObjectComparator& a, const ObjectComparator& b) {
                    return a.pixelsSaves < b.pixelsSaves;
                }
            )->object;

            //mark coverage and processed pixels
            bool currentPixelMarked = false;
            bestObjectTried.forEachPixel([&](int nx, int ny) {
                cocos2d::ccColor4B p = tile[ny][nx];
                const uint32_t pixelRGBKey = getRGBKey(p);
                //pixel is set to processed if the pixel has the same color as the object and the objects covering this pixel has a lower z-order
                if (pixelRGBKey == baseRGB && (cov[ny][nx] < 0 || bestObjectTried.ID > gdObjects[cov[ny][nx]].ID)) {
                    processed[ny][nx] = true;
                    if (ny == y && nx == x) {
                        currentPixelMarked = true;
                    }
                }

                //updates coverage for the pixel if the current object has the highest z-order for this pixel
                if (cov[ny][nx] < 0 || bestObjectTried.ID > gdObjects[cov[ny][nx]].ID) {
                    cov[ny][nx] = gdObjects.size(); //index of object that is at the top of this pixel
                }

                return true; // continue iterating
            });

            if (!currentPixelMarked) {
                log::debug("currentPixelMarked");
            //    x--;
            }


            for (GDObject& obj : gdObjects) if (obj.ID >= bestObjectTried.ID) obj.ID++;
            gdObjects.push_back(bestObjectTried);
        }
    }

    //sort the list of gd objects based on their insert index
    std::sort(gdObjects.begin(), gdObjects.end(), [&](const GDObject& a, const GDObject& b) {
        return a.ID < b.ID;
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

            auto makeTask = [&](bool flipX, bool flipY, int rotation = 0) {
                return async::runtime().spawnBlocking<TileVariant>(
                    [&image, &palette, x0, y0, tw, th, zLayer, startingZOrder, maxLinked, flipX, flipY, rotation] {
                        auto tile = getFlippedTile(image, x0, y0, tw, th, flipX, flipY, rotation);
                        std::vector<GDObject> objects = removeRedundantObjects(tileToGDObjects(tile, zLayer, startingZOrder, maxLinked, palette), rotation == 0 ? tw : th, rotation == 0 ? th : tw);
                        return TileVariant{std::move(objects), flipX, flipY, rotation};
                    }
                );
            };

            constexpr uint8_t taskCount = 10; //There are only 10 possible tile variants with rotation and flipping

            // Process variants of the tile in parallel to see which is best
            std::array<arc::BlockingTaskHandle<TileVariant>, taskCount> tasks = {
                makeTask(false, false), makeTask(false, true), makeTask(true, false), makeTask(true, true), //flipped vertical, horizontal and both
                makeTask(false, false, 1), makeTask(false, true, 1), makeTask(true, false, 1), //rotated right and flip variants
                makeTask(false, false, -1), makeTask(false, true, -1), makeTask(true, false, -1) //rotated left and flip variants
            };

            // Wait for all tasks to finish
            std::array<TileVariant, taskCount> contenders;
            for (uint8_t i = 0; i < taskCount; ++i) {
                contenders[i] = tasks[i].blockOn();
            }

            //chooses the tile variant with the least amount of objects
            auto bestVariant = std::min_element(contenders.begin(), contenders.end(),
                [](const TileVariant& a, const TileVariant& b) {
                    return a.objects.size() < b.objects.size();
                }
            );

            std::vector<GDObject>* tileObjects = &bestVariant->objects;
            //shrinkObjects(*tileObjects, bestVariant->rotation == 0 ? tw : th, bestVariant->rotation == 0 ? th : tw);
            assignZOrder2(*tileObjects);

            undoFlippedTile(*bestVariant, x0, y0, tw, th);
            totalObj.insert(totalObj.end(), tileObjects->begin(), tileObjects->end());
        }
    }
    return totalObj;
}

void ImageConverter::assignZOrder2(std::vector<GDObject>& objects) {
    struct Entry {
        int color = -1;
        int z = INT_MIN;
        int zLayerOffset = 0;
    };

    struct PixelState {
        Entry first;
        Entry second;

        // Track offset-2 colors independently of Z-order.
        int offset2Color1 = -1;
        int offset2Color2 = -1;
    };

    std::unordered_map<PixelPosition, PixelState, PixelPositionHash> pixels;

    // VERY Rough estimate.
    pixels.reserve(objects.size() * 4);

    auto updateEntry = [](Entry& entry, const GDObject& obj) {
        if (entry.color != obj.color) {
            return;
        }

        if (obj.zOrder > entry.z) {
            entry.z = obj.zOrder;
            entry.zLayerOffset = obj.ZLayerOffset;
        }
    };

    auto insertEntry = [&](PixelState& state, const GDObject& obj) {
        // Track offset-2 colors independently of Z-order.
        if (obj.ZLayerOffset == 2) {
            if (state.offset2Color1 == -1 ||
                state.offset2Color1 == obj.color) {
                state.offset2Color1 = obj.color;
            }
            else if (state.offset2Color2 == -1 ||
                state.offset2Color2 == obj.color) {
                state.offset2Color2 = obj.color;
            }
        }

        // Update an existing color's maximum Z-order.
        if (state.first.color == obj.color) {
            updateEntry(state.first, obj);
        }
        else if (state.second.color == obj.color) {
            updateEntry(state.second, obj);
        }
        else {
            Entry entry{ obj.color, obj.zOrder, obj.ZLayerOffset };

            if (entry.z > state.first.z) {
                state.second = state.first;
                state.first = entry;
            }
            else if (entry.z > state.second.z) {
                state.second = entry;
            }
        }

        // Keep entries sorted by Z-order.
        if (state.second.z > state.first.z) {
            std::swap(state.first, state.second);
        }
    };

    for (auto& o : objects) {
        int requiredZ = INT_MIN;
        bool overlapsOffset2DifferentColor = false;

        o.forEachPixel([&](int x, int y) {
            PixelPosition pixel{ x, y };

            auto it = pixels.find(pixel);
            if (it == pixels.end()) {
                return true;
            }

            const PixelState& state = it->second;

            // Preserve the existing Z-order calculation.
            const Entry& candidate = state.first.color != o.color ? state.first : state.second;

            if (candidate.color != -1 &&
                candidate.z > requiredZ) {
                requiredZ = candidate.z;
            }

            // Check independently for a differently colored object that requires ZLayerOffset == 2.
            if ((state.offset2Color1 != -1 && state.offset2Color1 != o.color) || (state.offset2Color2 != -1 && state.offset2Color2 != o.color)) {
                overlapsOffset2DifferentColor = true;
            }

            return true;
            });

        // Ensure this object renders above overlapping objects of different colors.
        if (requiredZ != std::numeric_limits<int>::min() && o.zOrder < requiredZ + 1) {
            o.zOrder = requiredZ + 1;
            if (o.zOrder == 0) {
                ++o.zOrder;
            }
        }

        // Propagate offset 2 independently of the highest-Z object.
        if (overlapsOffset2DifferentColor && o.ZLayerOffset == 0) {
            o.ZLayerOffset = 2;
        }

        // Record this object's final state at every covered pixel.
        o.forEachPixel([&](int x, int y) {
            insertEntry(pixels[PixelPosition{x, y}], o);
            return true;
        });
    }
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


        obj.forEachPixel([&](int x, int y) {
            if (cov[x][y] == 0 || objects[std::abs(cov[x][y]) - 1].color != obj.color) {
                cov[x][y] = i + 1;
            }
            else {
                cov[x][y] = -(int)(i + 1);
            }
            return true;
        });

        /*for (int y = obj.y; y < obj.y + obj.height; y++) {
            for (int x = obj.x; x < obj.x + obj.width; x++) {
                if (cov[x][y] == 0 || objects[std::abs(cov[x][y]) - 1].color != obj.color) {
                    cov[x][y] = i + 1;
                }
                else {
                    cov[x][y] = -(int)(i + 1);
                }

            }
        }*/
    }

    //remove objects
    //bool visible;
    for (size_t i = 0; i < objects.size(); i++) {
        const GDObject& obj = objects[i];

        obj.forEachPixel([&](int x, int y) {
            if (std::abs(cov[x][y]) - 1 == (int)i && cov[x][y] > 0) {
                nonRedundant.push_back(obj);
                return false; // return false is like breaking the loop
            }
            return true; // continue iterating
        });

        /*visible = false;
        for (int y = obj.y; y < obj.y + obj.height && !visible; y++) {
            for (int x = obj.x; x < obj.x + obj.width; x++) {

                if (std::abs(cov[x][y]) - 1 == (int)i && cov[x][y] > 0) {
                    nonRedundant.push_back(obj);
                    visible = true;
                    break;
                }

            }
        }*/
    }
    return nonRedundant;
}

void ImageConverter::shrinkObjects(std::vector<GDObject>& objects, int width, int height) {
    std::vector<std::vector<int>> cov(width, std::vector<int>(height));
    std::vector<std::vector<int>> color_below(width, std::vector<int>(height, -1));

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
                if (cov[x][y] == i && color_below[x][y] != obj.color) {
                    canShrink = false;
                    break;
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
                if (cov[x][y] == i && color_below[x][y] != obj.color) {
                    canShrink = false;
                    break;
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
                if (cov[x][y] == i && color_below[x][y] != obj.color) {
                    canShrink = false;
                    break;
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
                if (cov[x][y] == i && color_below[x][y] != obj.color) {
                    canShrink = false;
                    break;
                }
            }
            if (canShrink) {
                obj.width--;
            }
        }

        for (size_t y = obj.y; y < obj.y + obj.height; y++) {
            for (size_t x = obj.x; x < obj.x + obj.width; x++) {
                color_below[x][y] = obj.color;
            }
        }
    }
}

cocos2d::ccHSVValue ImageConverter::RGBtoHSV(const cocos2d::ccColor4B& color) {
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
        FLAlertLayer::create("Error", "Failed to load image. The image format used might not be supported.", "OK")->show();
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
                        auto colTrigger = static_cast<EffectGameObject*>(ui->m_editorLayer->createObject(899, ccp(-15, 90 + colorIndex * 15), false));
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

    auto start = std::chrono::steady_clock::now();

    //convert image to gd objects
    const std::vector<GDObject> gdObjects = ImageConverter::imgToGDObjects(image, palette, zLayer, startingZOrder, tileWidth, tileHeight);

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(end - start).count();
    log::debug("Import delay: {:.3f} ms", elapsed);

    bool zOrderPassedZero = false;

    //every object with the same linkID is put in the same list, each id has its own list of objects belonging to that group
    std::unordered_map<int, cocos2d::CCArray*> groupedObjects;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    CCPoint center = ui->m_editorLayer->m_objectLayer->convertToNodeSpace(ccp(winSize.width / 2.f, winSize.height / 2.f));
    center.x = (int)((center.x - width * 3.75f * scale) / 30) * 30.f;
    center.y = (int)((center.y + height * 3.75f * scale) / 30) * 30.f;

    //creates and places the gd objects in the level
    //for (const auto& o : gdObjects) { //1 block is 30 units. 1 pixel is 1/4 of a block, so its 7.5 units.
    for (int h = gdObjects.size() - 1; h >= 0; h--) {
        const GDObject& o = gdObjects[h];

        /*float centerX = center.x + ((o.x + o.width / 2.f) * 7.5f * scale);
        float centerY = center.y - ((o.y + o.height / 2.f) * 7.5f * scale);
        if (o.rotation % 2 != 0) {
            centerX = center.x + ((o.x + o.height / 2.f) * 7.5f * scale);
            centerY = center.y - ((o.y + o.width / 2.f) * 7.5f * scale);
        }*/

        float placePosX = center.x + o.getRealXPosition(scale);
        float placePosY = center.y - o.getRealYPosition(scale);

        /*if (o.objectType == ObjectType::STAIR)
            log::debug(
                "Stair x={}, y={}, rot={}, flipX={}, flipY={}, "
                "scaleX={}, scaleY={}, offset={}, realX={}, realY={}",
                o.x, o.y, o.rotation, o.flipX, o.flipY,
                o.scaleX, o.scaleY, o.positionOffset,
                placePosX,
                placePosY
            );*/

        if (o.zOrder == 0) zOrderPassedZero = true;

        auto gameObject = ui->m_editorLayer->createObject(o.objectType, CCPoint(placePosX, placePosY), false);
        if (hsvMode) {
            auto& hsvb = gameObject->m_baseColor->m_hsv;
            hsvb = hsv[startColorID + o.color - 1];
            hsvb.absoluteSaturation = true;
            hsvb.absoluteBrightness = true;
            gameObject->m_baseColor->m_colorID = 1010;

            if (o.hasDetailColor) {
                auto& hsvd = gameObject->m_detailColor->m_hsv;
                hsvd = hsv[startColorID + o.color - 1];
                hsvd.absoluteSaturation = true;
                hsvd.absoluteBrightness = true;
                gameObject->m_detailColor->m_colorID = 1010;
            }
        }
        else {
            gameObject->m_baseColor->m_colorID = startColorID + o.color - 1;
            if (o.hasDetailColor)
                gameObject->m_detailColor->m_colorID = startColorID + o.color - 1;
        }

        gameObject->m_zOrder = zOrderPassedZero ? o.zOrder + 1 : o.zOrder;
        //m_pixelScaleX/Y is a hardcoded base multiplier for some pixel objects
        gameObject->updateCustomScaleX(gameObject->m_pixelScaleX * o.scaleX * o.baseScale * scale + 2.f / 300.f); //+2 pixel width to remove graphical glitch lines
        gameObject->updateCustomScaleY(gameObject->m_pixelScaleY * o.scaleY * o.baseScale * scale + 2.f / 300.f); //+2 pixel height to remove graphical glitch lines
        gameObject->setRotation(o.rotation * 90);
        gameObject->setFlipX(o.flipX);
        gameObject->setFlipY(o.flipY);
        gameObject->setCustomZLayer(5 + o.ZLayerOffset);
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
