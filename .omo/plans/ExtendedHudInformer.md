# ExtendedHudInformer — План реализации и ТЗ

## 1. Цель

Плагин **ExtendedHudInformer** для AMX Mod X 1.10 (Pawn). Демонстрация и практическое использование
нового механизма плейсхолдеров `AmxxModularEcosystem/ParamsController@1.5.0-alpha.1`:

- вывод игрокам **HUD-информеров** — персистентных `hud`-сообщений (НЕ `dhud`), которые обновляются с заданной частотой;
- текст с плейсхолдерами, частота обновления, цвет и позиция настраиваются в JSON-конфиге через ParamsController;
- конфиг позволяет создавать **несколько информеров**;
- плагин пригоден для реальных серверов.

## 2. Стек и окружение

- AMXX 1.10.x, компилятор amxxpc, сборка через **amxx-builder** (`amxbuild.yml`), CI уже настроен (`.github/workflows/ci.yml` → `AmxxModularEcosystem/amxx-builder@v1`).
- Зависимость (deps в amxbuild.yml): `AmxxModularEcosystem/ParamsController@1.5.0-alpha.1` (git, include_path автодетектится: `amxmodx/scripting/include/ParamsController.inc`).
- Стиль кода — по образцу `AmxxModularEcosystem/ExtendedShop` (модульные `.inc`, префикс констант `EXHI_`, функции `Module_*`, хендлеры `@`, логирование `[INFO]`).

## 3. Проверенные факты (основание решений)

1. **ParamsController_Init()** идемпотентен; форварды `ParamsController_OnRegisterTypes`, `PCPH_OnRegisterGroups`, `PCPH_OnRegisterProxyGroups`, `ParamsController_OnInited` раздаются ровно один раз при первом `ParamsController_Init()` (`Forwards_RegAndCall` → CreateMultiForward + ExecuteForward + Destroy). ParamsController сам не вызывает Init (только через натив).
2. ParamsController должен быть загружен раньше плагина (резолв natives при загрузке) → мой плагин грузится ПОСЛЕ ParamsController. Возможны два сценария: (a) ParamsController уже инициализирован другим потребителем до загрузки моего плагина — форварды разосланы без меня; (b) мой `ParamsController_Init()` впервые запускает инициализацию — форварды приходят в мой плагин. → **dual-path init**: регистрация через форварды, а при срабатывании флага «форварды не приходили» — ручной повтор. Все функции регистрации идемпотентны (guard по статик-хендлерам), чтобы повторная рассылка форвардов была безвредной.
3. Колбеки чтения типов параметров вызываются с 4 аргументами `(const JSON:valueJson, const Trie:p, const key[], const tag[])`; в Pawn можно объявлять меньше параметров (ExtendedShop объявляет 1).
4. Тип **"PH-Template"**: читает строку (до `PARAM_VALUE_MAX_LEN`=512) и сразу компилирует её в `T_PHTemplate` (хендлер кладётся в trie через `SetCell`). Скомпилированный шаблон кеширует группы/форварды; **незарегистрированные на момент компиляции ключи фиксируются как литеральный текст** → компиляция только после регистрации всех плейсхолдеров.
5. `PCPH_FormatTemplate` читает **текущий контекст группы** на момент форматирования (`PHGroup_FillValue` берёт `PHGroup_CurrentContext`) → паттерн «push контекст игрока → формат → pop» корректен для скомпилированных шаблонов.
6. Для проактивных ключей без заданного значения выводится **пустая строка** (не литерал).
7. `PCSingle_ObjRGB` при отсутствии поля **не заполняет** `out[3]` → перед вызовом инициализировать дефолтом.
8. `get_user_frags(index)` в AMXX 1.10 возвращает только фраги (без deaths) → плейсхолдер `deaths` исключён.
9. `set_task(Float:time, func, id, ...)`: если data не передано, колбек вызывается с `id` в качестве первого аргумента; `remove_task(id)` удаляет по переданному id. → использую `id = _:(informer)`, `remove_task(_:(informer))` — хранить id задачи в структуре не нужно.
10. `set_hudmessage` параметры — глобальные (общие для всех плагинов) → устанавливать перед каждым показом. Для перманентного сообщения рекомендуется фиксированный канал (0-3) — избегает мигания. Канал 4 — служебный (радар). `next_hudchannel` возвращает 1-4 (авто-каналы).
11. `{map}`/`{real-map}` — проактивные константы, устанавливаются при инициализации ParamsController один раз → на смене карты устаревают → обновляю их в `plugin_cfg` через `PCPH_Set` (публичный API, ключи уже зарегистрированы).
12. Стандарт stdlib AMXX 1.10.5479: `get_user_health/armor/frags/ping/time`, `get_systime`, `format_time`, `get_cvar_string` (cvars.inc), `is_user_connected`, `set_task`, `forward plugin_cfg()`, `forward plugin_end()` — все подтверждены.
13. Плейсхолдер-колбеки: сигнатура `(const key[], const contextKey[])`, внутри обязательно вызвать `PCPH_Cb_Set*`. Буферы в колбеках — только `new` (не `static`) из-за возможной реентерабельности форматтера.
14. `PCPath_iMakePath("ExtendedHudInformer")` → `<amxx_configsdir>/plugins/ExtendedHudInformer` (куда мапится `amxmodx/configs/plugins/ExtendedHudInformer` в репозитории).

