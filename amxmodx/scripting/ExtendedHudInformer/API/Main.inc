#if defined __extended_hud_informer_src_api_main_included
    #endinput
#endif
#define __extended_hud_informer_src_api_main_included

#include <amxmodx>
#include <ParamsController>
#include <ExtendedHudInformer>

#include "ExtendedHudInformer/Objects/Informer"

API_Main_RegisterNatives() {
    register_native("ExtendedHudInformer_Init", "@API_Init");
    register_native("ExtendedHudInformer_Find", "@API_Find");
    register_native("ExtendedHudInformer_IsEnabled", "@API_IsEnabled");
    register_native("ExtendedHudInformer_SetEnabled", "@API_SetEnabled");
    register_native("ExtendedHudInformer_Show", "@API_Show");
}

@API_Init() {
    PluginInit();
}

T_ExtendedHudInformer:@API_Find() {
    enum { Arg_Key = 1 }

    new key[EXHI_INFORMER_KEY_MAX_LEN];
    get_string(Arg_Key, key, charsmax(key));

    return Informer_Find(key, .orFail = false);
}

bool:@API_IsEnabled() {
    enum { Arg_Informer = 1 }

    return Informer_IsEnabled(T_ExtendedHudInformer:get_param(Arg_Informer));
}

@API_SetEnabled() {
    enum { Arg_Informer = 1, Arg_Enabled }

    Informer_SetEnabled(T_ExtendedHudInformer:get_param(Arg_Informer), bool:get_param(Arg_Enabled));
}

@API_Show() {
    enum { Arg_Informer = 1, Arg_Player }

    Informer_Show(T_ExtendedHudInformer:get_param(Arg_Informer), get_param(Arg_Player));
}
