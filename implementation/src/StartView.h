#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/Image.h>
#include <gui/DrawableString.h>
#include <td/Types.h>
#include <functional>
#include <algorithm>
#include "Theme.h"

extern "C" int getThemeIndex();

#ifndef CONNECTXO_UI_FONT
#if defined(__APPLE__)
#define CONNECTXO_UI_FONT "Helvetica Neue"
#else
#define CONNECTXO_UI_FONT "Segoe UI"
#endif
#endif

enum class GameType
{
    TicTacToe,
    Connect4
};

// Start screen view with two clickable regions for game selection.
class StartView : public gui::Canvas
{
    std::function<void(GameType)> _onSelect;
    gui::Rect _ticTacToeRect;
    gui::Rect _connect4Rect;
    bool _layoutReady = false;
    bool _ticHovered = false;
    bool _connectHovered = false;

    gui::Image _ticImage;
    gui::Image _connectImage;

    // Track last loaded theme so we only reload resources when theme changes.
    ThemeIndex _lastTheme;
    bool _imagesLoaded = false;

    void layoutRects(const gui::Size& newSize)
    {
        const td::Coord gap = newSize.width * 0.10f;   // 10% horizontal gap
        const td::Coord rectW = newSize.width * 0.35f; // each card 35% of width
        const td::Coord rectH = newSize.height * 0.45f;
        const td::Coord centerY = newSize.height * 0.55f;  // Moved up from 0.68f to reduce gap

        const td::Coord leftX = (newSize.width * 0.5f) - rectW - gap * 0.5f;
        const td::Coord rightX = (newSize.width * 0.5f) + gap * 0.5f;
        const td::Coord top = centerY - rectH * 0.5f;
        const td::Coord bottom = centerY + rectH * 0.5f;

        _ticTacToeRect = gui::Rect(leftX, top, leftX + rectW, bottom);
        _connect4Rect = gui::Rect(rightX, top, rightX + rectW, bottom);
        _layoutReady = true;
    }

    void ensureLayout()
    {
        if (!_layoutReady || (_ticTacToeRect.width() <= 0) || (_connect4Rect.width() <= 0))
        {
            gui::Size sz;
            getSize(sz);          // match signature: void getSize(Size& sz) const;
            layoutRects(sz);
        }
    }

protected:
    void onResize(const gui::Size& newSize) override
    {
        layoutRects(newSize);
    }

    void onCursorMoved(const gui::InputDevice& inputDevice) override
    {
        ensureLayout();
        const gui::Point& pt = inputDevice.getModelPoint();
        auto contains = [&pt](const gui::Rect& r) {
            return (pt.x >= r.left && pt.x <= r.right && pt.y >= r.top && pt.y <= r.bottom);
        };

        bool wasTicHovered = _ticHovered;
        bool wasConnectHovered = _connectHovered;
        _ticHovered = contains(_ticTacToeRect);
        _connectHovered = contains(_connect4Rect);

        if (_ticHovered != wasTicHovered || _connectHovered != wasConnectHovered)
            reDraw();
    }