## 4. ТЗ — функциональные требования

### 4.1 Конфигурация
- Папка конфигов: `configs/plugins/ExtendedHudInformer/` (в репо — `amxmodx/configs/plugins/ExtendedHudInformer/`).
- Загрузка: рекурсивный обход папки, файлы `*.json`, пропуск файлов, начинающихся с `!` или `.` (паттерн ExtendedShop `Product_LoadFromFolder`).
- Корень файла — JSON-объект или массив объектов-информеров.
- Чтение через пользовательский тип параметра **`ExtendedHudInformer-Informer`** (read callback читает объект, конструирует информер, возвращает хендлер через `ParamsController_SetCell`).

Поля объекта информера (чтение через PCSingle_*):

| Поле | Тип | Обязательно | По умолчанию | Описание |
|---|---|---|---|---|
| `Key` | ShortString | да | — | Уникальный ключ (макс. 64), дубликаты пропускаются с WARNING |
| `Text` | PH-Template | да | — | Текст с плейсхолдерами (до 512 симв.), компилируется в шаблон |
| `Color` | RGB | нет | `[200, 100, 0]` | Цвет HUD (массив `[R,G,B]`, объект `{R,G,B}`/`{Red,Green,Blue}` или строка `"R,G,B"`) |
| `X` | Float | нет | `-1.0` | Позиция X (в процентах, `-1.0` = по центру) |
| `Y` | Float | нет | `0.35` | Позиция Y |
| `UpdateTime` | Float | нет | `1.0` | Частота обновления, сек. Если `<= 0` → WARNING + `1.0` |
| `HoldTime` | Float | нет | `0` (авто) | Время удержания. `<= 0` → `UpdateTime + 0.5` (без мигания) |
| `FadeInTime` | Float | нет | `0.0` | Время появления |
| `FadeOutTime` | Float | нет | `0.0` | Время исчезания |
| `Channel` | Integer | нет | `-1` (авто) | HUD-канал. Вне `-1..4` → WARNING + `-1`. Рекомендация 0-3 |
| `Enabled` | Boolean | нет | `true` | Включён ли информер (задача запускается/не запускается) |

- Обязательные поля (`Key`, `Text`): при отсутствии — `PCJson_LogForFile` WARNING, объект пропускается (без abort).
- Ошибка парсинга JSON-файла целиком → `abort` (стиль ExtendedShop).
- Дубликат `Key` → WARNING, шаблон уничтожается (`PCPH_DestroyTemplate`), объект пропускается.

