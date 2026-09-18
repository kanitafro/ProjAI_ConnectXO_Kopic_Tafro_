#pragma once
#include <gui/Window.h>
#include <gui/Application.h>
#include "MenuBar.h"
#include "ToolBar.h"
#include "StartView.h"     //This is the start view
#include "TicTacToeView.h"
#include "Connect4View.h"
#include "ViewSettings.h"
#include "ScoreManager.h"
#include <functional>
#include <gui/Panel.h>
#include <memory>

class MainWindow : public gui::Window
{
    template <class ViewT>
    class GameWindow : public gui::Window
    {
        std::unique_ptr<ViewT> _view;
    public:
        GameWindow()
            : gui::Window(gui::Size(900, 720))
            , _view(std::make_unique<ViewT>())
        {
            setCentralView(_view.get());
            open(); // show immediately
        }
    };
protected:
    MenuBar _mainMenuBar;
    ToolBar _toolBar;
    StartView* _pStartView{ nullptr };

    static constexpr td::UINT4 DlgSettingsID =200;

protected:
    void onInitialAppearance() override //will be called only once
    {
        if (_pStartView)
            _pStartView->setFocus();
    }

    bool shouldClose() override
    {
        return true;
    }

    bool onActionItem(gui::ActionItemDescriptor& aiDesc) override
    {
        auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();
        (void)firstSubMenuID;
        (void)lastSubMenuID;

        switch (menuID)
        {
        case MenuBar::MenuAppID:
            // Settings: show settings dialog
            if (actionID ==10)
            {
                // define dialog buttons: Restart (User0) and Don't restart (User1)
                std::initializer_list<gui::Dialog::ButtonDesc> buttons = {
                    { gui::Dialog::Button::ID::User0, tr("Restart"), gui::Button::Type::Default },
                    { gui::Dialog::Button::ID::User1, tr("DoNoRestart"), gui::Button::Type::Normal }
                };


                // Increase dialog size by 10% in both dimensions so _lblRestartInfo fits.
                // Base size preserved (420x160), compute a 10% larger integer size.
                int baseW = 440;
                int baseH = 180;
                int dlgW = (baseW * 11) / 10; // 10% wider
                int dlgH = (baseH * 11) / 10; // 10% taller

                gui::Panel::show<ViewSettings>(this, tr("dlgSettings"), gui::Size(baseW, baseH), DlgSettingsID, buttons,
                    [](gui::Dialog::Button::ID clickedBtn, ViewSettings* view)
                    {
                        if (!view)
                            return;

                        td::String newExt = view->getTranslationExt();
                        auto* app = gui::getApplication();

                        // If language was changed, handle restart or try to apply without restart
                        if (view->isRestartRequired())
                        {
                            if (clickedBtn == gui::Dialog::Button::ID::User0)
                            {
                                // Restart the application so translation takes full effect
                                if (app)
                                    app->restart();
                            }
                            else if (clickedBtn == gui::Dialog::Button::ID::User1)
                            {
                                // Try to apply language without forcing restart
                                if (app && newExt.length() > 0)
                                    app->setLanguage(newExt);
                            }
                        }
                    });

                return true;
            }
            return false;
        default:
            break;
        }
        return false;
    }

    void handleGameSelected(GameType game)
    {
        auto switchView = [this, game]()
            {
                switch (game)
                {
                case GameType::TicTacToe:
                {
                    // Open a separate window for TicTacToe to avoid swapping central views.
                    new GameWindow<TicTacToeView>();
                    break;
                }
                case GameType::Connect4:
                {
                    // Open a separate window for Connect4 to avoid swapping central views.
                    new GameWindow<Connect4View>();
                    break;
                }
                default:
                    break;
                }
            };

        // Already called from StartView via asyncCall on main thread.
        // Create the game window directly to avoid nested async scheduling and races.
        switchView();
    }

public:
    MainWindow()
        : gui::Window(gui::Size(900, 720))
    {
        setTitle(tr("appTitle"));
        _mainMenuBar.setAsMain(this);
        setToolBar(_toolBar);
        _pStartView = new StartView();
        setCentralView(_pStartView);

        _pStartView->setOnSelect(std::bind(&MainWindow::handleGameSelected, this, std::placeholders::_1));
    }
};
