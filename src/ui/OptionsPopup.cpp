#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <ui/OptionsPopup.hpp>
#include <util/ImageConverter.hpp>

#include <Geode/utils/general.hpp>

bool OptionsPopup::init(std::string const& value) {
    float popupW = 400.f;
    float popupH = 280.f;

    if (!Popup::init(popupW, popupH))
        return false;

    this->setAnchorPoint(ccp(0, 0));
    this->setContentSize(CCDirector::sharedDirector()->getWinSize());
    this->setKeypadEnabled(true);

    auto mainPanel = CCMenu::create();
    mainPanel->setAnchorPoint(ccp(0.5f, 0.5f));
    mainPanel->setContentSize({ popupW * 0.90f, popupH * 0.90f });
    m_mainLayer->addChildAtPosition(mainPanel, Anchor::Center, ccp(0.0f, 0.0f));

    auto leftPanel = CCMenu::create();
    leftPanel->setAnchorPoint(ccp(0.0f, 0.0f));
    leftPanel->setContentSize({ mainPanel->getContentWidth() / 2.0f, mainPanel->getContentHeight() });
    leftPanel->setLayout(ColumnLayout::create()->setAxisReverse(true)->setGap(15));
    mainPanel->addChildAtPosition(leftPanel, Anchor::BottomLeft, ccp(0.0f, 0.0f));

    auto rightPanel = CCMenu::create();
    rightPanel->setAnchorPoint(ccp(0.0f, 0.0f));
    rightPanel->setContentSize({ mainPanel->getContentWidth() / 2.0f, mainPanel->getContentHeight() });
    rightPanel->setLayout(ColumnLayout::create()->setAxisReverse(true)->setGap(15));
    mainPanel->addChildAtPosition(rightPanel, Anchor::Bottom, ccp(0.0f, 0.0f));

    // browse button
    auto spr1 = ButtonSprite::create("Browse Image");
    auto btn1 = CCMenuItemSpriteExtra::create(spr1, this, menu_selector(OptionsPopup::onBrowse));
    rightPanel->addChild(btn1);

    // preview sprite
    colorLayer = NineSlice::create("square02b_001.png");
    colorLayer->setColor({108, 60, 36});
    colorLayer->setContentSize({ rightPanel->getContentWidth(), rightPanel->getContentHeight() * 0.7f });
    rightPanel->addChild(colorLayer);

    renderImage(Mod::get()->getSavedValue<std::filesystem::path>("path", ""));

    // import button
    auto spr2 = ButtonSprite::create("Import");
    auto btn2 = CCMenuItemSpriteExtra::create(spr2, this, menu_selector(OptionsPopup::onImport));
    rightPanel->addChild(btn2);

    // info button
    auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoSpr->setScale(0.8f);
    auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(OptionsPopup::onInfo));
    mainPanel->addChildAtPosition(infoBtn, Anchor::TopRight, ccp(popupW * 0.05f, popupH * 0.05f));


    // left panel title
    auto leftPanelLabel = CCLabelBMFont::create("Settings", "bigFont.fnt");
    leftPanel->addChild(leftPanelLabel);

    // scale/color panel
    auto scolPanel = CCMenu::create();
    scolPanel->setContentSize({ leftPanel->getContentWidth(), 30.f }); // match other inputs
    scolPanel->setLayout(RowLayout::create()->setGap(5));
    leftPanel->addChild(scolPanel);

    // scale input
    auto scaleInput = TextInput::create(scolPanel->getContentWidth() * 0.5f, "");
    scaleInput->setCommonFilter(CommonFilter::Float);
    scaleInput->setLabel("Scale");
    scaleInput->setScale(.7f);
    scaleInput->setCallback([this, scaleInput](auto const& val) {
        if (val.empty() || val == "-" || val == ".") return;
        //float v = std::stof(val);
        float v = numFromString<float>(val).unwrapOr(0.25);
        auto str = val;
        if (v < 0.f) {
            v = 0.f;
            str = fmt::format("{}", v);
            scaleInput->setString(str, false);
        }
        OptionsPopup::scale = v;
        Mod::get()->setSavedValue<std::string>("scale", str);
    });
    scaleInput->setString(Mod::get()->getSavedValue<std::string>("scale", "0.25"), true);
    scolPanel->addChild(scaleInput);

    // starting color id input
    auto colorInput = TextInput::create(scolPanel->getContentWidth() * 0.5f, "");
    colorInput->setCommonFilter(CommonFilter::Int);
    colorInput->setLabel("Starting Color ID");
    colorInput->setScale(.7f);
    colorInput->setCallback([this, colorInput](auto const& val) {
        if (val.empty() || val == "-" || val == ".") return;
        //float v = std::stoi(val);
        int v = numFromString<int>(val).unwrapOr(1);
        auto str = val;
        if (v < 1) {
            v = 1;
            str = fmt::format("{}", v);
            colorInput->setString(str, false);
        }
        OptionsPopup::startColorID = v;
        Mod::get()->setSavedValue<std::string>("color-id", str);
    });
    colorInput->setString(Mod::get()->getSavedValue<std::string>("color-id", "1"), true);
    scolPanel->addChild(colorInput);

    // layer/order panel
    auto layerPanel = CCMenu::create();
    layerPanel->setContentSize({ leftPanel->getContentWidth(), 30.f });
    layerPanel->setLayout(RowLayout::create()->setGap(5));
    leftPanel->addChild(layerPanel);

    // starting z-order input
    auto zOrderInput = TextInput::create(layerPanel->getContentWidth() * 0.5f, "");
    zOrderInput->setCommonFilter(CommonFilter::Int);
    zOrderInput->setLabel("Starting Z-Order");
    zOrderInput->setScale(.7f);
    zOrderInput->setCallback([this, zOrderInput](auto const& val) {
        if (val.empty() || val == "-" || val == ".") return;
        //float v = std::stoi(val);
        int v = numFromString<int>(val).unwrapOr(-7);
        auto str = val;
        if (v < -100) {
            v = -100;
            str = fmt::format("{}", v);
            zOrderInput->setString(str, false);
        }
        else if (v > 50) {
            v = 50;
            str = fmt::format("{}", v);
            zOrderInput->setString(str, false);
        }
        OptionsPopup::startingZOrder = v;
        Mod::get()->setSavedValue<std::string>("z-order", str);
    });
    zOrderInput->setString(Mod::get()->getSavedValue<std::string>("z-order", "-7"), true);
    layerPanel->addChild(zOrderInput);

    // z-layer input
    auto zLayerInput = TextInput::create(layerPanel->getContentWidth() * 0.5f, "");
    zLayerInput->setCommonFilter(CommonFilter::Int);
    zLayerInput->setLabel("Editor Layer");
    zLayerInput->setScale(.7f);
    zLayerInput->setCallback([this, zLayerInput](auto const& val) {
        if (val.empty() || val == "-" || val == ".") return;
        //float v = std::stoi(val);
        int v = numFromString<int>(val).unwrapOr(0);
        auto str = val;
        if (v < 0) {
            v = 0;
            str = fmt::format("{}", v);
            zLayerInput->setString(str, false);
        }
        OptionsPopup::zLayer = v;
        Mod::get()->setSavedValue<std::string>("editor-layer", str);
    });
    zLayerInput->setString(Mod::get()->getSavedValue<std::string>("editor-layer", "0"), true);
    layerPanel->addChild(zLayerInput);

    //tile size panel
    auto tilePanel = CCMenu::create();
    tilePanel->setContentSize({ leftPanel->getContentWidth(), 30.f });
    tilePanel->setLayout(RowLayout::create()->setGap(5));
    leftPanel->addChild(tilePanel);

    // tile width input
    auto tWidthInput = TextInput::create(tilePanel->getContentWidth() * 0.5f, "");
    tWidthInput->setCommonFilter(CommonFilter::Int);
    tWidthInput->setLabel("Tile Width");
    tWidthInput->setScale(.7f);
    tWidthInput->setCallback([this, tWidthInput](auto const& val) {
        if (val.empty() || val == "-" || val == ".") return;
        //float v = std::stoi(val);
        int v = numFromString<int>(val).unwrapOr(0);
        auto str = val;
        if (v < 0) {
            v = 0;
            str = fmt::format("{}", v);
            tWidthInput->setString(str, false);
        }
        OptionsPopup::tileWidth = v;
        Mod::get()->setSavedValue<std::string>("tile-width", str);
    });
    tWidthInput->setString(Mod::get()->getSavedValue<std::string>("tile-width", "0"), true);
    tilePanel->addChild(tWidthInput);

    // tile height input
    auto tHeightInput = TextInput::create(tilePanel->getContentWidth() * 0.5f, "");
    tHeightInput->setCommonFilter(CommonFilter::Int);
    tHeightInput->setLabel("Tile Height");
    tHeightInput->setScale(.7f);
    tHeightInput->setCallback([this, tHeightInput](auto const& val) {
        if (val.empty() || val == "-" || val == ".") return;
        //float v = std::stoi(val);
        int v = numFromString<int>(val).unwrapOr(0);
        auto str = val;
        if (v < 0) {
            v = 0;
            str = fmt::format("{}", v);
            tHeightInput->setString(str, false);
        }
        OptionsPopup::tileHeight = v;
        Mod::get()->setSavedValue<std::string>("tile-height", str);
    });
    tHeightInput->setString(Mod::get()->getSavedValue<std::string>("tile-height", "0"), true);
    tilePanel->addChild(tHeightInput);




    // hsv panel
    auto hsvPanel = CCMenu::create();
    hsvPanel->setContentSize({ leftPanel->getContentWidth(), 30.f });
    hsvPanel->setLayout(RowLayout::create()->setGap(5)->setAxisAlignment(AxisAlignment::Start));
    leftPanel->addChild(hsvPanel);

    // color trig panel
    auto colTrigPanel = CCMenu::create();
    colTrigPanel->setContentSize({ leftPanel->getContentWidth(), 30.f });
    colTrigPanel->setLayout(RowLayout::create()->setGap(5)->setAxisAlignment(AxisAlignment::Start));
    leftPanel->addChild(colTrigPanel);




    auto colTrigToggler = CCMenuItemExt::createTogglerWithStandardSprites(
        1.f,
        [this](CCMenuItemToggler* btn) {
            OptionsPopup::createColTriggers = !btn->isToggled();
            Mod::get()->setSavedValue<bool>("create-col-trigs", OptionsPopup::createColTriggers);
        }
    );
    colTrigPanel->addChild(colTrigToggler);
    OptionsPopup::createColTriggers = Mod::get()->getSavedValue<bool>("create-col-trigs", true);
    colTrigToggler->toggle(OptionsPopup::createColTriggers);
    colTrigToggler->updateSprite();

    // label
    auto colTrigLabel = CCLabelBMFont::create("Create Color Triggers", "goldFont.fnt");
    colTrigPanel->addChild(colTrigLabel);
    colTrigPanel->updateLayout();
    colTrigLabel->setScale(0.4f);



    auto hsvToggler = CCMenuItemExt::createTogglerWithStandardSprites(
        1.f,
        [this, colTrigLabel, colTrigToggler, colorInput, scolPanel, leftPanel, scaleInput](CCMenuItemToggler* btn) {
            bool active = btn->isToggled();
            OptionsPopup::hsvMode = !active;
            Mod::get()->setSavedValue<bool>("hsv-mode", OptionsPopup::hsvMode);
            //FLAlertLayer::create("Error", "Balls", "OK")->show();

            colTrigLabel->setVisible(!OptionsPopup::hsvMode);
            colTrigToggler->setVisible(!OptionsPopup::hsvMode);
            colorInput->setVisible(active);
            scaleInput->setWidth(!active ? scolPanel->getContentWidth() : scolPanel->getContentWidth() * 0.5f);
            scolPanel->updateLayout();
            leftPanel->updateLayout();
        }
    );
    hsvPanel->addChild(hsvToggler);
    OptionsPopup::hsvMode = Mod::get()->getSavedValue<bool>("hsv-mode", true);
    hsvToggler->toggle(OptionsPopup::hsvMode);
    hsvToggler->updateSprite();
    colTrigLabel->setVisible(!OptionsPopup::hsvMode);
    colTrigToggler->setVisible(!OptionsPopup::hsvMode);
    colorInput->setVisible(!OptionsPopup::hsvMode);
    scaleInput->setWidth(OptionsPopup::hsvMode ? scolPanel->getContentWidth() : scolPanel->getContentWidth() * 0.5f);

    // label
    auto hsvLabel = CCLabelBMFont::create("HSV Mode               ", "goldFont.fnt");
    hsvPanel->addChild(hsvLabel);
    hsvPanel->updateLayout();
    hsvLabel->setScale(0.4f);





    tilePanel->updateLayout();
    scolPanel->updateLayout();
    layerPanel->updateLayout();
    leftPanel->updateLayout();
    rightPanel->updateLayout();

    return true;
}

