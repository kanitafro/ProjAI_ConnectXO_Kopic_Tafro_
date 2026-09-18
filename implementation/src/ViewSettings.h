#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/ComboBox.h>
#include <gui/LineEdit.h>
#include <gui/Slider.h>
#include <gui/GridLayout.h>
#include <gui/GridComposer.h>
#include <gui/Button.h>
#include <td/Types.h>
#include <gui/Lang.h>
#include <vector>
#include "Theme.h"
#include "ScoreManager.h"

// Functions provided by AIPlayer module to change/get difficulty at runtime
extern "C" void setAIDifficultyIndex(int idx);
extern "C" int getAIDifficultyIndex();

class ViewSettings : public gui::View
{
protected:
    gui::Label _lblLangNow;
    gui::LineEdit _leLang;
    gui::Label _lblLangNew;
    gui::ComboBox _cmbLangs;
    gui::Label _lblAIDifficulty;
    gui::Slider _sliderAIDifficulty;
    gui::Label _lblAIDifficultyValue; // Shows current difficulty text (kept for compatibility)
    gui::Label _lblHintDisplay;
    gui::ComboBox _cmbHint;
    gui::Label _lblTheme;
    gui::ComboBox _cmbThemes;
    gui::Label _lblResetScores;
    gui::Button _btnResetTTT;
    gui::Button _btnResetC4;
    gui::GridLayout _gl;
    gui::Label _lblRestartInfo;
    int _initialLangSelection;
    td::String _initialLangExtension; // store extension to compare reliably
    std::vector<gui::Lang> _langs;
    int _initialAIDifficulty;
    int _initialThemeSelection;
    td::String _baseRestartText; // Store the original restart text
    bool _suppressChangeEvents = false;
    td::Accent _restartWarningAccent = td::Accent::Warning;
    td::Accent _restartNormalAccent = td::Accent::Plain;

    // Map slider values to difficulty names and indices
    td::String getDifficultyName(int index) const
    {
        switch (index)
        {
        case 0: return tr("VeryEasy");
        case 1: return tr("Easy");
        case 2: return tr("Medium");
        case 3: return tr("Hard");
        case 4: return tr("VeryHard");
        default: return tr("Medium");
        }
    }

    int sliderValueToIndex(double sliderValue) const
    {
        // Convert slider value (0.0-1.0) to difficulty index (0-4)
        int index = static_cast<int>(std::round(sliderValue * 4.0));
        if (index < 0) index = 0;
        if (index > 4) index = 4;
        return index;
    }

    double indexToSliderValue(int index) const
    {
        // Convert difficulty index (0-4) to slider value (0.0-1.0)
        if (index < 0) index = 0;
        if (index > 4) index = 4;
        return static_cast<double>(index) / 4.0;
    }

    void updateDifficultyLabel()
    {
        // Compose the label so the difficulty text appears inline with the label:
        // e.g. "Difficulty mode: (Medium)"
        double sliderVal = _sliderAIDifficulty.getValue();
        int difficultyIndex = sliderValueToIndex(sliderVal);
        td::String difficultyText = getDifficultyName(difficultyIndex);

        td::String base = tr("lblAIDifficulty");
        td::String composed = base;
        composed += " (";
        composed += difficultyText;
        composed += ")";

        _lblAIDifficulty.setTitle(composed);

        // Keep the separate value label updated too (if other code expects it)
        _lblAIDifficultyValue.setTitle(difficultyText);
    }

