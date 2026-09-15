#if defined _extended_hud_informer_included
    #endinput
#endif
#define _extended_hud_informer_included

#include <ParamsController>

/**
 * Версия плагина ExtendedHudInformer.
 *
 * Используется в качестве значения PluginVersion при регистрации плагина.
 */
#define EXHI_VERSION "0.0.2"

/**
 * Название библиотеки, регистрируемое через register_library().
 *
 * Используется для проверки загруженности плагина через LibraryExists():
 *     if (!LibraryExists(EXHI_LIBRARY, LibType_Library)) {
 *         // плагин ExtendedHudInformer не загружен
 *     }
 */
#define EXHI_LIBRARY "ExtendedHudInformer"

/**
 * Максимальная длина ключа информера (поле "Key" в конфиге).
 *
 * Используется при чтении конфига и в качестве размера буфера
 * в нативе ExtendedHudInformer_Find().
 */
#define EXHI_INFORMER_KEY_MAX_LEN 64

/**
 * Максимальная длина текста HUD-сообщения, формируемого из шаблона информера.
 *
 * Ограничение совпадает с PARAM_VALUE_MAX_LEN (512) — лимитом текстовых
 * сегментов скомпилированного шаблона ParamsController.
 */
#define EXHI_HUD_TEXT_MAX_LEN 512

/**
 * Максимальная длина текста dHUD-сообщения (show_dhudmessage).
 *
 * Ограничение движка: show_dhudmessage автоматически обрезает
 * текст длиннее этого лимита.
 */
#define EXHI_DHUD_TEXT_MAX_LEN 128

/**
 * Относительный путь к папке конфигов плагина.
 *
 * Результирующий путь на сервере: <amxx_configsdir>/plugins/ExtendedHudInformer.
 * Папка рекурсивно обходится при загрузке, обрабатываются все файлы *.json
 * (файлы и папки, начинающиеся с '!' или '.', пропускаются).
 */
new const EXHI_CONFIG_PATH[] = "ExtendedHudInformer";

/**
 * Название типа параметра ParamsController, читающего объект информера.
 *
 * Тип регистрируется плагином в форварде ParamsController_OnRegisterTypes.
 * Позволяет другим плагинам читать объекты информеров через PCSingle_* / PCGet_*,
 * например:
 *     new Trie:p = PCSingle_Read(json, EXHI_INFORMER_PARAM_TYPE);
 */
new const EXHI_INFORMER_PARAM_TYPE[] = "ExtendedHudInformer-Informer";

/**
 * Хендлер информера.
 *
 * Возвращается нативами ExtendedHudInformer_Find() / ExtendedHudInformer_Init()
 * и используется в качестве аргумента остальных нативов API.
 *
 * Значение -1 (Invalid_ExtendedHudInformer) означает, что информер не найден
 * или переданный хендлер некорректен.
 */
enum T_ExtendedHudInformer { Invalid_ExtendedHudInformer = -1 }

/**
 * Принудительная инициализация плагина ExtendedHudInformer.
 *
 * Вызывает ParamsController_Init(), регистрирует тип параметра информера
 * и загружает конфиги из папки EXHI_CONFIG_PATH.
 *
 * @note    Функция идемпотентна: повторные вызовы (в том числе до фактической
 *          инициализации ParamsController) безопасны.
 * @note    Обычно вызывать её не требуется — плагин инициализируется сам
 *          в plugin_precache. Натив нужен другим плагинам, которые хотят
 *          гарантировать инициализацию до обращения к API.
 *
 * @noreturn
 */
native ExtendedHudInformer_Init();

/**
 * Поиск информера по ключу.
 *
 * @param key   Ключ информера (значение поля "Key" из конфига).
 *
 * @return      Хендлер найденного информера, либо Invalid_ExtendedHudInformer,
 *              если информер с указанным ключом не существует.
 */
native T_ExtendedHudInformer:ExtendedHudInformer_Find(const key[]);

/**
 * Проверка, включён ли информер.
 *
 * Включённый информер имеет запущенную повторяющуюся задачу обновления HUD.
 *
 * @param informer  Хендлер информера (из ExtendedHudInformer_Find()).
 *
 * @return          true, если информер существует и включён, иначе false.
 *                  Для невалидного хендлера всегда возвращает false.
 */
native bool:ExtendedHudInformer_IsEnabled(const T_ExtendedHudInformer:informer);

/**
 * Включение/выключение информера на лету.
 *
 * При включении запускается повторяющаяся задача обновления и информер
 * немедленно показывается. При выключении задача останавливается (уже
 * показанное сообщение исчезнет само по истечении HoldTime).
 *
 * @note    Повторный вызов с тем же состоянием — безопасная операция (no-op).
 *
 * @param informer  Хендлер информера (из ExtendedHudInformer_Find()).
 * @param enabled   true — включить информер, false — выключить.
 *
 * @noreturn
 */
native ExtendedHudInformer_SetEnabled(const T_ExtendedHudInformer:informer, const bool:enabled);

/**
 * Принудительный показ информера.
 *
 * Форматирует текст информера (с подстановкой текущих значений плейсхолдеров)
 * и выводит его в HUD. Работает независимо от состояния информера
 * (в том числе для выключенных) и не влияет на повторяющуюся задачу.
 *
 * @param informer      Хендлер информера (из ExtendedHudInformer_Find()).
 * @param playerIndex   Индекс игрока, которому показать информер.
 *                      По умолчанию 0 — показать всем подключённым игрокам.
 *
 * @noreturn
 * @error               Невалидный хендлер информера игнорируется без ошибки.
 */
native ExtendedHudInformer_Show(const T_ExtendedHudInformer:informer, const playerIndex = 0);