    void onDraw(const gui::Rect& rect) override
    {
        // pick theme
        ThemeIndex themeIdx = clampThemeIndex(getThemeIndex());

        td::ColorID accentColor = td::ColorID::Blue; // THIS COLOR CHANGES WITH THEMES
        td::ColorID secondaryColor = td::ColorID::Black; // THIS COLOR CHANGES WITH THEMES
        td::ColorID bgColor = td::ColorID::White; // THIS COLOR CHANGES WITH THEMES

        td::String tictactoeLabel, connect4Label;

        switch (themeIdx)
        {
            case ThemeIndex::Classic:
                bgColor = td::ColorID::GhostWhite;  // Very light off-white instead of pure White
                accentColor = td::ColorID::Blue;
                secondaryColor = td::ColorID::Black;
                tictactoeLabel = ":tictactoeIcon";
                connect4Label = ":connect4Icon";
                break;
            case ThemeIndex::Nature:
                bgColor = td::ColorID::Linen;  // Warm off-white instead of Snow
                accentColor = td::ColorID::ForestGreen;
                secondaryColor = td::ColorID::Rust;
                tictactoeLabel = ":tictactoeIconWood";
                connect4Label = ":connect4IconWood";
                break;
            case ThemeIndex::Strawberry:
                bgColor = td::ColorID::FloralWhite;
                accentColor = td::ColorID::Crimson;
                secondaryColor = td::ColorID::MediumSpringGreen;
                tictactoeLabel = ":tictactoeIconStrawberry";
                connect4Label = ":connect4IconStrawberry";
                break;
            case ThemeIndex::Beachy:
                bgColor = td::ColorID::SeaShell;
                accentColor = td::ColorID::CadetBlue;
                secondaryColor = td::ColorID::DarkKhaki;
                tictactoeLabel = ":tictactoeIconBeachy";
                connect4Label = ":connect4IconBeachy";
                break;
            case ThemeIndex::Dark:
                bgColor = td::ColorID::Black;
                accentColor = td::ColorID::DarkOrange;
                secondaryColor = td::ColorID::SteelBlue;
                tictactoeLabel = ":tictactoeIconDark";
                connect4Label = ":connect4IconDark";
                break;
            default:
                bgColor = td::ColorID::GhostWhite;
                accentColor = td::ColorID::Blue;
                secondaryColor = td::ColorID::Black;
                tictactoeLabel = ":tictactoeIcon";
                connect4Label = ":connect4Icon";
                break;
        }

        // If theme changed (or images not yet loaded), load correct images for this theme.
        if (!_imagesLoaded || themeIdx != _lastTheme)
        {
            _ticImage.load(tictactoeLabel.c_str());
            _connectImage.load(connect4Label.c_str());
            _lastTheme = themeIdx;
            _imagesLoaded = true;
        }
        td::ColorID rectColor = secondaryColor; // THIS COLOR CHANGES WITH THEMES
        td::ColorID textLblColor = accentColor; // THIS COLOR CHANGES WITH THEMES
        td::ColorID titleColor = accentColor; // THIS COLOR CHANGES WITH THEMES
        td::ColorID rectBorderColor = rectColor; // THIS COLOR CHANGES WITH THEMES
        td::ColorID rectBorderHoverColor = accentColor;
        td::ColorID cardFillColor = bgColor;


        // Fill background with gradient effect for visual interest
        gui::Shape::drawRect(rect, bgColor);

        ensureLayout();

        // Draw centered localized title above the two option cards.
        td::String title = tr("chooseGame");
        std::string titleStr = title.c_str();
        std::string spacedTitle;
        spacedTitle.reserve(titleStr.size() * 2);
        for (size_t i = 0; i < titleStr.size(); ++i)
        {
            spacedTitle.push_back(titleStr[i]);
            if (i + 1 < titleStr.size())
                spacedTitle.push_back(' ');
        }
        gui::Font titleFont;
        float fontSizePt = 40.0f;
        titleFont.create(CONNECTXO_UI_FONT, fontSizePt, gui::Font::Style::Bold, gui::Font::Unit::Point);
        // Place the text in the top area, centered horizontally across the whole window and vertically
        // between the top of the window and the top of the selectable cards.
        const double topMargin = rect.height() * 0.08;
        const double bottomMargin = rect.height() * 0.04;  // Reduced from 0.10 to close the gap
        gui::Rect titleRect(rect.left, rect.top + topMargin, rect.right, _ticTacToeRect.top - bottomMargin);
        gui::DrawableString::draw(title, titleRect, &titleFont, titleColor, td::TextAlignment::Center, td::VAlignment::Center);


        // Draw card outlines only (no fill) using rounded rects

        // Draw images inside rectangles (with padding). Images come from resources:
        // resource labels: "tictactoeIcon" and "connect4Icon".
        const double padFraction = 0.10; // 10% padding inside each card

        // Label font for the captions inside each card (Segoe UI,20pt)
        gui::Font labelFont;
        const float labelFontSize = 20.0f;
        labelFont.create(CONNECTXO_UI_FONT, labelFontSize, gui::Font::Style::Bold, gui::Font::Unit::Point);

        auto drawImageInRect = [&](const gui::Image& img, const gui::Rect& cardRect, const td::String& caption, double scale)
            {
                const double cardH = cardRect.height();
                const double pw = cardRect.width() * padFraction;
                const double ph = cardH * padFraction;

                // Reserve a small area at the bottom of the card for the caption
                const double labelHeight = cardH * 0.14; // ~14% of card height

                // Image area is the card minus padding and label area
                const double imgTop = cardRect.top + ph;
                double imgBottom = cardRect.bottom - ph - labelHeight;
                if (imgBottom < imgTop)
                    imgBottom = imgTop; // guard

                gui::Rect inner(cardRect.left + pw, imgTop, cardRect.right - pw, imgBottom);
                if (scale > 1.0)
                {
                    const double cx = (inner.left + inner.right) * 0.5;
                    const double cy = (inner.top + inner.bottom) * 0.5;
                    const double halfW = inner.width() * 0.5 * scale;
                    const double halfH = inner.height() * 0.5 * scale;
                    inner = gui::Rect(cx - halfW, cy - halfH, cx + halfW, cy + halfH);
                }

                if (img.isOK())
                {
                    img.draw(inner, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center, nullptr);
                }

                // Compute label rectangle centered vertically between the bottom of the image area (imgBottom)
                // and the bottom padded edge of the card (cardRect.bottom - ph).
                const double labelAreaTop = imgBottom;
                const double labelAreaBottom = cardRect.bottom - ph;
                const double midY = (labelAreaTop + labelAreaBottom) * 0.5;

                const double lblH = labelHeight; // desired height for the label rect
                double lblTop = midY - lblH * 0.5;
                double lblBottom = midY + lblH * 0.5;

                // Clamp into the available label area
                if (lblTop < labelAreaTop)
                {
                    lblTop = labelAreaTop;
                    lblBottom = lblTop + lblH;
                }
                if (lblBottom > labelAreaBottom)
                {
                    lblBottom = labelAreaBottom;
                    lblTop = lblBottom - lblH;
                }

                gui::Rect labelRect(cardRect.left + pw, lblTop, cardRect.right - pw, lblBottom);
                gui::DrawableString::draw(caption, labelRect, &labelFont, textLblColor, td::TextAlignment::Center, td::VAlignment::Center);
            };

        drawImageInRect(_ticImage, _ticTacToeRect, tr("ticTacToe"), _ticHovered ? 1.05 : 1.0);
        drawImageInRect(_connectImage, _connect4Rect, tr("connectFour"), _connectHovered ? 1.05 : 1.0);

        // Draw card borders (outline-only)
        {
            gui::Shape sh;
            td::Coord radius = 20;
            sh.createRoundedRect(_ticTacToeRect, radius, 1.0f);
            sh.drawWire(_ticHovered ? rectBorderHoverColor : rectBorderColor, 3.0f);
        }
        {
            gui::Shape sh;
            td::Coord radius = 20;
            sh.createRoundedRect(_connect4Rect, radius, 1.0f);
            sh.drawWire(_connectHovered ? rectBorderHoverColor : rectBorderColor, 3.0f);
        }
    }

