#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <cstddef>

class TileView {
public:
    using Image = std::vector<std::vector<cocos2d::ccColor4B>>;
private:
    const Image& tile;
    int x0;
    int y0;
    int tileWidth;
    int tileHeight;

    struct Coordinate {
        int x;
        int y;
    };

    std::vector<Coordinate> lookup;

public:
    TileView(const Image& image, int x0, int y0, int tw, int th, bool flipX = false, bool flipY = false, int rotation = 0)
        : tile(image), x0(x0), y0(y0), tileWidth(rotation != 0 ? th : tw), tileHeight(rotation != 0 ? tw : th) {

        lookup.resize(static_cast<size_t>(tileWidth) * tileHeight);

        for (int y = 0; y < tileHeight; ++y) {
            for (int x = 0; x < tileWidth; ++x) {

                // Coordinates in the rotated image.
                int rx = flipX ? tileWidth - 1 - x : x;
                int ry = flipY ? tileHeight - 1 - y : y;

                int srcX;
                int srcY;

                switch (rotation) {
                case 1: // 90 degrees clockwise
                    srcX = ry;
                    srcY = th - 1 - rx;
                    break;

                case -1: // 90 degrees counter-clockwise
                    srcX = tw - 1 - ry;
                    srcY = rx;
                    break;

                default:
                    srcX = rx;
                    srcY = ry;
                    break;
                }

                lookup[static_cast<size_t>(y) * tileWidth + x] = Coordinate(srcX, srcY);
            }
        }
    }

    int width() const {
        return tileWidth;
    }

    int height() const {
        return tileHeight;
    }

    cocos2d::ccColor4B at(int x, int y) const {
        const Coordinate& coord = lookup[static_cast<size_t>(y) * tileWidth + x];
        return tile[y0 + coord.y][x0 + coord.x];
    }
};