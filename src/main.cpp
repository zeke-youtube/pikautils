// geode
#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/loader/Setting.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/file.hpp>
#include <chrono>
#include <ctime>
#include <discord_rpc.h>
#include <filesystem>
#include <string>
// sound effect
#include "OiiaCatDeathMp3.hpp"
// vars
constexpr auto APP_ID = "1515238223763345488";
constexpr auto OIIA_CAT_DEATH_SOUND = "oiia-cat-death.mp3";

static bool g_rpcInitialized = false;
static bool g_lastEnabled = false;
static bool g_clickBetweenFramesEnabled = false;
static bool g_processingQueuedButtons = false;
static ListenerHandle* g_enableRpcListener = nullptr;
static ListenerHandle* g_clickBetweenFramesListener = nullptr;
static int64_t g_startTimestamp = 0;
static std::string g_currentDetails;
static std::string g_currentState;
static std::chrono::steady_clock::time_point g_lastDeathSoundTime;
//function
struct PresenceText {
    std::string details;
    std::string state;
};

std::string levelName(GJGameLevel* level) {
    if (!level || level->m_levelName.empty()) {
        return "Unknown Level";
    }

    return level->m_levelName;
}

PresenceText menuPresence() { // main menu
    return {
        "In Main Menu",
        "Browsing menus • via PikaUtils"
    };
}

PresenceText playPresence(PlayLayer* layer) { // playlayer
    auto level = layer ? layer->m_level : nullptr;
    auto name = levelName(level);

    if (level && level->m_dailyID.value() > 0) {
        return {
            level->m_dailyID.value() > 100000 ? "Weekly Demon" : "Daily Level",
            fmt::format("{} • via PikaUtils", name)
        };
    }

    if (layer && layer->m_isPracticeMode) {
        return {
            "Practice Mode",
            fmt::format("{} • via PikaUtils", name)
        };
    }

    if (layer && layer->m_attempts > 0) {
        return {
            fmt::format("Attempt {}", layer->m_attempts),
            fmt::format("{} • via PikaUtils", name)
        };
    }

    return {
        "Playing",
        fmt::format("{} • via PikaUtils", name)
    };
}

PresenceText editorPresence(LevelEditorLayer* layer) { // editor
    auto name = levelName(layer ? layer->m_level : nullptr);

    return {
        "Editing Level",
        fmt::format("[{}] • via PikaUtils", name)
    };
}

PresenceText currentPresence() {
    if (auto playLayer = PlayLayer::get()) {
        return playPresence(playLayer);
    }

    if (auto editorLayer = LevelEditorLayer::get()) {
        return editorPresence(editorLayer);
    }

    return menuPresence();
}

void updatePresence(PresenceText const& presence) {
    if (!g_rpcInitialized) {
        return;
    }

    if (
        presence.details == g_currentDetails &&
        presence.state == g_currentState
    ) {
        return;
    }

    g_currentDetails = presence.details;
    g_currentState = presence.state;

    DiscordRichPresence rpc{};
    rpc.details = g_currentDetails.c_str();
    rpc.state = g_currentState.c_str();
    rpc.startTimestamp = g_startTimestamp;

    Discord_UpdatePresence(&rpc);
}

void startRPC() { //rpc
    if (g_rpcInitialized) {
        return;
    }

    log::info("Starting Discord RPC...");

    DiscordEventHandlers handlers{};

    Discord_Initialize(
        APP_ID,
        &handlers,
        1,
        nullptr
    );

    g_rpcInitialized = true;
    g_startTimestamp = std::time(nullptr);
    g_currentDetails.clear();
    g_currentState.clear();

    updatePresence(currentPresence());

    log::info("Discord RPC started!");
}

void stopRPC() { // still rpc
    if (!g_rpcInitialized) {
        return;
    }

    log::info("Stopping Discord RPC...");

    Discord_ClearPresence();
    Discord_Shutdown();

    g_rpcInitialized = false;
    g_currentDetails.clear();
    g_currentState.clear();

    log::info("Discord RPC stopped!");
}

