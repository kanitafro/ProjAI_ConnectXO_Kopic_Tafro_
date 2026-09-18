// minimal changes needed

#pragma once
#include <gui/ToolBar.h>
#include <gui/Image.h>
#include <gui/Symbol.h>

class ToolBar : public gui::ToolBar
{
    gui::Image _settings;
public:
    ToolBar()
    : gui::ToolBar("mainTB", 1)
    , _settings(":settings")
    {
        addItem(tr("settings"), &_settings, tr("settingsTT"), 10, 0, 0, 10);
    }
};

