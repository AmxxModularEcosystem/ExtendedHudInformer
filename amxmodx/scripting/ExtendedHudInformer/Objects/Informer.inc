#if defined __extended_hud_informer_src_objects_informer_included
    #endinput
#endif
#define __extended_hud_informer_src_objects_informer_included

#include <amxmodx>
#include <json>
#include <regex>
#include <ParamsController>
#include <ExtendedHudInformer>

enum _:S_Informer {
    Informer_Key[EXHI_INFORMER_KEY_MAX_LEN],
    T_PHTemplate:Informer_Template,
    Informer_Color[3],
    Float:Informer_X,
    Float:Informer_Y,
    Float:Informer_UpdateTime,
    Float:Informer_HoldTime,
    Float:Informer_FadeInTime,
    Float:Informer_FadeOutTime,
    Informer_Channel,
    bool:Informer_Sync,
    bool:Informer_DHUD,
    Informer_SyncObj,
    bool:Informer_Enabled,
}

static Array:Informers = Invalid_Array;
static Trie:InformersMap = Invalid_Trie;

Informer_Init() {
    if (Informers == Invalid_Array) {
        Informers = ArrayCreate(S_Informer, 1);
    }

    if (InformersMap == Invalid_Trie) {
        InformersMap = TrieCreate();
    }
}

static Informer_AutoKeyCounter = 0;

/**
 * Генерация уникального ключа для информера без явного "Key".
 *
 * Формат: "#auto:N" (N — сквозной счётчик в рамках запуска плагина).
 * Счётчик пропускает значения, которые уже заняты в карте информеров
 * (например, если такой ключ задан явно в конфиге).
 */
Informer_MakeAutoKey(out[], const outLen) {
    do {
        ++Informer_AutoKeyCounter;
        formatex(out, outLen, "#auto:%d", Informer_AutoKeyCounter);
    } while (InformersMap != Invalid_Trie && TrieKeyExists(InformersMap, out));
}

Informer_LoadFromFolder(const path[]) {
    static pathNonConst[PLATFORM_MAX_PATH];
    copy(pathNonConst, charsmax(pathNonConst), path);

    new file[PLATFORM_MAX_PATH], dir, FileType:fileType;
    dir = open_dir(pathNonConst, file, charsmax(file), fileType);
    if (!dir) {
        abort(AMX_ERR_PARAMS, "[ERROR] Can't open folder '%s'.", path);
        return;
    }

    new Regex:iRegEx_FileName, ret;
    iRegEx_FileName = regex_compile("(.+).json$", ret, "", 0, "i");

    do {
        if (file[0] == '!' || file[0] == '.') {
            continue;
        }

        if (fileType == FileType_Directory) {
            Informer_LoadFromFolder(PCPath_iMakePath(file, path));
        } else if (fileType == FileType_File) {
            if (regex_match_c(file, iRegEx_FileName) > 0) {
                regex_substr(iRegEx_FileName, 1, file, charsmax(file));

                Informer_LoadFromFile(PCPath_iMakePath(file, path, "json"));
            }
        }
    } while (next_file(dir, file, charsmax(file), fileType));

    regex_free(iRegEx_FileName);
    close_dir(dir);
}

Informer_LoadFromFile(const path[]) {
    new JSON:informersJson = PCJson_ParseFile(path, PCPath_iMakePath(EXHI_CONFIG_PATH));
    if (informersJson == Invalid_JSON) {
        abort(AMX_ERR_PARAMS, "[ERROR] Can't parse file '%s'.", path);
        return;
    }

    Informer_LoadFromJson(informersJson);

    PCJson_Free(informersJson);
}

Informer_LoadFromJson(const JSON:informersJson) {
    if (json_is_object(informersJson)) {
        new Trie:p = PCSingle_Read(informersJson, EXHI_INFORMER_PARAM_TYPE);
        if (p == Invalid_Trie || !TrieKeyExists(p, PCSINGLE_PARAM_KEY)) {
            PCJson_LogForFile(informersJson, "WARNING", "Error while reading informer object.");
        }

        TrieDestroy(p);
    } else if (json_is_array(informersJson)) {
        for (new i = 0, ii = json_array_get_count(informersJson); i < ii; ++i) {
            new JSON:informerJson = json_array_get_value(informersJson, i);
            Informer_LoadFromJson(informerJson);
            json_free(informerJson);
        }
    } else {
        PCJson_LogForFile(informersJson, "WARNING", "Informer file must contain an object or array of objects.");
    }
}

T_ExtendedHudInformer:Informer_Construct(informerObject[S_Informer]) {
    // -1 — sync-объект не создан (вывод через dHUD или классический канал)
    if (!informerObject[Informer_DHUD] && informerObject[Informer_Sync]) {
        informerObject[Informer_SyncObj] = CreateHudSyncObj();
    } else {
        informerObject[Informer_SyncObj] = -1;
    }

    new T_ExtendedHudInformer:informer = T_ExtendedHudInformer:ArrayPushArray(Informers, informerObject);
    TrieSetCell(InformersMap, informerObject[Informer_Key], informer);
    TrieSetCell(InformersMap, fmt("#%d", informer), informer);

    if (informerObject[Informer_Enabled]) {
        Informer_StartTask(informer);
    }

    return informer;
}

