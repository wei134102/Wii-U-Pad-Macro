/*
 * UI strings (English / Chinese).
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace i18n {

    enum class Lang : int {
        EN = 0,
        ZH = 1,
    };

    enum class Key {
        CatTitle,
        OptPluginEnabled,
        OptLanguage,
        LangEnglish,
        LangChinese,
        On,
        Off,
        OptDelay,
        OptSlotCount,
        OptMacroToRun,
        MacroEditCat,
        MacroTree,
        MacroOsTitle,
        MacroOsHelp,
        MacroOsField,
        MacroOsNextField,
        MacroClear,
        MacroUndo,
        EditGap,
        EditHold,
        EditKeys,
        EditLStick,
        EditRStick,
        Empty,
        HelpFormat,
        HelpFormatBody,
        HelpEdit,
        HelpEditBody,
        HelpAfterClose,
        HelpAfterCloseBody,
        HelpSdPath,
        HelpSdPathBody,
        ExportSd,
        ImportSd,
        Exporting,
        Importing,
        ExportOk,
        ImportOk,
        ExportFail,
        ImportFail,
        MacroPressA,
        MacroGraphUnfocused,
        BtnBack,
        Count,
    };

    void
    set_lang(Lang lang)
        noexcept;

    Lang
    current_lang()
        noexcept;

    void
    set_lang_from_int(int v)
        noexcept;

    const char *
    tr(Key key)
        noexcept;

    const char *
    language_name(Lang lang)
        noexcept;

} // namespace i18n