void applyRPCSetting(bool enabled) {
    if (enabled == g_lastEnabled) {
        return;
    }

    g_lastEnabled = enabled;

    log::info("enable-rpc changed to {}", enabled);

    if (enabled) {
        startRPC();
    }
    else {
        stopRPC();
    }
}

double currentInputTimestamp() { // time
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

void applyClickBetweenFramesSetting(bool enabled) { //cbf
    if (enabled == g_clickBetweenFramesEnabled) {
        return;
    }

    g_clickBetweenFramesEnabled = enabled;
    log::info("click-between-frames changed to {}", enabled);
}

void playOiiaCatDeathSound() { // oiia
    auto now = std::chrono::steady_clock::now();
    if (now - g_lastDeathSoundTime < std::chrono::milliseconds(250)) {
        return;
    }
    g_lastDeathSoundTime = now;

    auto path = Mod::get()->getResourcesDir() / OIIA_CAT_DEATH_SOUND;

    if (!std::filesystem::exists(path)) {
        path = Mod::get()->getConfigDir() / OIIA_CAT_DEATH_SOUND;

        if (!std::filesystem::exists(path)) {
            auto writeResult = utils::file::writeBinary(
                path,
                ByteSpan(OIIA_CAT_DEATH_MP3, OIIA_CAT_DEATH_MP3_SIZE)
            );

            if (!writeResult) {
                log::warn(
                    "Unable to write Oiia Cat death sound: {}",
                    writeResult.unwrapErr()
                );
                return;
            }
        }
    }

    auto pathString = path.generic_string();
    log::debug("Playing Oiia Cat death sound from {}", pathString);
    FMODAudioEngine::sharedEngine()->playEffect(pathString);
}

$on_mod(Loaded) { // on mod load
    g_lastEnabled = Mod::get()->getSettingValue<bool>("enable-rpc");
    g_clickBetweenFramesEnabled =
        Mod::get()->getSettingValue<bool>("click-between-frames");

    if (g_lastEnabled) {
        startRPC();
    }

    g_enableRpcListener = listenForSettingChanges<bool>(
        "enable-rpc",
        [](bool enabled) {
            applyRPCSetting(enabled);
        },
        Mod::get()
    );

    g_clickBetweenFramesListener = listenForSettingChanges<bool>(
        "click-between-frames",
        [](bool enabled) {
            applyClickBetweenFramesSetting(enabled);
        },
        Mod::get()
    );
}

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (g_clickBetweenFramesEnabled && !g_processingQueuedButtons) {
            this->queueButton(
                button,
                down,
                !isPlayer1,
                currentInputTimestamp()
            );
            return;
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        auto oldProcessingQueuedButtons = g_processingQueuedButtons;
        g_processingQueuedButtons = true;

        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        g_processingQueuedButtons = oldProcessingQueuedButtons;
    }
};

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        updatePresence(menuPresence());

        this->schedule(
            schedule_selector(MyMenuLayer::tick),
            0.5f
        );

        return true;
    }

    void tick(float) {
        if (g_rpcInitialized) {
            Discord_RunCallbacks();
        }
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        updatePresence(playPresence(this));

        this->schedule(
            schedule_selector(MyPlayLayer::tick),
            0.5f
        );

        return true;
    }

    void updateAttempts() {
        PlayLayer::updateAttempts();
        updatePresence(playPresence(this));
    }

    void togglePracticeMode(bool practiceMode) {
        PlayLayer::togglePracticeMode(practiceMode);
        updatePresence(playPresence(this));
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        log::debug("PlayLayer::destroyPlayer fired; playing Oiia Cat sound");
        playOiiaCatDeathSound();
    }

    void onQuit() {
        PlayLayer::onQuit();
        updatePresence(menuPresence());
    }

    void tick(float) {
        if (g_rpcInitialized) {
            Discord_RunCallbacks();
        }
    }
};

class $modify(MyLevelEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI)) {
            return false;
        }

        updatePresence(editorPresence(this));

        this->schedule(
            schedule_selector(MyLevelEditorLayer::tick),
            0.5f
        );

        return true;
    }

    void tick(float) {
        if (g_rpcInitialized) {
            Discord_RunCallbacks();
        }
    }
};
