#pragma once

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class OptionsPopup : public Popup {
protected:
    float scale;
    int startColorID;
    int startingZOrder;
    int zLayer;
    int tileWidth;
    int tileHeight;
    bool createColTriggers = false;
    bool hsvMode = false;

    //CCMenu* rightPanel = nullptr;
    //CCLayerColor* colorLayer = nullptr;
    NineSlice* colorLayer = nullptr;
    LazySprite* imagePreview = nullptr;
    std::filesystem::path imagePath;

    bool init(std::string const& value);

public:
    static OptionsPopup* create(std::string const& text);
    void onBrowse(CCObject* sender);
    void onImport(CCObject* sender);
    void renderImage(const std::filesystem::path& path);
};