### 4.2 Отображение
- На каждый включённый информер — повторяющаяся задача `set_task(UpdateTime, "@Informer_Task", _:informer, .flags = "b")` + немедленный первый показ.
- Тик задачи: `set_hudmessage(...)` один раз → цикл по подключённым игрокам → для каждого: `PCPH_PushIntContext(PlayerPHGroup, i)` → `PCPH_FormatTemplate(tmpl, text, 512)` → `PCPH_PopContext(PlayerPHGroup)` → `show_hudmessage(i, text)`.
- Буфер текста HUD — 512 (EXHI_HUD_TEXT_MAX_LEN).
- `plugin_end()`: для всех информеров `remove_task(_:(informer))` + `PCPH_DestroyTemplate`, затем освобождение Array/Trie.

### 4.3 Плейсхолдеры (демонстрация механизма)
Встроенные (ParamsController) — доступны из коробки: `{map}`, `{real-map}`, `{server-name}`, `{server-ip}`, `{date}`, `{time}`, `{datetime}`, `{players-count}`, `{max-players-count}`, `{p:authid}`, `{p:name}`, `{p:ip}`, `{p:userid}`.

Собственные (регистрируются в `PCPH_OnRegisterGroups` / fallback):
- **Группа `p`** (существующая, расширяется реактивными ключами): `{p:health}`, `{p:armor}`, `{p:frags}`, `{p:ping}`, `{p:connect-time}` (формат `H:MM:SS`). Все — реактивные колбеки, контекст — индекс игрока.
- **Новая группа `sv`** (демонстрация регистрации своей группы): `{sv:uptime}` (время аптайма сервера, формат `[Dd ]HH:MM:SS`), `{sv:nextmap}` (из cvar `amx_nextmap`, пусто → `-`).

Константы ключей/префиксов — в публичном include (`EXHI_PH_*`).

### 4.4 Обновление `{map}`/`{real-map}` на смене карты
`public plugin_cfg()`: через `PCPH_Set(PCPH_GetGlobalGroup(), DEFAULT_PH_GLOBAL_MAP_KEY / DEFAULT_PH_GLOBAL_REAL_MAP_KEY, mapname)` (guard: группа валидна).

### 4.5 Публичный API (natives + include)
`amxmodx/scripting/include/ExtendedHudInformer.inc`:
- `EXHI_VERSION` "1.0.0"; `EXHI_LIBRARY` "ExtendedHudInformer"; `EXHI_INFORMER_KEY_MAX_LEN` 64; `EXHI_CONFIG_PATH` "ExtendedHudInformer"; `EXHI_INFORMER_PARAM_TYPE` "ExtendedHudInformer-Informer"; константы `EXHI_PH_*`.
- `enum T_ExtendedHudInformer { Invalid_ExtendedHudInformer = -1 }`.
- Natives: `ExtendedHudInformer_Init()`, `ExtendedHudInformer_Find(const key[])`, `bool:ExtendedHudInformer_IsEnabled(informer)`, `ExtendedHudInformer_SetEnabled(informer, bool)`, `ExtendedHudInformer_Show(informer, playerIndex = 0)`.
- Валидация хендлера в natives (диапазон индексов) — защита от мусорных хендлеров от других плагинов.
- `register_library(EXHI_LIBRARY)` в PluginInit; `plugin_natives()` → `API_Main_RegisterNatives()`.

### 4.6 Пример конфига
`amxmodx/configs/plugins/ExtendedHudInformer/ExampleInformers.json` — 3 информера:
1. welcome (центр-верх, `{server-name}`, `{map}`, `{players-count}`, `{time}`, канал 1);
2. player-stats (верх-лево, `{p:health}`, `{p:armor}`, `{p:frags}`, `{p:ping}`, `{p:connect-time}`, UpdateTime 0.5, канал 2);
3. server-info (низ-лево, `{sv:uptime}`, `{sv:nextmap}`, UpdateTime 5.0, канал 3).

