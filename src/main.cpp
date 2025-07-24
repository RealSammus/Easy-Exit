#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>

using namespace geode::prelude;
using namespace cocos2d;

CCMenuItemSpriteExtra* g_invisibleExitButton = nullptr;

class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
        PlayLayer::levelComplete();

        log::info("[No End Screen]: Level complete → showing invisible exit button");

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // === Get tint color and opacity from settings ===
        ccColor3B tintColor = Mod::get()->getSettingValue<ccColor3B>("TintColor");
        float opacityValue = Mod::get()->getSettingValue<float>("TintOpacity");

        // === Create a tint-colored sprite ===
        auto tintSprite = CCSprite::create();
        tintSprite->setTextureRect({0, 0, winSize.width, winSize.height});
        tintSprite->setColor(tintColor);
        tintSprite->setOpacity(static_cast<GLubyte>(opacityValue * 255));
        tintSprite->setAnchorPoint({0.5f, 0.5f});

        // === Create the invisible button ===
        g_invisibleExitButton = CCMenuItemSpriteExtra::create(
            tintSprite,
            this,
            menu_selector(MyPlayLayer::onInvisibleButtonPressed)
        );
        g_invisibleExitButton->setPosition(winSize.width / 2, winSize.height / 2);

        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        menu->addChild(g_invisibleExitButton);

        this->addChild(menu, 9999); // On top of everything
    }

    void onInvisibleButtonPressed(CCObject*) {
        log::info("[No End Screen]: Invisible button clicked — exiting level");

        if (Mod::get()->getSettingValue<bool>("ListReturn")) {
            CCDirector::sharedDirector()->popScene(); // Back to list
        } else {
            auto scene = GJGarageLayer::scene(); // Fallback
            if (auto pl = GameManager::sharedState()->getPlayLayer()) {
                if (auto level = pl->m_level) {
                    scene = LevelInfoLayer::scene(level, false);
                }
            }
            CCDirector::sharedDirector()->replaceScene(scene);
        }
    }
};