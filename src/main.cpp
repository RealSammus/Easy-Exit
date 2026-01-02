#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/loader/Loader.hpp>
#include <filesystem>
#include <prevter.imageplus/include/api.hpp>

using namespace geode::prelude;
using namespace cocos2d;

FMOD::Channel* g_completionAudioChannel = nullptr;

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        if (!Mod::get()->getSettingValue<bool>("ButtonEnabled"))
            return;

        auto btn = geode::cocos::CCMenuItemExt::createSpriteExtraWithFilename(
            "ModMenuButton.png"_spr, .67f, [](CCObject*) {
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
    class AnyKeyListener : public CCLayer {
    public:
        MyPlayLayer* m_owner = nullptr;
        bool m_armed = false;

        static AnyKeyListener* create(MyPlayLayer* owner) {
            auto p = new AnyKeyListener();
            if (p && p->initWithOwner(owner)) {
                p->autorelease();
                return p;
            }
            CC_SAFE_DELETE(p);
            return nullptr;
        }

        bool initWithOwner(MyPlayLayer* owner) {
            if (!CCLayer::init()) return false;
            m_owner = owner;
        #ifndef __APPLE__   // <-- add
            CCDirector::sharedDirector()->getKeyboardDispatcher()->addDelegate(this);
        #endif              // <-- add
            return true;
        }
        
        void onExit() override {
        #ifndef __APPLE__   // <-- add
            CCDirector::sharedDirector()->getKeyboardDispatcher()->removeDelegate(this);
        #endif              // <-- add
            CCLayer::onExit();
        }        

        void keyDown(enumKeyCodes) override {
            if (m_owner && m_armed) {
                m_owner->returnToMenu();
            }
        }
    };

    struct Fields {
        CCMenuItemSpriteExtra* m_invisibleExitButton = nullptr;
        AnyKeyListener* m_anyKeyListener = nullptr;
    };

    void onEnterTransitionDidFinish() override {
        PlayLayer::onEnterTransitionDidFinish();

        if (m_fields->m_anyKeyListener) {
            m_fields->m_anyKeyListener->removeFromParent();
            m_fields->m_anyKeyListener = nullptr;
        }
    }

    void levelComplete() {
        if (!Mod::get()->getSettingValue<bool>("ModEnabled")) {
            PlayLayer::levelComplete();
            return;
        }

        if (Mod::get()->getSettingValue<bool>("PracticeReturn") && m_isPracticeMode) {
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
        } else if (visualMode == "Colored Edge Glow") {
            sprite = CCSprite::create("EdgeGlow.png"_spr);
            if (sprite) {
                sprite->setScaleX(winSize.width / sprite->getContentSize().width);
                sprite->setScaleY(winSize.height / sprite->getContentSize().height);
            }
        } else if (visualMode == "Custom Image") {
            auto path = Mod::get()->getSettingValue<std::filesystem::path>("CustomTintImage");
            if (!path.empty()) {
                sprite = CCSprite::create(geode::utils::string::pathToString(path).c_str());
                if (!sprite) {
                    sprite = CCSprite::create();
                    if (sprite) sprite->setTextureRect({ 0, 0, winSize.width, winSize.height });
                } else {
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
            } else if (visualStyle == "Fade In Hold") {
                fadeAction = CCFadeTo::create(fadeIn, static_cast<GLubyte>(visualOpacity * 255));
            } else if (visualStyle == "Fade In Out") {
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

            if (!m_fields->m_anyKeyListener) {
                m_fields->m_anyKeyListener = AnyKeyListener::create(this);
                if (m_fields->m_anyKeyListener) {
                    m_fields->m_anyKeyListener->m_armed = true;
                    this->addChild(m_fields->m_anyKeyListener, 10000);
                }
            } else {
                m_fields->m_anyKeyListener->m_armed = true;
            }

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

            auto audioPath = Mod::get()->getSettingValue<std::filesystem::path>("CustomCompletionAudio");
            float volume = Mod::get()->getSettingValue<float>("AudioVolume");
            float speed = Mod::get()->getSettingValue<float>("AudioSpeed");

            if (!audioPath.empty()) {
                FMOD::System* system = FMODAudioEngine::sharedEngine()->m_system;
                FMOD::Sound* sound = nullptr;

                std::string pathStr = audioPath.string();

                if (system->createSound(pathStr.c_str(), FMOD_DEFAULT, nullptr, &sound) == FMOD_OK) {

                    if (Mod::get()->getSettingValue<bool>("PauseNormalMusic")) {
                        FMOD::ChannelGroup* masterGroup = nullptr;
                        if (system->getMasterChannelGroup(&masterGroup) == FMOD_OK && masterGroup) {
                            masterGroup->stop();
                        }
                    }

                    if (system->playSound(sound, nullptr, false, &g_completionAudioChannel) == FMOD_OK) {
                        if (g_completionAudioChannel) {
                            g_completionAudioChannel->setVolume(volume);
                            float baseFreq;
                            if (g_completionAudioChannel->getFrequency(&baseFreq) == FMOD_OK) {
                                g_completionAudioChannel->setFrequency(baseFreq * speed);
                            }
                        }
                    }
                }
            }
        }
    }

    void addExitButton(CCNode* ref) {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_fields->m_invisibleExitButton = CCMenuItemSpriteExtra::create(
            ref,
            this,
            menu_selector(MyPlayLayer::onInvisibleButtonPressed)
        );
        m_fields->m_invisibleExitButton->setPosition(winSize.width / 2, winSize.height / 2);

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        menu->addChild(m_fields->m_invisibleExitButton);
        this->addChild(menu, 9999);
    }

    void onInvisibleButtonPressed(CCObject*) {
        returnToMenu();
    }

    void returnToMenu() {
        if (!Mod::get()->getSettingValue<bool>("ModEnabled"))
            return;

        if (m_fields->m_anyKeyListener) {
            m_fields->m_anyKeyListener->m_armed = false;
        }

        this->onQuit();
    }

    void onQuit() {
        if (Mod::get()->getSettingValue<bool>("AudioStyle")) {
            if (g_completionAudioChannel) {
                bool isPlaying = false;
                g_completionAudioChannel->isPlaying(&isPlaying);
                if (isPlaying)
                    g_completionAudioChannel->stop();
                g_completionAudioChannel = nullptr;
            }
        }

        if (m_fields->m_anyKeyListener) {
            m_fields->m_anyKeyListener->removeFromParent();
            m_fields->m_anyKeyListener = nullptr;
        }

        PlayLayer::onQuit();
    }
};