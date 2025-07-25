#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <cocos2d.h>

using namespace geode::prelude;
using namespace cocos2d;

CCMenuItemSpriteExtra* g_invisibleExitButton = nullptr;
bool g_inPracticeMode = false;

class $modify(MyPauseLayer, PauseLayer) {
    void onPracticeMode(CCObject* sender) {
        PauseLayer::onPracticeMode(sender);
        bool newState = GameManager::sharedState()->getGameVariable("0049"); // Practice mode toggle
        g_inPracticeMode = newState;
        log::info("[No End Screen]: Practice Mode {}", g_inPracticeMode ? "ENABLED" : "DISABLED");
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
        if (Mod::get()->getSettingValue<bool>("PracticeReturn") && g_inPracticeMode) {
            log::info("[No End Screen]: Skipping mod effects due to Practice Mode");
            PlayLayer::levelComplete();
            return;
        }

        PlayLayer::levelComplete();
        log::info("[No End Screen]: Level complete");
        g_inPracticeMode = false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        ccColor3B tintColor = Mod::get()->getSettingValue<ccColor3B>("TintColor");
        float opacityValue = Mod::get()->getSettingValue<float>("TintOpacity");

        auto tintSprite = CCSprite::create();
        tintSprite->setTextureRect({0, 0, winSize.width, winSize.height});
        tintSprite->setColor(tintColor);
        tintSprite->setOpacity(static_cast<GLubyte>(opacityValue * 255));
        tintSprite->setAnchorPoint({0.f, 0.f});
        tintSprite->setPosition({0.f, 0.f});
        tintSprite->setZOrder(9998);
        this->addChild(tintSprite);

        if (Mod::get()->getSettingValue<bool>("AutoReturn")) {
            float delay = Mod::get()->getSettingValue<float>("AutoReturnDelay");
            log::info("[No End Screen]: AutoReturn enabled — returning in {}s", delay);
            this->runAction(CCSequence::createWithTwoActions(
                CCDelayTime::create(delay),
                CCCallFunc::create(this, callfunc_selector(MyPlayLayer::returnToMenu))
            ));
            return;
        }

        log::info("[No End Screen]: Showing invisible exit button");
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

        std::string destination = Mod::get()->getSettingValue<std::string>("Return Destination");
        if (destination == "Levels List Screen") {
            CCDirector::sharedDirector()->popScene();
        }
        else if (destination == "Current Level Screen") {
            auto scene = GJGarageLayer::scene(); // Fallback
            if (auto pl = GameManager::sharedState()->getPlayLayer()) {
                if (auto level = pl->m_level) {
                    scene = LevelInfoLayer::scene(level, false);
                }
            }
            CCDirector::sharedDirector()->replaceScene(scene);
        }
        else {
            log::warn("[No End Screen]: Unknown destination '{}'", destination);
            CCDirector::sharedDirector()->popScene();
        }
    }

    ~MyPlayLayer() {
        g_inPracticeMode = false;
        log::info("[No End Screen]: Exiting PlayLayer — Practice Mode reset");
    }
};
