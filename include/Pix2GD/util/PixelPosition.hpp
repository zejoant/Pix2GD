#pragma once

struct PixelPosition {
    const unsigned int x, y;

    PixelPosition(int x = 0, int y = 0) : x(x), y(y) {}

    bool operator==(const PixelPosition& other) const {
        return x == other.x && y == other.y;
    }
};

struct PixelPositionHash {
    std::size_t operator()(const PixelPosition& p) const {
        const std::size_t h1 = std::hash<int>{}(p.x);
        const std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};