#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
using namespace geode::prelude;

#include <ui/OptionsPopup.hpp>

class $modify(MyMenuLayer, EditorUI) {
public:
    bool init(LevelEditorLayer * editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        auto menu = this->getChildByID("editor-buttons-menu");
        //auto myButton = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_pasteColorBtn_001.png"), this, menu_selector(MyMenuLayer::onModButtonClicked));
        auto buttonSprite = EditorButtonSprite::createWithSprite("ImportButton.png"_spr, 1.0f, EditorBaseColor::LightBlue);//CCSprite::create("ImportButton2.png"_spr);
        auto myButton = CCMenuItemSpriteExtra::create(buttonSprite, this, menu_selector(MyMenuLayer::onModButtonClicked));
        myButton->setContentSize(ccp(40, 40));
        myButton->setID("import-pixl-art"_spr);
        menu->addChild(myButton);
        menu->updateLayout();
        //buttonSprite->setContentSize(ccp(myButton->getContentWidth()*0.825f, myButton->getContentHeight() * 0.85f));


        return true;
    }

    void onModButtonClicked(CCObject*) {
        OptionsPopup::create("")->show();
    }
};