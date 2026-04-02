#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
using namespace geode::prelude;

#include <ui/OptionsPopup.hpp>

class $modify(MyMenuLayer, EditorUI) {
    bool init(LevelEditorLayer * editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        auto menu = this->getChildByID("editor-buttons-menu");
        auto buttonSprite = EditorButtonSprite::createWithSprite("ImportButton.png"_spr, 1.0f, EditorBaseColor::LightBlue);
        auto myButton = CCMenuItemSpriteExtra::create(buttonSprite, this, menu_selector(MyMenuLayer::onModButtonClicked));
        myButton->setContentSize(ccp(40, 40));
        myButton->setID("import-pixl-art"_spr);
        menu->addChild(myButton);
        menu->updateLayout();

        this->m_uiItems->addObject(myButton);

        return true;
    }

    void onModButtonClicked(CCObject*) {
        OptionsPopup::create("")->show();
    }
};