    void updateRestartInfoDisplay()
    {
        if (isRestartRequired())
        {
            td::String warningText;
            warningText = _baseRestartText;
            warningText += " !!";
            _lblRestartInfo.setTitle(warningText);

            // Use Accent (required by Label API)
            _lblRestartInfo.setTextColor(_restartWarningAccent);
        }
        else
        {
            _lblRestartInfo.setTitle(_baseRestartText);
            _lblRestartInfo.setTextColor(_restartNormalAccent);
            _lblRestartInfo.setTitle(_baseRestartText);
        }

        // Ensure UI updates immediately
        reDraw();
    }

public:
    ViewSettings()
        : _lblLangNow(tr("lblLang"))
        , _lblLangNew(tr("lblLang2"))
        , _lblAIDifficulty(tr("lblAIDifficulty"))
        , _lblAIDifficultyValue("")
        , _lblHintDisplay(tr("lblHintDisplay"))
        , _lblTheme(tr("themesList"))
        , _lblResetScores(tr("resetScoresLbl"))
        , _btnResetTTT(tr("resetTTTLbl"))
        , _btnResetC4(tr("resetC4Lbl"))
        , _gl(9, 2) // Increased rows to accommodate reset buttons
        , _lblRestartInfo(tr("RestartRequiredInfo"))
        , _baseRestartText(tr("RestartRequiredInfo"))
    {
        gui::Application* pApp = getApplication();
        auto appProperties = pApp->getProperties();
        assert(appProperties);
        td::String strTr1 = appProperties->getValue("translation", "BA");
        td::String strTr2 = appProperties->getValue("translation", "ES");

        _leLang.setAsReadOnly();
        int newLangIndex = -1;
        // populate local langs from global supported languages
        auto& gl = getSupportedLanguages();
        _langs.clear();
        _langs.reserve(gl.size() + 2);
        for (const auto& L : gl)
            _langs.push_back(L);

        // ensure both strTr1 and strTr2 exist as entries (so both appear)
        auto ensureLang = [&](const td::String& ext) {
            if (ext.length() == 0) return;
            for (const auto& L : _langs)
                if (L.getExtension() == ext) return; // already present
            gui::Lang l;
            // set description same as extension as a fallback
            l.set(ext, td::String(ext));
            _langs.push_back(l);
            };

        ensureLang(strTr1);
        ensureLang(strTr2);

        // determine current translation extension from global translation index
        auto globalIdx = getTranslationLanguageIndex();
        td::String currExt = "";
        if (globalIdx >= 0 && (size_t)globalIdx < gl.size())
            currExt = gl[globalIdx].getExtension();

        // find index of current language in local list
        int currTranslationIndex = 0;
        for (size_t idx = 0; idx < _langs.size(); ++idx)
        {
            if (_langs[idx].getExtension() == currExt)
            {
                currTranslationIndex = (int)idx;
                break;
            }
        }

        // show current language
        auto& strCurrentLanguage = _langs[currTranslationIndex].getDescription();
        _leLang.setText(strCurrentLanguage);

        // populate combo and choose selection: prefer strTr1, then strTr2, then current
        int i = 0;
        for (const auto& lang : _langs)
        {
            if (lang.getExtension() == strTr1)
                newLangIndex = i; // prefer strTr1
            // if strTr1 not found, allow strTr2 to set selection
            else if ((newLangIndex == -1) && (lang.getExtension() == strTr2))
                newLangIndex = i;

            _cmbLangs.addItem(lang.getDescription());
            ++i;
        }

        if (newLangIndex == -1)
            newLangIndex = currTranslationIndex;

        _cmbLangs.selectIndex(newLangIndex);
        _initialLangSelection = newLangIndex;
        // store extension instead of relying on index comparisons (robust if combo items reorder)
        if (_initialLangSelection >= 0 && (size_t)_initialLangSelection < _langs.size())
            _initialLangExtension = _langs[_initialLangSelection].getExtension();
        else
            _initialLangExtension = td::String("");

        // AI difficulty: set up slider with 5 discrete positions (0.0, 0.25, 0.5, 0.75, 1.0)
        _sliderAIDifficulty.setRange(0.0, 1.0); // Range from 0.0 to 1.0
        int aiIdx = getAIDifficultyIndex();
        if (aiIdx < 0) aiIdx = 0;
        if (aiIdx > 4) aiIdx = 4;
        _sliderAIDifficulty.setValue(indexToSliderValue(aiIdx), false); // Don't send message initially
        _initialAIDifficulty = aiIdx;
        updateDifficultyLabel(); // Set initial label text (now composes into the main label)

        // Hint display combo: On/Off
        _cmbHint.addItem(tr("hintOn"));
        _cmbHint.addItem(tr("hintOff"));
        int hintSetting = appProperties->getValue("showHint", 1);
        _cmbHint.selectIndex(hintSetting ? 0 : 1);

        // Theme combo: Classic, Nature, Strawberry, Beachy, Dark
        _cmbThemes.addItem(tr("themeClassic"));
        _cmbThemes.addItem(tr("themeNature"));
        _cmbThemes.addItem(tr("themeStrawberry"));
        _cmbThemes.addItem(tr("themeBeachy"));
        _cmbThemes.addItem(tr("themeDark"));

        // Initialize theme selection from persistent app properties (apply on next restart).
        int themeIdx = appProperties->getValue("theme", to_int(ThemeIndex::Classic)); // default to Classic
        if (themeIdx < to_int(ThemeIndex::Classic)) themeIdx = to_int(ThemeIndex::Classic);
        if (themeIdx > to_int(ThemeIndex::Dark)) themeIdx = to_int(ThemeIndex::Dark);
        _cmbThemes.selectIndex(themeIdx);
        _initialThemeSelection = themeIdx;

        // event handlers
        _cmbLangs.onChangedSelection([this, appProperties]() {
            if (_suppressChangeEvents) return;

            // Update UI immediately
            updateRestartInfoDisplay();

            // Persist only if different from stored value (avoid unnecessary side-effects)
            td::String strTr = this->getTranslationExt();
            if (strTr.length() > 0 && appProperties)
            {
                td::String cur = appProperties->getValue("translation", "");
                if (cur != strTr)
                {
                    // guard against reentrant notifications caused by setValue
                    _suppressChangeEvents = true;
                    appProperties->setValue("translation", strTr);
                    _suppressChangeEvents = false;
                }
                else _suppressChangeEvents = false;
            }
            });

        _sliderAIDifficulty.onChangedValue([this]() {
            double sliderVal = this->_sliderAIDifficulty.getValue();
            int difficultyIndex = sliderValueToIndex(sliderVal);

            // Snap to discrete positions (0.0, 0.25, 0.5, 0.75, 1.0)
            double snappedValue = indexToSliderValue(difficultyIndex);
            if (std::abs(sliderVal - snappedValue) > 0.01) // Only update if significantly different
            {
                this->_sliderAIDifficulty.setValue(snappedValue, false); // Don't trigger another event
            }

            setAIDifficultyIndex(difficultyIndex); // change takes effect at runtime; no restart required
            updateDifficultyLabel();
            });

        _cmbHint.onChangedSelection([this, appProperties]() {
            int sel = this->_cmbHint.getSelectedIndex();
            if (sel < 0) sel = 0;
            if (sel > 1) sel = 1;
            if (appProperties)
            {
                appProperties->setValue("showHint", sel == 0 ? 1 : 0);
            }
            });

        // Persist theme choice to app properties — apply after restart
        _cmbThemes.onChangedSelection([this, appProperties]() {
            if (_suppressChangeEvents) return;

            // Update UI immediately
            updateRestartInfoDisplay();

            // Persist only if different from stored value (avoid unnecessary side-effects)
            int sel = this->_cmbThemes.getSelectedIndex();
            if (sel < 0) sel = 0;
            if (sel > 4) sel = 4;

            if (appProperties)
            {
                int cur = appProperties->getValue("theme", to_int(ThemeIndex::Classic));
                if (cur != sel)
                {
                    _suppressChangeEvents = true;
                    appProperties->setValue("theme", sel);
                    _suppressChangeEvents = false;
                }
            }
            });

        auto maxCmbWidth = _cmbLangs.getWidthToFitLongestItem();
        _cmbLangs.setSizeLimits((td::UINT2)maxCmbWidth, gui::Control::Limit::UseAsMin);

        // populate grid using correct GridComposer syntax
        gui::GridComposer gc(_gl);
        gc.appendRow(_lblLangNow) << _leLang;
        gc.appendRow(_lblLangNew) << _cmbLangs;

        // Place the difficulty text inline with the label so it appears before the slider:
        // "Difficulty mode: (Medium)    <SLIDER>"
        gc.appendRow(_lblAIDifficulty) << _sliderAIDifficulty;

        // Note: _lblAIDifficultyValue is still kept updated above for compatibility,
        // but it is not added as a separate row anymore.
        gc.appendRow(_lblHintDisplay) << _cmbHint;
        gc.appendRow(_lblTheme) << _cmbThemes;
        gc.appendRow(_lblResetScores, 2); // Span for section header
        gc.appendRow(_btnResetTTT) << _btnResetC4;
        gc.appendRow(_lblRestartInfo, 2); // Span across both columns for the restart info

        setLayout(&_gl);
    }

    td::String getTranslationExt()
    {
        td::String strExt;
        int currSelection = _cmbLangs.getSelectedIndex();
        if (currSelection >= 0)
        {
            if ((size_t)currSelection < _langs.size())
                strExt = _langs[currSelection].getExtension();
        }

        return strExt;
    }

    bool onClick(gui::Button* pBtn) override
    {
        if (pBtn == &_btnResetTTT)
        {
            ScoreManager::getInstance().resetTTTStats();
            return true;
        }
        if (pBtn == &_btnResetC4)
        {
            ScoreManager::getInstance().resetC4Stats();
            return true;
        }
        return false;
    }

    bool isRestartRequired() const
    {
        // Compare stored initial extension with current selection's extension (robust
        // if the combo is repopulated/reordered by translation changes).
        td::String currExt = "";
        int currSelection = _cmbLangs.getSelectedIndex();
        if (currSelection >= 0 && (size_t)currSelection < _langs.size())
            currExt = _langs[currSelection].getExtension();

        bool langChanged = (_initialLangExtension != currExt);
        bool themeChanged = (_initialThemeSelection != _cmbThemes.getSelectedIndex());
        return langChanged || themeChanged;
    }
};