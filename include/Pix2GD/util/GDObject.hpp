#pragma once

#include <util/ObjectType.hpp>
#include <util/PixelPosition.hpp>

struct GDObject {
    int x, y, scaleX, scaleY;
    int color, zLayer, zOrder;
    int linkID, ID;
    ObjectType objectType;
    int rotation;
    bool flipX, flipY;

    bool hasDetailColor;

    int ZLayerOffset; //objects on different sprite sheets need to be on a different z-layer to be ordered correctly.
    float positionOffset; //the offset im both x and y between the objects position and visual center
    int width, height; //the actual pixel width and height of the texture
    float baseScale; //the scale offset needed to get a pixel size of 1/16 of a block

    GDObject(int x, int y, int sX, int sY, int c, int zl, int zo, int lid, int id, ObjectType objType = ObjectType::PIXEL, int rot = 0, bool fX = false, bool fY = false)
        : x(x), y(y), scaleX(sX), scaleY(sY), color(c), zLayer(zl), zOrder(zo), linkID(lid), ID(id), objectType(objType), rotation(rot), flipX(fX), flipY(fY) {

        if (objectType == ObjectType::PIXEL) {
            hasDetailColor = false;
            ZLayerOffset = 0;
            positionOffset = 0.f;
            width = scaleX;
            height = scaleY;

            baseScale = 1;
        }

        if (objectType == ObjectType::STAIR) {
            hasDetailColor = true;
            ZLayerOffset = 2;
            positionOffset = 6.5f;

            width = 3 * scaleX;
            height = 3 * scaleY;

            baseScale = 4;
        }
    }

    template <typename Func>
    void forEachPixel(Func&& func) const {
        if (objectType == ObjectType::PIXEL) {
            for (int dy = 0; dy < height; ++dy) {
                for (int dx = 0; dx < width; ++dx) {
                    if (!func(x + dx, y + dy))
                        return;
                }
            }
        }
        else if (objectType == ObjectType::STAIR) {
            for (int dy = 0; dy < 3; ++dy) {
                for (int dx = 0; dx < 3 - dy; ++dx) {
                    if (!func(x + dx, y + dy))
                        return;
                }
            }
        }
    }

    const float getRealXPosition(float scaleSetting) const {
        float xOffset = positionOffset * scaleX;

        if (rotation == 1) {    
            xOffset = -xOffset;
        }
        if (rotation == 0 && flipX) {
            xOffset = -xOffset;
        }
        if (rotation != 0 && flipY) {
            xOffset = -xOffset;
        }

        float actualHalfWidth = (rotation % 2 == 0 ? width : height) / 2.f;
        return (x + actualHalfWidth + xOffset) * 7.5f * scaleSetting;
    }

    const float getRealYPosition(float scaleSetting) const {
        float yOffset = positionOffset * scaleY;

        if (rotation == -1) {
            yOffset = -yOffset;
        }
        if (rotation == 0 && flipY) {
            yOffset = -yOffset;
        }
        if (rotation != 0 && flipX) {
            yOffset = -yOffset;
        }

        float actualHalfHeight = (rotation % 2 == 0 ? height : width) / 2.f;
        return (y + actualHalfHeight + yOffset) * 7.5f * scaleSetting;
    }
};
