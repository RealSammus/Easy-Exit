#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;
using namespace cocos2d;

CCMenuItemSpriteExtra* g_invisibleExitButton = nullptr;
bool g_inPracticeMode = false;

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
    
        if (!Mod::get()->getSettingValue<bool>("ButtonEnabled"))
            return;
    
        if (PlayLayer::get() && PlayLayer::get()->m_level &&
            PlayLayer::get()->m_level->m_levelType == GJLevelType::Editor)
            return;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
    
        auto btn = geode::cocos::CCMenuItemExt::createSpriteExtraWithFilename(
            "ModMenuButton.png"_spr,
            1.f,
            [](CCObject*) {
                openSettingsPopup(Mod::get());
            }
        );
    
        btn->setPosition({ winSize.width - 523.f, winSize.height - 44.f });
    
        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        menu->addChild(btn);
        menu->setZOrder(9999);
        btn->setZOrder(9999);
        this->addChild(menu, 9999);
    }    

    void onPracticeMode(CCObject* sender) {
        PauseLayer::onPracticeMode(sender);
        bool newState = GameManager::sharedState()->getGameVariable("0049");
        g_inPracticeMode = newState;
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
        if (!Mod::get()->getSettingValue<bool>("ModEnabled")) {
            PlayLayer::levelComplete();
            return;
        }

        if (m_level && m_level->m_levelType == GJLevelType::Editor) {
            PlayLayer::levelComplete();
            return;
        }

        if (Mod::get()->getSettingValue<bool>("PracticeReturn") && g_inPracticeMode) {
            PlayLayer::levelComplete();
            return;
        }

        PlayLayer::levelComplete();
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
            this->runAction(CCSequence::createWithTwoActions(
                CCDelayTime::create(delay),
                CCCallFunc::create(this, callfunc_selector(MyPlayLayer::returnToMenu))
            ));
            return;
        }

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
        returnToMenu();
    }

    void returnToMenu() {
        if (!Mod::get()->getSettingValue<bool>("ModEnabled"))
            return;

        FMODAudioEngine::sharedEngine()->clearAllAudio();
        GameManager::sharedState()->playMenuMusic();

        std::string destination = Mod::get()->getSettingValue<std::string>("ReturnDestination");

        if (destination.empty()) {
            destination = "Levels List Screen";
        }

        GJGameLevel* lastLevel = m_level;

        if (destination == "Levels List Screen") {
            if (lastLevel && static_cast<int>(lastLevel->m_levelType) == 1) {
                auto searchObj = GJSearchObject::create(static_cast<SearchType>(2));
                CCDirector::sharedDirector()->replaceScene(LevelBrowserLayer::scene(searchObj));
            } else {
                CCDirector::sharedDirector()->popScene();
            }
        }
        else if (destination == "Current Level Screen") {
            if (lastLevel) {
                CCDirector::sharedDirector()->replaceScene(LevelInfoLayer::scene(lastLevel, false));
            } else {
                CCDirector::sharedDirector()->popScene();
            }
        }
        else {
            CCDirector::sharedDirector()->popScene();
        }
    }

    ~MyPlayLayer() {
        g_inPracticeMode = false;
    }
};