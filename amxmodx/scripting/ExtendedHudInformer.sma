#include <amxmodx>
#include <json>
#include <ParamsController>
#include <ExtendedHudInformer>

#include "ExtendedHudInformer/Objects/Informer"
#include "ExtendedHudInformer/DefaultObjects/Registrar"

public stock const PluginName[] = "Extended HUD Informer";
public stock const PluginVersion[] = EXHI_VERSION;
public stock const PluginAuthor[] = "ArKaNeMaN & DeepSeek";
public stock const PluginURL[] = "https://github.com/AmxxModularEcosystem/ExtendedHudInformer";

static bool:PluginInited = false;
static bool:ForwardsReceived = false;

public plugin_precache() {
    PluginInit();
}

PluginInit() {
    if (PluginInited) {
        return;
    }
    PluginInited = true;

    log_amx("[INFO] Initialize ExtendedHudInformer.");

    register_plugin(PluginName, PluginVersion, PluginAuthor);
    register_library(EXHI_LIBRARY);

    Informer_Init();
    ParamsController_Init();

    // ParamsController рассылает форварды регистрации только один раз, при первом
    // вызове ParamsController_Init(). Если контроллер уже был инициализирован до
    // загрузки этого плагина, форварды были разосланы без нас — регистрируемся вручную.
    if (!ForwardsReceived) {
        log_amx("[INFO] ParamsController was already initialized. Registering ExtendedHudInformer objects manually.");

        DefaultObjects_Register();
    }

    Informer_LoadFromFolder(PCPath_iMakePath(EXHI_CONFIG_PATH));

    log_amx("[INFO] ExtendedHudInformer initialized.");
}

public ParamsController_OnRegisterTypes() {
    ForwardsReceived = true;

    log_amx("[INFO] Registering ExtendedHudInformer's param types.");

    DefaultObjects_ParamType_Informer_Register();

    log_amx("[INFO] ExtendedHudInformer's param types registered.");
}

#include "ExtendedHudInformer/API/Main"
public plugin_natives() {
    API_Main_RegisterNatives();
}
