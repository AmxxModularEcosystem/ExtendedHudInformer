#if defined __extended_hud_informer_src_paramtype_informer_object_included
    #endinput
#endif
#define __extended_hud_informer_src_paramtype_informer_object_included

#include <amxmodx>
#include <json>
#include <ParamsController>
#include <ExtendedHudInformer>

#include "ExtendedHudInformer/Objects/Informer"

static T_ParamType:InformerParamType = Invalid_ParamType;

DefaultObjects_ParamType_Informer_Register() {
    if (InformerParamType != Invalid_ParamType) {
        return;
    }

    InformerParamType = ParamsController_RegSimpleType(EXHI_INFORMER_PARAM_TYPE, "@DefaultObjects_ParamType_Informer_OnRead");
}

bool:@DefaultObjects_ParamType_Informer_OnRead(const JSON:valueJson) {
    if (!json_is_object(valueJson)) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer value must be an object.");
        return false;
    }

    if (!json_object_has_value(valueJson, "Text")) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer must contain 'Text' field.");
        return false;
    }

    new informerObject[S_Informer];

    // 'Key' не обязателен: без него генерируется уникальный ключ вида "#auto:N".
    // Явный ключ проверяется на дубликат, автоключ уникален по построению.
    PCSingle_ObjShortString(valueJson, "Key", informerObject[Informer_Key], charsmax(informerObject[Informer_Key]));
    if (informerObject[Informer_Key][0] == EOS) {
        Informer_MakeAutoKey(informerObject[Informer_Key], charsmax(informerObject[Informer_Key]));
    } else if (Informer_Find(informerObject[Informer_Key], .orFail = false) != Invalid_ExtendedHudInformer) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer with key '%s' already exists. Skipped.", informerObject[Informer_Key]);
        return false;
    }

    new JSON:textJson = json_object_get_value(valueJson, "Text");
    new bool:isTextString = json_is_string(textJson);
    json_free(textJson);
    if (!isTextString) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer '%s' 'Text' must be a string.", informerObject[Informer_Key]);
        return false;
    }

    informerObject[Informer_Template] = PCSingle_ObjPHTemplate(valueJson, "Text");
    if (informerObject[Informer_Template] == Invalid_PHTemplate) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer '%s' has invalid 'Text'.", informerObject[Informer_Key]);
        return false;
    }

    informerObject[Informer_Color][0] = 200;
    informerObject[Informer_Color][1] = 100;
    informerObject[Informer_Color][2] = 0;
    PCSingle_ObjRGB(valueJson, "Color", informerObject[Informer_Color]);

    informerObject[Informer_X] = PCSingle_ObjFloat(valueJson, "X", -1.0);
    informerObject[Informer_Y] = PCSingle_ObjFloat(valueJson, "Y", 0.35);
    informerObject[Informer_UpdateTime] = PCSingle_ObjFloat(valueJson, "UpdateTime", 1.0);
    informerObject[Informer_HoldTime] = PCSingle_ObjFloat(valueJson, "HoldTime", 0.0);
    informerObject[Informer_FadeInTime] = PCSingle_ObjFloat(valueJson, "FadeInTime", 0.0);
    informerObject[Informer_FadeOutTime] = PCSingle_ObjFloat(valueJson, "FadeOutTime", 0.0);
    informerObject[Informer_Channel] = PCSingle_ObjInt(valueJson, "Channel", -1);
    informerObject[Informer_Enabled] = PCSingle_ObjBool(valueJson, "Enabled", true);
    informerObject[Informer_Sync] = PCSingle_ObjBool(valueJson, "Sync", true);
    informerObject[Informer_DHUD] = PCSingle_ObjBool(valueJson, "DHUD", false);

    if (informerObject[Informer_UpdateTime] <= 0.0) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer '%s' 'UpdateTime' must be positive, using 1.0.", informerObject[Informer_Key]);
        informerObject[Informer_UpdateTime] = 1.0;
    } else if (informerObject[Informer_UpdateTime] < 0.1) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer '%s' 'UpdateTime' is too small, using 0.1.", informerObject[Informer_Key]);
        informerObject[Informer_UpdateTime] = 0.1;
    }

    if (informerObject[Informer_HoldTime] <= 0.0) {
        informerObject[Informer_HoldTime] = informerObject[Informer_UpdateTime] + 0.5;
    }

    if (informerObject[Informer_Channel] < -1 || informerObject[Informer_Channel] > 4) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer '%s' 'Channel' must be in -1..4, using -1 (auto).", informerObject[Informer_Key]);
        informerObject[Informer_Channel] = -1;
    }

    if (informerObject[Informer_DHUD] && informerObject[Informer_Channel] != -1) {
        PCJson_LogForFile(valueJson, "WARNING", "Informer '%s': 'Channel' is ignored when 'DHUD' is enabled.", informerObject[Informer_Key]);
    }

    return ParamsController_SetCell(Informer_Construct(informerObject));
}