Informer_Get(const T_ExtendedHudInformer:informer, informerObject[S_Informer]) {
    ArrayGetArray(Informers, _:informer, informerObject);
}

bool:Informer_IsValid(const T_ExtendedHudInformer:informer) {
    return informer != Invalid_ExtendedHudInformer
        && _:informer >= 0
        && Informers != Invalid_Array
        && _:informer < ArraySize(Informers);
}

T_ExtendedHudInformer:Informer_Find(const key[], const bool:orFail = true) {
    if (InformersMap == Invalid_Trie || !TrieKeyExists(InformersMap, key)) {
        if (orFail) {
            abort(AMX_ERR_PARAMS, "Informer not found: %s", key);
        }

        return Invalid_ExtendedHudInformer;
    }

    new T_ExtendedHudInformer:informer;
    TrieGetCell(InformersMap, key, informer);

    return informer;
}

bool:Informer_IsEnabled(const T_ExtendedHudInformer:informer) {
    if (!Informer_IsValid(informer)) {
        return false;
    }

    new informerObject[S_Informer];
    Informer_Get(informer, informerObject);

    return informerObject[Informer_Enabled];
}

Informer_SetEnabled(const T_ExtendedHudInformer:informer, const bool:enabled) {
    if (!Informer_IsValid(informer)) {
        return;
    }

    new informerObject[S_Informer];
    Informer_Get(informer, informerObject);
    if (informerObject[Informer_Enabled] == enabled) {
        return;
    }

    informerObject[Informer_Enabled] = enabled;

    if (enabled) {
        Informer_StartTask(informer);
    } else {
        // +1: task id 0 не должен использоваться (хрупкая семантика remove_task по id 0)
        remove_task(_:informer + 1);
    }

    ArraySetArray(Informers, _:informer, informerObject);
}

static Informer_StartTask(const T_ExtendedHudInformer:informer) {
    new informerObject[S_Informer];
    Informer_Get(informer, informerObject);

    // +1: первый информер имеет хендлер 0, а id 0 нельзя использовать как task id
    // (set_task/remove_task с id 0 опираются на недокументированную семантику)
    set_task(informerObject[Informer_UpdateTime], "@Informer_Task", _:informer + 1, .flags = "b");

    Informer_Show(informer);
}

@Informer_Task(const taskId) {
    new T_ExtendedHudInformer:informer = T_ExtendedHudInformer:(taskId - 1);
    
    Informer_Show(informer);
}

Informer_Show(const T_ExtendedHudInformer:informer, const playerIndex = 0) {
    if (!Informer_IsValid(informer)) {
        return;
    }

    new informerObject[S_Informer];
    Informer_Get(informer, informerObject);

    new bool:useDhud = informerObject[Informer_DHUD];

    // Канал задаётся всегда: классический show_hudmessage его использует,
    // а ShowSyncHudMsg (ветка Sync) игнорирует — отдельная ветка не нужна.
    if (useDhud) {
        set_dhudmessage(
            informerObject[Informer_Color][0],
            informerObject[Informer_Color][1],
            informerObject[Informer_Color][2],
            informerObject[Informer_X],
            informerObject[Informer_Y],
            .holdtime = informerObject[Informer_HoldTime],
            .fadeintime = informerObject[Informer_FadeInTime],
            .fadeouttime = informerObject[Informer_FadeOutTime]
        );
    } else {
        set_hudmessage(
            informerObject[Informer_Color][0],
            informerObject[Informer_Color][1],
            informerObject[Informer_Color][2],
            informerObject[Informer_X],
            informerObject[Informer_Y],
            .holdtime = informerObject[Informer_HoldTime],
            .fadeintime = informerObject[Informer_FadeInTime],
            .fadeouttime = informerObject[Informer_FadeOutTime],
            .channel = informerObject[Informer_Channel]
        );
    }

    if (playerIndex > 0) {
        if (is_user_connected(playerIndex)) {
            Informer_ShowToPlayer(playerIndex, informerObject);
        }
    } else {
        for (new i = 1; i <= MaxClients; ++i) {
            if (is_user_connected(i)) {
                Informer_ShowToPlayer(i, informerObject);
            }
        }
    }
}

static Informer_ShowToPlayer(const playerIndex, const informerObject[S_Informer]) {
    static T_PHGroup:playerPHGroup = Invalid_PHGroup;
    if (playerPHGroup == Invalid_PHGroup) {
        playerPHGroup = PCPH_FindGroup(DEFAULT_PH_PLAYER_GROUP_PREFIX);
    }

    new text[EXHI_HUD_TEXT_MAX_LEN];

    if (playerPHGroup != Invalid_PHGroup) {
        PCPH_Group_PushIntContext(playerPHGroup, playerIndex);
    }
    PCPH_FormatTemplate(informerObject[Informer_Template], text, charsmax(text));
    if (playerPHGroup != Invalid_PHGroup) {
        PCPH_Group_PopContext(playerPHGroup);
    }

    if (informerObject[Informer_DHUD]) {
        show_dhudmessage(playerIndex, text);
    } else if (informerObject[Informer_Sync] && informerObject[Informer_SyncObj] != -1) {
        ShowSyncHudMsg(playerIndex, informerObject[Informer_SyncObj], "%s", text);
    } else {
        show_hudmessage(playerIndex, text);
    }
}
