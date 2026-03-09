#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <ui/OptionsPopup.hpp>
#include <util/ImageConverter.hpp>

bool OptionsPopup::init(std::string const& value) {
    float popupW = 400.f;
    float popupH = 280.f;

    if (!Popup::init(popupW, popupH))
        return false;

    this->setAnchorPoint(ccp(0, 0));
    this->setContentSize(CCDirector::sharedDirector()->getWinSize());
    this->setKeypadEnabled(true);
    //this->setTitle("Import Pixel Art");

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
    imagePreview = LazySprite::create({ rightPanel->getContentWidth(), rightPanel->getContentHeight() }, false);
    imagePreview->setAutoResize(true);
    rightPanel->addChild(imagePreview);
    //rightPanel->addChildAtPosition(imagePreview, Anchor::Center);
    
    // import button
    auto spr2 = ButtonSprite::create("Import");
    auto btn2 = CCMenuItemSpriteExtra::create(spr2, this, menu_selector(OptionsPopup::onImport));
    rightPanel->addChild(btn2);

    // left panel title
    auto leftPanelLabel = CCLabelBMFont::create("Settings", "bigFont.fnt");
    leftPanel->addChild(leftPanelLabel);

    // scale input
    auto scaleInput = TextInput::create(leftPanel->getContentWidth() * 0.5f, "");
    scaleInput->setCommonFilter(CommonFilter::Float);
    scaleInput->setLabel("Scale");
    scaleInput->setScale(.7f);
    scaleInput->setCallback([this](auto const& val) {
        if (val.empty() || val == "-" || val == ".")
            return;
        OptionsPopup::scale = std::stof(val); 
    });
    scaleInput->setString("1.0", true);
    leftPanel->addChild(scaleInput);

    // starting color id input
    auto colorInput = TextInput::create(leftPanel->getContentWidth() * 0.5f, "");
    colorInput->setCommonFilter(CommonFilter::Int);
    colorInput->setLabel("Starting Color ID");
    colorInput->setScale(.7f);
    //colorInput->setCallback([this](auto const& val) {OptionsPopup::startColorID = std::stoi(val); });
    colorInput->setCallback([this](auto const& val) {
        if (val.empty() || val == "-" || val == ".")
            return;
        OptionsPopup::startColorID = std::stoi(val);
    });
    colorInput->setString("1", true);
    leftPanel->addChild(colorInput);

    // starting z-order input
    auto zOrderInput = TextInput::create(leftPanel->getContentWidth() * 0.5f, "");
    zOrderInput->setCommonFilter(CommonFilter::Int);
    zOrderInput->setLabel("Starting Z-Order");
    zOrderInput->setScale(.7f);
    //zOrderInput->setCallback([this](auto const& val) {OptionsPopup::startingZOrder = std::stoi(val); });
    zOrderInput->setCallback([this](auto const& val) {
        if (val.empty() || val == "-" || val == ".")
            return;
        OptionsPopup::startingZOrder = std::stoi(val);
    });
    zOrderInput->setString("-7", true);
    leftPanel->addChild(zOrderInput);

    // z-layer input
    auto zLayerInput = TextInput::create(leftPanel->getContentWidth() * 0.5f, "");
    zLayerInput->setCommonFilter(CommonFilter::Int);
    zLayerInput->setLabel("Z-Layer");
    zLayerInput->setScale(.7f);
    //zLayerInput->setCallback([this](auto const& val) {OptionsPopup::zLayer = std::stoi(val); });
    zLayerInput->setCallback([this](auto const& val) {
        if (val.empty() || val == "-" || val == ".")
            return;
        OptionsPopup::zLayer = std::stoi(val);
    });
    zLayerInput->setString("5", true);
    leftPanel->addChild(zLayerInput);

    //tile size panel
    auto tilePanel = CCMenu::create();
    tilePanel->setContentSize({ leftPanel->getContentWidth(), 30.f }); // match other inputs
    tilePanel->setLayout(RowLayout::create()->setGap(5));
    leftPanel->addChild(tilePanel);

    // tile width input
    auto tWidthInput = TextInput::create(tilePanel->getContentWidth() * 0.5f, "");
    tWidthInput->setCommonFilter(CommonFilter::Int);
    tWidthInput->setLabel("Tile Width");
    tWidthInput->setScale(.7f);
    //tWidthInput->setCallback([this](auto const& val) {OptionsPopup::tileWidth = std::stoi(val); });
    tWidthInput->setCallback([this](auto const& val) {
        if (val.empty() || val == "-" || val == ".")
            return;
        OptionsPopup::tileWidth = std::stoi(val);
    });
    tWidthInput->setString("0", true);
    tilePanel->addChild(tWidthInput);

    // tile height input
    auto tHeightInput = TextInput::create(tilePanel->getContentWidth() * 0.5f, "");
    tHeightInput->setCommonFilter(CommonFilter::Int);
    tHeightInput->setLabel("Tile Height");
    tHeightInput->setScale(.7f);
    //tHeightInput->setCallback([this](auto const& val) {OptionsPopup::tileHeight = std::stoi(val); });
    tHeightInput->setCallback([this](auto const& val) {
        if (val.empty() || val == "-" || val == ".")
            return;
        OptionsPopup::tileHeight = std::stoi(val);
    });
    tHeightInput->setString("0", true);
    tilePanel->addChild(tHeightInput);


    tilePanel->updateLayout();
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
    ImageConverter::run(OptionsPopup::imagePath.string(), OptionsPopup::scale, OptionsPopup::startColorID, OptionsPopup::startingZOrder, OptionsPopup::zLayer, OptionsPopup::tileWidth, OptionsPopup::tileHeight);
    this->onClose(nullptr);
}

void OptionsPopup::onBrowse(CCObject*) {
    using namespace geode::utils::file;


    FilePickOptions::Filter filter;
    filter.description = "Images (png, jpg, jpeg, gif)";
    filter.files.insert({ "*.png", "*.jpg", "*.jpeg", "*.gif" });

    async::spawn(pick(PickMode::OpenFile, { std::nullopt, {filter} }), [this](auto result) {
        if (result.isOk()) {
            auto opt = result.unwrap();
            if (opt) {
                auto path = opt.value();
                OptionsPopup::imagePath = path;
                imagePreview->setLoadCallback([sprite = imagePreview](geode::Result<void, std::string> result) {
                    if (result.isOk()) {
                        if (auto tex = sprite->getTexture())
                            tex->setAliasTexParameters(); // set sharp pixels
                    }
                    else geode::log::error("Failed to load image: {}", result.unwrapErr());
                });
                imagePreview->loadFromFile(path);

                //FLAlertLayer::create("Selected File", path.string(), "OK")->show();
            } 
        }
    });
}