## 5. Структура файлов

```
amxbuild.yml                                # name, version, deps (ParamsController@1.5.0-alpha.1)
README.md                                   # полная документация (см. 4.7)
amxmodx/scripting/ExtendedHudInformer.sma   # main: PluginInit, plugin_precache/cfg/end, публичные форварды
amxmodx/scripting/include/ExtendedHudInformer.inc   # публичный API
amxmodx/scripting/ExtendedHudInformer/
  Objects/Informer.inc                      # S_Informer, Array+Trie хранилище, Construct/Find/Get/Show/SetEnabled
  DefaultObjects/Registrar.inc              # public ParamsController_OnRegisterTypes + подключение модулей
  DefaultObjects/ParamType/InformerObject.inc  # тип ExtendedHudInformer-Informer
  DefaultObjects/Placeholder/Player.inc     # p:health/armor/frags/ping/connect-time
  DefaultObjects/Placeholder/Server.inc     # sv:uptime/nextmap
  API/Main.inc                              # регистрация natives
amxmodx/configs/plugins/ExtendedHudInformer/ExampleInformers.json
```

Порядок инициализации (plugin_precache → PluginInit):
1. `Informer_Init()` (создание Array/Trie, без зависимостей);
2. `register_plugin(...)`, `g_iServerStartTime = get_systime()`;
3. `ParamsController_Init()` — если не инициализирован, форварды приходят в мой плагин: OnRegisterTypes → тип, PCPH_OnRegisterGroups → плейсхолдеры, OnInited → `ConfigInit()` (загрузка конфигов, запуск задач);
4. если `ForwardsReceived == false` (ParamsController уже был инициализирован до загрузки плагина) → ручной вызов: регистрация типа, плейсхолдеров, `ConfigInit()`;
5. `register_library`.

Все функции регистрации идемпотентны (guard по статик-хендлерам) — повторная рассылка форвардов безвредна.

## 6. README.md (содержание)

- Назначение, скриншот-описание (текст), требования (AMXX 1.10, ParamsController@1.5.0-alpha.1, порядок plugins.ini).
- Установка (копирование файлов, подключение ParamsController + ExtendedHudInformer в plugins.ini).
- Конфигурация: таблица полей, пример конфига, лимиты (Text ≤ 512, каналы, HoldTime-авто).
- Плейсхолдеры: таблица встроенных ParamsController + собственных (с примерами).
- Советы: каналы для нескольких информеров, `\n` для многострочности, перезагрузка конфига через `amxx reload`.
- Отладка: логи `[INFO]`/WARNING, `params_controller_types` srvcmd, типичные ошибки (ParamsController не загружен, дубликат Key).

## 7. Критерии готовности (verification)

1. `amxb build` (или `amxx-dep-resolver_compile_sma`) — компиляция без ошибок и предупреждений (CI трактует ворнинги как ошибки? — минимум: чистая компиляция).
2. `amxx-dep-resolver_validate_manifest` — валидный amxbuild.yml.
3. `amxx-dep-resolver_resolve_include` — все `#include <...>` резолвятся (stdlib + деп ParamsController).
4. Ручная проверка логики: инициализация (обе ветки), типы, плейсхолдеры, задача, plugin_end.
5. README покрывает конфиг и плейсхолдеры.

## 8. Риски и ограничения

- `deaths` не добавлен (в AMXX 1.10 `get_user_frags` без deaths).
- CS-специфичные плейсхолдеры (money и т.п.) не добавляются — плагин остаётся игро-независимым.
- ParamsController 1.5.0-alpha.1 — альфа; `PARAMS_CONTROLLER_VERSION` в include = "1.4.3" (не бампнут) — не влияет на использование.
- Если другой плагин перезапишет HUD-канал — визуальное пересечение (документируется в README).
