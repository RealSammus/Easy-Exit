#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/cocos.hpp>
#include <filesystem>

using namespace geode::prelude;
using namespace cocos2d;

CCMenuItemSpriteExtra* g_invisibleExitButton = nullptr;

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        if (!Mod::get()->getSettingValue<bool>("ButtonEnabled"))
            return;

        if (PlayLayer::get() && PlayLayer::get()->m_level &&
            PlayLayer::get()->m_level->m_levelType == GJLevelType::Editor)
            return;

        auto btn = geode::cocos::CCMenuItemExt::createSpriteExtraWithFilename(
            "ModMenuButton.png"_spr,
            .6f,
            [](CCObject*) {
                openSettingsPopup(Mod::get());
            }
        );
        btn->setID("easy-exit-button"_spr);

        if (auto rightMenu = typeinfo_cast<CCMenu*>(this->getChildByIDRecursive("right-button-menu"))) {
            rightMenu->removeChild(btn, false);
            rightMenu->addChild(btn, 9999);
            rightMenu->updateLayout();
        }
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

        if (Mod::get()->getSettingValue<bool>("PracticeReturn") &&
            GameManager::sharedState()->getGameVariable("0049")) {
            PlayLayer::levelComplete();
            return;
        }

        PlayLayer::levelComplete();

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        ccColor3B visualColor = Mod::get()->getSettingValue<ccColor3B>("TintColor");
        float visualOpacity = Mod::get()->getSettingValue<float>("TintOpacity");
        std::string visualMode = Mod::get()->getSettingValue<std::string>("TintMode");
        std::string visualStyle = Mod::get()->getSettingValue<std::string>("TintStyle");
        float fadeIn = Mod::get()->getSettingValue<float>("FadeInDuration");
        float fadeOut = Mod::get()->getSettingValue<float>("FadeOutDuration");

        CCSprite* sprite = nullptr;

        if (visualMode == "Full Screen Color") {
            sprite = CCSprite::create();
            sprite->setTextureRect({ 0, 0, winSize.width, winSize.height });
        } 
        else if (visualMode == "Colored Edge Glow") {
            sprite = CCSprite::create("EdgeGlow.png"_spr);
            if (sprite) {
                sprite->setScaleX(winSize.width / sprite->getContentSize().width);
                sprite->setScaleY(winSize.height / sprite->getContentSize().height);
            }
        } 
        else if (visualMode == "Custom Image") {
            auto path = Mod::get()->getSettingValue<std::filesystem::path>("CustomTintImage");
        
            if (!path.empty()) {
                sprite = CCSprite::create(path.string().c_str());
                if (sprite) {
                    sprite->setScaleX(winSize.width / sprite->getContentSize().width);
                    sprite->setScaleY(winSize.height / sprite->getContentSize().height);
                }
            }
        
            if (!sprite) {
                sprite = CCSprite::create();
                sprite->setTextureRect({ 0, 0, winSize.width, winSize.height });
            }
        }        

        if (sprite) {
            sprite->setAnchorPoint({ 0.f, 0.f });
            sprite->setPosition({ 0.f, 0.f });
            sprite->setColor(visualColor);
            sprite->setOpacity(0);
            this->addChild(sprite, 9998);

            CCActionInterval* fadeAction = nullptr;

            if (visualStyle == "Hold") {
                sprite->setOpacity(static_cast<GLubyte>(visualOpacity * 255));
            } 
            else if (visualStyle == "Fade In Hold") {
                fadeAction = CCFadeTo::create(fadeIn, static_cast<GLubyte>(visualOpacity * 255));
            } 
            else if (visualStyle == "Fade In Out") {
                fadeAction = CCSequence::create(
                    CCFadeTo::create(fadeIn, static_cast<GLubyte>(visualOpacity * 255)),
                    CCDelayTime::create(0.3f),
                    CCFadeTo::create(fadeOut, 0),
                    nullptr
                );
            }

            if (fadeAction)
                sprite->runAction(fadeAction);

            addExitButton(sprite);

            bool autoLeave = Mod::get()->getSettingValue<bool>("AutoReturn");
            float returnDelay = Mod::get()->getSettingValue<float>("AutoReturnDelay");

            if (autoLeave) {
                this->runAction(
                    CCSequence::createWithTwoActions(
                        CCDelayTime::create(returnDelay),
                        CCCallFunc::create(this, callfunc_selector(MyPlayLayer::returnToMenu))
                    )
                );
            }
        }
    }

    void addExitButton(CCNode* ref) {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        g_invisibleExitButton = CCMenuItemSpriteExtra::create(
            ref,
            this,
            menu_selector(MyPlayLayer::onInvisibleButtonPressed)
        );
        g_invisibleExitButton->setPosition(winSize.width / 2, winSize.height / 2);

        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        menu->addChild(g_invisibleExitButton);
        this->addChild(menu, 9999);
    }

    void onInvisibleButtonPressed(CCObject*) {
        returnToMenu();
    }

    void returnToMenu() {
        if (!Mod::get()->getSettingValue<bool>("ModEnabled"))
            return;

        this->onQuit();
    }
};