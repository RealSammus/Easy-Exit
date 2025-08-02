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
#include <filesystem>

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
            sprite = CCSprite::create("BorderGlow.png"_spr);
            if (sprite) {
                sprite->setScaleX(winSize.width / sprite->getContentSize().width);
                sprite->setScaleY(winSize.height / sprite->getContentSize().height);
            }
        } 
        else if (visualMode == "Custom Image") {
            auto path = Mod::get()->getSettingValue<std::filesystem::path>("CustomTintImage");

            if (!path.empty()) {
                sprite = CCSprite::create(path.string().c_str());
                if (!sprite) {
                    sprite = CCSprite::create("FallbackImage.png"_spr);
                }
            } else {
                sprite = CCSprite::create("FallbackImage.png"_spr);
            }

            if (sprite) {
                sprite->setScaleX(winSize.width / sprite->getContentSize().width);
                sprite->setScaleY(winSize.height / sprite->getContentSize().height);
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

            // Automatic return after delay
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
        } else {
            log::error("No sprite could be loaded for visual mode '{}'", visualMode);
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

        FMODAudioEngine::sharedEngine()->clearAllAudio();
        GameManager::sharedState()->playMenuMusic();

        std::string destination = Mod::get()->getSettingValue<std::string>("ReturnDestination");
        if (destination.empty()) destination = "Levels List Screen";

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