OptionsPopup* OptionsPopup::create(std::string const& text) {
    auto ret = new OptionsPopup();
    if (ret->init(text)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

void OptionsPopup::onImport(CCObject*) {
    ImageConverter::run(OptionsPopup::imagePath.string(), OptionsPopup::scale, OptionsPopup::startColorID, OptionsPopup::startingZOrder, OptionsPopup::zLayer, OptionsPopup::tileWidth, OptionsPopup::tileHeight, OptionsPopup::createColTriggers, OptionsPopup::hsvMode);
    this->onClose(nullptr);
}

void OptionsPopup::onInfo(CCObject*) {
    MDPopup::create(
        "Pix2GD Help",

        "<cb>Scale:</c> Scales the imported image. With a scale of 1, a 4x4 area of pixels will fit in a block.\n\n"
        "<cy>Starting Color ID:</c> The color channels used will start at this ID and increase as needed.\n\n"
        "<cp>Starting Z-Order:</c> The objects will start at this z-order and increase as needed.\n\n"
        "<cl>Editor Layer:</c> The editor layer where objects are placed.\n\n"
        "<cb>Create Color Triggers:</c> Will set the colors through color triggers instead of setting the color channels directly.\n\n"
        "<cy>HSV Mode:</c> Uses HSV instead of color channels. The only reason to disable this option is if you plan to modify the color palette after importing.\n\n"

        "<cg>Tile Width / Height:</c> The image is processed in tiles of the specified size. All tiles are completely separate from eachother, meaning objects of one tile will never overlap another tile. Each tile's objects will be linked together.\n\n"
        "<cr>Keep both at 0 if you are not importing a tileset or spritesheet,</c> as this will treat the whole image as a single tile resulting in the least amount of objects. However, if importing a large detailed image is freezing the game for a long time, consider setting a tile size to speed up the process\n\n"
        "If you are importing a tileset or spritesheet, make sure the set the tile width and height even if your tileset has separated tiles, as it will both take less time to import and result in less objects.\n\n"
        "<cb>File Formats:</c> Pix2GD supports .png, .jpg, .jpeg .gif .tga and .bmp files, though the last three will not display an image preview. Since Pix2GD is primarily designed for pixel art, .png is the recommended format due to its lossless image quality.\n\n",
        "OK"
    )->show();
}

void OptionsPopup::onBrowse(CCObject*) {
    using namespace geode::utils::file;


    FilePickOptions::Filter filter;
    filter.description = "Images";
    filter.files.insert({ "*.png", "*.jpg", "*.jpeg", "*.gif", "*.tga", "*.bmp"});

    async::spawn(pick(PickMode::OpenFile, { std::nullopt, {filter} }), [this](auto result) {
        if (result.isOk()) {
            auto opt = result.unwrap();
            if (opt) {
                auto path = opt.value();
                Mod::get()->setSavedValue<std::filesystem::path>("path", path);
                
                renderImage(path);
            } 
        }
    });
}

void OptionsPopup::renderImage(const std::filesystem::path& path) {
    OptionsPopup::imagePath = path;
    if (this->colorLayer->getChildByID("image-preview")) {
        this->colorLayer->removeChild(this->imagePreview, true);
    }
    this->imagePreview = LazySprite::create({ this->colorLayer->getContentWidth() * 0.9f, this->colorLayer->getContentHeight() * 0.9f }, false);
    this->imagePreview->setAutoResize(true);
    this->imagePreview->setID("image-preview");
    this->colorLayer->addChildAtPosition(this->imagePreview, Anchor::Center, ccp(0.0f, 0.0f));

    this->imagePreview->setLoadCallback([sprite = this->imagePreview](geode::Result<void, std::string> result) {
        if (result.isOk()) {
            if (auto tex = sprite->getTexture())
                tex->setAliasTexParameters(); // set sharp pixels
        }
        else geode::log::error("Failed to load image: {}", result.unwrapErr());
        });
    this->imagePreview->loadFromFile(path);
}
