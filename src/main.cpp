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
        log::info("[No End Screen]: Level complete");

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        ccColor3B tintColor = Mod::get()->getSettingValue<ccColor3B>("TintColor");
        float opacityValue = Mod::get()->getSettingValue<float>("TintOpacity");

        // === Create and add tint sprite ===
        auto tintSprite = CCSprite::create();
        tintSprite->setTextureRect({0, 0, winSize.width, winSize.height});
        tintSprite->setColor(tintColor);
        tintSprite->setOpacity(static_cast<GLubyte>(opacityValue * 255));
        tintSprite->setAnchorPoint({0.f, 0.f});
        tintSprite->setPosition({0.f, 0.f});
        tintSprite->setZOrder(9998);
        this->addChild(tintSprite);

        // === Handle auto return ===
        if (Mod::get()->getSettingValue<bool>("AutoReturn")) {
            float delay = Mod::get()->getSettingValue<float>("AutoReturnDelay");
            log::info("[No End Screen]: AutoReturn is enabled — returning in {}s", delay);

            this->runAction(CCSequence::createWithTwoActions(
                CCDelayTime::create(delay),
                CCCallFunc::create(this, callfunc_selector(MyPlayLayer::returnToMenu))
            ));
            return;
        }

        log::info("[No End Screen]: Showing invisible exit button");

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

        this->addChild(menu, 9999);
    }

    void onInvisibleButtonPressed(CCObject*) {
        log::info("[No End Screen]: Invisible button clicked — exiting level");
        returnToMenu();
    }

    void returnToMenu() {
        if (!Mod::get()->getSettingValue<bool>("ContinueLevelMusic")) {
            log::info("[No End Screen]: Restarting menu music");
    
            FMODAudioEngine::sharedEngine()->clearAllAudio();
            GameManager::sharedState()->playMenuMusic();
        }
    
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