    void onPrimaryButtonReleased(const gui::InputDevice& inputDevice) override
    {
        ensureLayout();
        const gui::Point& pt = inputDevice.getModelPoint();

        auto contains = [&pt](const gui::Rect& r) {
            return (pt.x >= r.left && pt.x <= r.right && pt.y >= r.top && pt.y <= r.bottom);
            };

        if (contains(_ticTacToeRect))
        {
            if (_onSelect)
            {
                auto cb = _onSelect; // copy to avoid capturing a soon-to-be-destroyed view
                auto* fn = new gui::AsyncFn([cb]() {
                    cb(GameType::TicTacToe);
                    });
                gui::NatObject::asyncCall(fn, true);
            }
            return;
        }

        if (contains(_connect4Rect))
        {
            if (_onSelect)
            {
                auto cb = _onSelect; // copy to avoid capturing a soon-to-be-destroyed view
                auto* fn = new gui::AsyncFn([cb]() {
                    cb(GameType::Connect4);
                    });
                gui::NatObject::asyncCall(fn, true);
            }
            return;
        }
    }

public:
    StartView()
        : Canvas({ gui::InputDevice::Event::PrimaryClicks, gui::InputDevice::Event::CursorMove })
        , _layoutReady(false)
        , _ticImage()
        , _connectImage()
        , _lastTheme(ThemeIndex::Classic)
        , _imagesLoaded(false)
    {
        enableResizeEvent(true);

        // Do not hard-code Strawberry images in the ctor.
        // Images will be loaded on first draw (or whenever theme changes).
    }

    void setOnSelect(const std::function<void(GameType)>& cb)
    {
        _onSelect = cb;
    }
};