// minimal changes needed
// start/stop won't be used

#pragma once
#include <gui/MenuBar.h>
#include <td/Types.h>

class MenuBar : public gui::MenuBar
{
public:
    static constexpr td::UINT4 MenuAppID = 10;

private:
    gui::SubMenu _subFirst;
protected:
    void populateFirstMenu()
    {
        auto& items = _subFirst.getItems();
        items[0].initAsActionItem(tr("settings"), 10); //prevedeno u natGUI
        items[1].initAsSeparator();
        items[2].initAsQuitAppActionItem(tr("Quit"), "q"); //prevedeno u natGUI
    }
    
    
public:
    MenuBar()
    : gui::MenuBar(1) //one menu
    , _subFirst(MenuAppID, tr("App"), 3) //allocate items for the Application subMenu
    {
        populateFirstMenu();
        _menus[0] = &_subFirst;
    }
};
