#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/Image.h>
#include <gui/Sound.h>
#include <gui/DrawableString.h>
#include <gui/Application.h>
#include <gui/GridLayout.h>
#include <gui/TabView.h>
#include <gui/BaseView.h>
//#include <gui/GridLayoutHelper.h>
#include <td/Types.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <thread>
#include <chrono>
#include <cmath>
#include <cnt/StringBuilder.h>
#include <string>
#include "TicTacToe.h"
#include "AIPlayer.h"
#include "Theme.h"
#include "ScoreManager.h"


extern "C" int getThemeIndex();

#ifndef CONNECTXO_UI_FONT
#if defined(__APPLE__)
#define CONNECTXO_UI_FONT "Helvetica Neue"
#else
#define CONNECTXO_UI_FONT "Segoe UI"
#endif
#endif

// Tic-Tac-Toe view with integrated game-over display
class TicTacToeView : public gui::Canvas
{
public:
    TicTacToeView()
        : Canvas({ gui::InputDevice::Event::PrimaryClicks, gui::InputDevice::Event::CursorMove })
        , _game()
        , _aiPlayer()
        , _imgX()
        , _imgO()
        , _clickSound(":click")
        , _imgWood() // used in NATURE / LOG CABIN theme
        , _boardLeft(0)
        , _boardTop(0)
        , _cellSize(0)
        , _aiMoveScheduled(false)
        , _winCounter(0)
        , _lossCounter(0)
        , _countersUpdated(false)
        , _aiGen(0)
        , _humanPlayer(Player::X)
        , _AIrole(Player::O)
        , _lastTheme(ThemeIndex::Classic)
        , _imagesLoaded(false)
        , _hoverRow(-1)
        , _hoverCol(-1)
        , _isHintActive(false)
        , _hintCell(-1)
        , _hintStartTime(std::chrono::steady_clock::now())
        , _hoverFadeTime(0.0f)
        , _winPulseStartTime(std::chrono::steady_clock::now())
        , _quitBtnHovered(false)
        , _replayBtnHovered(false)
        , _hintBtnHovered(false)
    {
        enableResizeEvent(true);
        _game.reset(Player::X);

        // Load persisted scores from ScoreManager
        _winCounter = ScoreManager::getInstance().getTTTStats().wins;
        _lossCounter = ScoreManager::getInstance().getTTTStats().losses;
    }

    // Allow parent to handle closing/removing this view
    void onQuit(const std::function<void()>& fn) { _onQuit = fn; }
    void onReplay(const std::function<void()>& fn) { _onReplay = fn; }

protected:
    void onResize(const gui::Size& newSize) override
    {
        // Recalculate board layout when window resizes
        reDraw();
    }

    void onCursorMoved(const gui::InputDevice& inputDevice) override
    {
        if (_cellSize <= 0) return;

        const gui::Point& pt = inputDevice.getModelPoint();

        // Helper to test point in rect
        auto pointInRect = [](const gui::Rect& r, const gui::Point& p) {
            return (p.x >= r.left) && (p.x <= r.right) && (p.y >= r.top) && (p.y <= r.bottom);
            };

        bool wasQuitHovered = _quitBtnHovered;
        bool wasReplayHovered = _replayBtnHovered;
        bool wasHintHovered = _hintBtnHovered;
        const bool hintEnabled = isHintEnabled();

        _quitBtnHovered = pointInRect(_quitBtn, pt);
        _replayBtnHovered = pointInRect(_replayBtn, pt);
        _hintBtnHovered = hintEnabled && pointInRect(_hintBtn, pt);

        // Redraw if button hover state changed
        if (_quitBtnHovered != wasQuitHovered || _replayBtnHovered != wasReplayHovered || _hintBtnHovered != wasHintHovered)
        {
            reDraw();
        }

        // Early exit if hovering over buttons
        if (_quitBtnHovered || _replayBtnHovered || _hintBtnHovered)
        {
            if (_hoverRow != -1 || _hoverCol != -1)
            {
                _hoverRow = -1;
                _hoverCol = -1;
                reDraw();
            }
            return;
        }

        int col = static_cast<int>((pt.x - _boardLeft) / _cellSize);
        int row = static_cast<int>((pt.y - _boardTop) / _cellSize);

        // Check if within board bounds and cell is empty
        if (row >= 0 && row < 3 && col >= 0 && col < 3)
        {
            int moveIndex = row * 3 + col;
            if (_game.getCell(moveIndex) == Player::None && !_game.isGameOver() &&
                _game.getCurrentPlayer() == _humanPlayer && !_aiMoveScheduled)
            {
                if (_hoverRow != row || _hoverCol != col)
                {
                    _hoverRow = row;
                    _hoverCol = col;
                    _hoverFadeTime = 0.0f; // Reset fade timer
                    reDraw();
                }
                else if (_hoverRow >= 0 && _hoverCol >= 0)
                {
                    // Update fade time while hovering
                    auto now = std::chrono::steady_clock::now();
                    static auto lastFadeTime = std::chrono::steady_clock::now();
                    float dt = std::chrono::duration<float>(now - lastFadeTime).count();
                    lastFadeTime = now;
                    _hoverFadeTime = std::min(_hoverFadeTime + dt, 0.2f);
                    if (_hoverFadeTime >= 0.2f)
                        reDraw();
                }
                return;
            }
        }

        // Clear hover if invalid position
        if (_hoverRow != -1 || _hoverCol != -1)
        {
            _hoverRow = -1;
            _hoverCol = -1;
            reDraw();
        }
    }

    // Helper: Find which cells form the winning line
    std::vector<int> getWinningLine()
    {
        const int winningLines[8][3] = {
            // Rows
            {0,1,2}, {3,4,5}, {6,7,8},
            // Columns
            {0,3,6}, {1,4,7}, {2,5,8},
            // Diagonals
            {0,4,8}, {2,4,6}
        };

        Player winner = _game.getWinner();
        if (winner == Player::None) return {};

        for (const auto& line : winningLines)
        {
            if (_game.getCell(line[0]) == winner &&
                _game.getCell(line[1]) == winner &&
                _game.getCell(line[2]) == winner)
            {
                return { line[0], line[1], line[2] };
            }
        }
        return {};
    }

    void onDraw(const gui::Rect& rect) override
    {
        // pick theme
        ThemeIndex themeIdx = clampThemeIndex(getThemeIndex());

        // Define colors
        td::ColorID lineColor;

        td::ColorID bgColor = td::ColorID::White; // THIS COLOR CHANGES WITH THEMES
        td::ColorID accentColor = td::ColorID::Blue; // THIS COLOR CHANGES WITH THEMES
        td::ColorID secondaryColor = td::ColorID::Black; // THIS COLOR CHANGES WITH THEMES

        td::ColorID gridColor = td::ColorID::Black; // THIS COLOR CHANGES WITH THEMES

        td::ColorID winLine = td::ColorID::LimeGreen; // THIS COLOR CHANGES WITH THEMES
        td::ColorID loseLine = td::ColorID::Red; // THIS COLOR CHANGES WITH THEMES

        td::ColorID highlightCOL = td::ColorID::LightGray; // used for hover animation
        td::String imgXLabel, imgOLabel;

        switch (themeIdx)
        {
        case ThemeIndex::Classic:
            bgColor = td::ColorID::White;
            accentColor = td::ColorID::Blue;
            secondaryColor = td::ColorID::Black;
            gridColor = td::ColorID::Black;
            winLine = td::ColorID::LimeGreen;
            loseLine = td::ColorID::Red;
            highlightCOL = td::ColorID::LightGray;
            imgXLabel = ":redX";
            imgOLabel = ":blueO";
            break;
        case ThemeIndex::Nature:
            bgColor = td::ColorID::Snow;
            accentColor = td::ColorID::ForestGreen;
            secondaryColor = td::ColorID::Rust;
            gridColor = td::ColorID::Black;
            winLine = td::ColorID::Lime;
            loseLine = td::ColorID::Red;
            highlightCOL = td::ColorID::BurlyWood;
            imgXLabel = ":woodX";
            imgOLabel = ":woodO";
            break;
        case ThemeIndex::Strawberry:
            bgColor = td::ColorID::FloralWhite;
            accentColor = td::ColorID::Crimson;
            secondaryColor = td::ColorID::MediumSpringGreen;
            gridColor = td::ColorID::Salmon;
            winLine = td::ColorID::ForestGreen;
            loseLine = td::ColorID::Red;
            highlightCOL = td::ColorID::LightGray;
            imgXLabel = ":strawberryX";
            imgOLabel = ":strawberryO";
            break;
        case ThemeIndex::Beachy:
            bgColor = td::ColorID::SeaShell;
            accentColor = td::ColorID::CadetBlue;
            secondaryColor = td::ColorID::DarkKhaki;
            gridColor = td::ColorID::Gray;
            winLine = td::ColorID::Navy;
            loseLine = td::ColorID::DarkSalmon;
            highlightCOL = td::ColorID::LightGray;
            imgXLabel = ":beachyX";
            imgOLabel = ":beachyO";
            break;
        case ThemeIndex::Dark:
            bgColor = td::ColorID::Black;
            accentColor = td::ColorID::DarkOrange;
            secondaryColor = td::ColorID::SteelBlue;
            gridColor = td::ColorID::ObsidianGray;
            winLine = td::ColorID::LimeGreen;
            loseLine = td::ColorID::Red;
            highlightCOL = td::ColorID::SlateGray;
            imgXLabel = ":darkX";
            imgOLabel = ":darkO";
            break;
        default:
            bgColor = td::ColorID::White;
            accentColor = td::ColorID::Blue;
            secondaryColor = td::ColorID::Black;
            gridColor = td::ColorID::Black;
            winLine = td::ColorID::LimeGreen;
            loseLine = td::ColorID::Red;
            highlightCOL = td::ColorID::LightGray;
            imgXLabel = ":redX";
            imgOLabel = ":blueO";
            break;
        }
        // Load images once per theme change (avoid reloading every frame)
        if (!_imagesLoaded || themeIdx != _lastTheme)
        {
            _imgX.load(imgXLabel.c_str());
            _imgO.load(imgOLabel.c_str());
            if (themeIdx == ThemeIndex::Nature)
                _imgWood.load(":XOwood1");
            else
                _imgWood.load(""); // clear/unload wood image for non-Nature themes

            _lastTheme = themeIdx;
            _imagesLoaded = true;
        }
        td::ColorID quitBtnColor = accentColor; // THIS COLOR CHANGES WITH THEMES
        td::ColorID replayBtnColor = secondaryColor; // THIS COLOR CHANGES WITH THEMES
        td::ColorID rectColor = td::ColorID::SeaGreen; // THIS COLOR CHANGES WITH THEMES
        td::ColorID textLblColor = accentColor; // THIS COLOR CHANGES WITH THEMES
        td::ColorID rectBorderColor = textLblColor; // THIS COLOR CHANGES WITH THEMES
        td::ColorID drawLine = secondaryColor;


        gui::Shape::drawRect(rect, bgColor);

        //3x3 grid: columns equal width (1/3 each)
        // rows: top2/10, middle7/10, bottom1/10 (overlay uses bottom row)
        const double totalH = rect.height();
        const double row0H = totalH * 0.20; // title (2/10)
        const double row1H = totalH * 0.70; // play area (7/10)
        const double row2H = totalH * 0.10; // bottom / overlay (1/10)

        const double colW = rect.width() / 3.0;

        // Row boundaries
        const double row0Top = rect.top;
        const double row0Bottom = row0Top + row0H;
        const double row1Top = row0Bottom;
        const double row1Bottom = row1Top + row1H;
        const double row2Top = row1Bottom;
        const double row2Bottom = rect.bottom;

        // Top row: title spanning full width
        {
            td::String title = tr("ticTacToe");
            gui::Font titleFont;
            titleFont.create(CONNECTXO_UI_FONT, 37.0f, gui::Font::Style::Bold, gui::Font::Unit::Point);
            const double hPad = rect.width() * 0.02;
            gui::Rect titleRect(rect.left + hPad, row0Top, rect.right - hPad, row0Bottom);
            gui::DrawableString::draw(title, titleRect, &titleFont, textLblColor, td::TextAlignment::Center, td::VAlignment::Center);
        }

        // Middle row: left two columns unified for playing field
        gui::Rect playCell(rect.left, row1Top, rect.left + colW * 2.0, row1Bottom);
        // Right column in middle row for two stacked placeholders
        gui::Rect rightCol(rect.left + colW * 2.0, row1Top, rect.right, row1Bottom);

        // Compute board layout inside playCell (3x3 board)
        constexpr int rows = 3, cols = 3;
        const double boardScale = 0.92; // fill most of playCell
        const double cell = std::min(playCell.width() * boardScale / cols, playCell.height() * boardScale / rows);
        const double boardW = cell * cols;
        const double boardH = cell * rows;

        // Center board within playCell
        _boardLeft = playCell.left + (playCell.width() - boardW) * 0.5;
        _boardTop = playCell.top + (playCell.height() - boardH) * 0.5;
        _cellSize = cell;

        const double boardRight = _boardLeft + boardW;
        const double boardBottom = _boardTop + boardH;

        const float borderStroke = std::max(1.0f, static_cast<float>(cell * 0.06));
        const float middleStroke = std::max(1.0f, static_cast<float>(cell * 0.10));

        // Draw board background
        gui::Rect borderRect(_boardLeft, _boardTop, boardRight, boardBottom);
        gui::Shape::drawRect(borderRect, bgColor, borderStroke);


        // Draw grid lines for board
        const double v1 = _boardLeft + cell, v2 = _boardLeft + cell * 2.0;
        const double h1 = _boardTop + cell, h2 = _boardTop + cell * 2.0;

        gui::Shape::drawLine(gui::Point(v1, _boardTop), gui::Point(v1, boardBottom), gridColor, middleStroke);
        gui::Shape::drawLine(gui::Point(v2, _boardTop), gui::Point(v2, boardBottom), gridColor, middleStroke);
        gui::Shape::drawLine(gui::Point(_boardLeft, h1), gui::Point(boardRight, h1), gridColor, middleStroke);
        gui::Shape::drawLine(gui::Point(_boardLeft, h2), gui::Point(boardRight, h2), gridColor, middleStroke);

        // Draw wood texture over the board panel if available
        if (themeIdx == ThemeIndex::Nature && _imgWood.isOK())
        {
            _imgWood.draw(borderRect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center, nullptr);
        }

        // Draw pieces inside board
        std::vector<int> winningLine = _game.isGameOver() ? getWinningLine() : std::vector<int>();
        float pulseScale = 1.0f;
        if (!winningLine.empty())
        {
            // Calculate win pulse animation
            auto now = std::chrono::steady_clock::now();
            float elapsedWin = std::chrono::duration<float>(now - _winPulseStartTime).count();
            const float pi = 3.1415926f;
            pulseScale = 1.0f + 0.08f * std::sin(elapsedWin * 8.0f);
        }

        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                int idx = r * 3 + c;
                Player p = _game.getCell(idx);
                if (p == Player::None) continue;

                double cx = _boardLeft + (c + 0.5) * cell;
                double cy = _boardTop + (r + 0.5) * cell;
                double pieceSize = cell * 0.7;

                // Apply pulse scale if part of winning line
                float scale = 1.0f;
                if (!winningLine.empty() && std::find(winningLine.begin(), winningLine.end(), idx) != winningLine.end())
                {
                    scale = pulseScale;
                    pieceSize *= scale;
                }

                gui::Rect pieceRect(cx - pieceSize * 0.5, cy - pieceSize * 0.75,
                    cx + pieceSize * 0.5, cy + pieceSize * 0.75);

                const gui::Image& img = (p == Player::X) ? _imgX : _imgO;
                if (img.isOK())
                    img.draw(pieceRect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center, nullptr);
            }
        }

        // Draw hover preview with fade effect
        if (_hoverRow >= 0 && _hoverCol >= 0 && !_game.isGameOver() &&
            _game.getCurrentPlayer() == _humanPlayer && !_aiMoveScheduled)
        {
            double cx = _boardLeft + (_hoverCol + 0.5) * cell;
            double cy = _boardTop + (_hoverRow + 0.5) * cell;
            double pieceSize = cell * 0.7;

            // Make hover preview smaller for subtle effect
            const double scale = 0.75;  // 75% of normal size
            double hoverSize = pieceSize * scale;
            gui::Rect hoverRect(cx - hoverSize * 0.5, cy - hoverSize * 0.75,
                cx + hoverSize * 0.5, cy + hoverSize * 0.75);

            const gui::Image& hoverImg = (_humanPlayer == Player::X) ? _imgX : _imgO;
            if (hoverImg.isOK())
            {
                // Draw smaller preview without outline (fade-in effect applied by alpha)
                float hoverAlpha = std::min(1.0f, _hoverFadeTime / 0.2f);
                // Note: Alpha blending would need to be supported by the framework
                // For now, draw normally (framework may not support alpha transparency)
                hoverImg.draw(hoverRect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center, nullptr);
            }
        }

        if (!isHintFeatureEnabled())
        {
            _isHintActive = false;
            _hintCell = -1;
        }

        // Draw hint target highlight
        if (_isHintActive && _hintCell >= 0 && _hintCell < 9)
        {
            auto now = std::chrono::steady_clock::now();
            float hintElapsed = std::chrono::duration<float>(now - _hintStartTime).count();
            if (hintElapsed <= 1.2f)
            {
                int hintRow = _hintCell / 3;
                int hintCol = _hintCell % 3;
                double cx = _boardLeft + (hintCol + 0.5) * cell;
                double cy = _boardTop + (hintRow + 0.5) * cell;
                double pieceSize = cell * 0.75;

                float strokeWidth = std::max(2.0f, static_cast<float>(cell * 0.12));
                gui::Rect hintRect(cx - pieceSize * 0.5, cy - pieceSize * 0.5,
                    cx + pieceSize * 0.5, cy + pieceSize * 0.5);
                gui::Shape shHintFlash;
                shHintFlash.createRect(hintRect);
                shHintFlash.drawWire(td::ColorID::Gold, strokeWidth);
            }
            else
            {
                _isHintActive = false;
            }
        }

        // Right column buttons: Quit, Hint, Replay
        {
            const double gap = row1H * 0.06;
            const double vertMargin = row1H * 0.08;
            const double availH = rightCol.height() - vertMargin * 2.0 - gap * 2.0;
            const double rectH = availH / 3.0;
            const double rectW = rightCol.width() * 0.8;
            const double cx = rightCol.left + (rightCol.width() - rectW) * 0.5;

            const double usedH = rectH * 3.0 + gap * 2.0;
            const double extraTopPad = (rightCol.height() - usedH) * 0.5;
            const double topY = rightCol.top + extraTopPad;
            gui::Rect topRect(cx, topY, cx + rectW, topY + rectH);
            double hintTop = topY + rectH + gap;
            gui::Rect hintRect(cx, hintTop, cx + rectW, hintTop + rectH);
            double bottomTop = hintTop + rectH + gap;
            gui::Rect bottomRect(cx, bottomTop, cx + rectW, bottomTop + rectH);

            // store for click handling
            _quitBtn = topRect;
            _hintBtn = hintRect;
            _replayBtn = bottomRect;

            gui::Shape shTop;
            gui::Shape shHint;
            gui::Shape shBottom;
            td::Coord radiusTop = static_cast<td::Coord>(std::min(topRect.width(), topRect.height()) * 0.5);
            td::Coord radiusHint = static_cast<td::Coord>(std::min(hintRect.width(), hintRect.height()) * 0.5);
            td::Coord radiusBottom = static_cast<td::Coord>(std::min(bottomRect.width(), bottomRect.height()) * 0.5);
            shTop.createRoundedRect(topRect, radiusTop, 1.0f);
            shHint.createRoundedRect(hintRect, radiusHint, 1.0f);
            shBottom.createRoundedRect(bottomRect, radiusBottom, 1.0f);

            const bool hintFeatureEnabled = isHintFeatureEnabled();
            const bool hintEnabled = isHintEnabled();
            td::ColorID hintFillColor = hintEnabled ? td::ColorID::Gold : td::ColorID::Gainsboro;
            float quitBorder = 3.0f;
            float replayBorder = 3.0f;
            float hintBorder = hintEnabled ? 2.0f : 1.0f;

            // Enhance colors on hover
            if (_quitBtnHovered)
            {
                quitBtnColor = td::ColorID::LightBlue; // lighter on hover
                quitBorder = 5.0f; // thicker border
            }
            if (hintEnabled && _hintBtnHovered)
            {
                hintFillColor = td::ColorID::Yellow;
                hintBorder = 3.0f;
            }
            if (_replayBtnHovered)
            {
                replayBtnColor = td::ColorID::LightGray; // lighter on hover
                replayBorder = 5.0f; // thicker border
            }

            shTop.drawFillAndWire(quitBtnColor, bgColor, quitBorder);
            if (hintFeatureEnabled)
            {
                shHint.drawFillAndWire(hintFillColor, bgColor, hintBorder);
            }
            shBottom.drawFillAndWire(replayBtnColor, bgColor, replayBorder);

            gui::Font btnFont;
            float btnFontSize = std::max(1.0f, static_cast<float>(rectH * 0.28));
            btnFont.create(CONNECTXO_UI_FONT, btnFontSize, gui::Font::Style::BoldItalic, gui::Font::Unit::Point);
            td::String quitLbl = tr("quitBtn");
            td::String hintLbl = tr("Hint");
            td::String replayLbl = tr("replayBtn");
            gui::DrawableString::draw(quitLbl, topRect, &btnFont, bgColor, td::TextAlignment::Center, td::VAlignment::Center);
            if (hintFeatureEnabled)
            {
                gui::DrawableString::draw(hintLbl, hintRect, &btnFont, hintEnabled ? td::ColorID::Black : td::ColorID::DarkGray, td::TextAlignment::Center, td::VAlignment::Center);
            }
            gui::DrawableString::draw(replayLbl, bottomRect, &btnFont, bgColor, td::TextAlignment::Center, td::VAlignment::Center);
        }

        // Bottom row: show wins (col0) and losses (col2)
        // Reset countersUpdated when a new game is in progress
        if (!_game.isGameOver())
            _countersUpdated = false;

        // If game over and counters not updated yet, update them
        if (_game.isGameOver() && !_countersUpdated)
        {
            Player winner = _game.getWinner();
            if (winner == _humanPlayer)
            {
                ++_winCounter; // human wins
                ++ScoreManager::getInstance().getTTTStats().wins;
                ScoreManager::getInstance().saveTTTStats();
            }
            else if (winner == _AIrole)
            {
                ++_lossCounter; // human loses
                ++ScoreManager::getInstance().getTTTStats().losses;
                ScoreManager::getInstance().saveTTTStats();
            }
            _countersUpdated = true;
        }

        // Prepare bottom labels
        cnt::StringBuilderSmall sbW;
        sbW.appendString(tr("winsCounterLbl"));
        sbW.appendCString(" ");
        std::string tmpW = std::to_string(_winCounter);
        sbW.appendString(tmpW.c_str());
        td::String winsStr = sbW.toString();

        cnt::StringBuilderSmall sbL;
        sbL.appendString(tr("lossesCounterLbl"));
        sbL.appendCString(" ");
        std::string tmpL = std::to_string(_lossCounter);
        sbL.appendString(tmpL.c_str());
        td::String lossStr = sbL.toString();

        // Draw bottom left (col0) and bottom right (col2) with padding and larger font
        const double hPad = std::max(rect.width() * 0.02, colW * 0.06); // horizontal padding
        float fontSizePt = std::max(1.0f, static_cast<float>(row2H * 0.25));
        gui::Font counterFont;
        counterFont.create(CONNECTXO_UI_FONT, fontSizePt, gui::Font::Style::Bold, gui::Font::Unit::Point);

        gui::Rect bottomCol0(rect.left + hPad, row2Top, rect.left + colW - hPad, row2Bottom);
        gui::DrawableString::draw(winsStr, bottomCol0, &counterFont, textLblColor, td::TextAlignment::Left, td::VAlignment::Center);

        gui::Rect bottomCol2(rect.left + colW * 2.0 + hPad, row2Top, rect.right - hPad, row2Bottom);
        gui::DrawableString::draw(lossStr, bottomCol2, &counterFont, textLblColor, td::TextAlignment::Right, td::VAlignment::Center);

        // Draw game over overlay/message if needed
        if (_game.isGameOver())
        {
            // Highlight winning line by drawing a single thick line between the centers of the first and last cells
            auto winningLine = getWinningLine();
            if (winningLine.size() == 3)
            {
                const double cellSz = _cellSize;
                const float winStroke = std::max(3.0f, static_cast<float>(cellSz * 0.15));
                //td::ColorID lineColor;

                if (_game.getWinner() == _humanPlayer)
                    lineColor = winLine;
                else if (_game.getWinner() == _AIrole)
                    lineColor = loseLine;


                int idx0 = winningLine.front();
                int idx2 = winningLine.back();
                int row0 = idx0 / 3; int col0 = idx0 % 3;
                int row2 = idx2 / 3; int col2 = idx2 % 3;

                double cx0 = _boardLeft + (col0 + 0.5) * cellSz;
                double cy0 = _boardTop + (row0 + 0.5) * cellSz;
                double cx2 = _boardLeft + (col2 + 0.5) * cellSz;
                double cy2 = _boardTop + (row2 + 0.5) * cellSz;

                double dx = cx2 - cx0;
                double dy = cy2 - cy0;
                double len = std::hypot(dx, dy);
                if (len > 1e-6)
                {
                    double nx = dx / len;
                    double ny = dy / len;
                    double pad = cellSz * 0.6;
                    double sx = cx0 - nx * pad;
                    double sy = cy0 - ny * pad;
                    double ex = cx2 + nx * pad;
                    double ey = cy2 + ny * pad;
                    gui::Shape::drawLine(gui::Point(sx, sy), gui::Point(ex, ey), lineColor, winStroke);
                }
                else
                {
                    gui::Shape::drawLine(gui::Point(cx0, cy0), gui::Point(cx2, cy2), lineColor, winStroke);
                }
            }

            td::String lbl;
            if (_game.getWinner() == _humanPlayer)
                lbl = tr("youWin");
            else if (_game.getWinner() == _AIrole)
                lbl = tr("youLose");
            else
                lbl = tr("draw");

            // Message drawn in the bottom row center column
            gui::Rect overlay(rect.left + colW, row2Top, rect.left + colW * 2.0, row2Bottom);
            //gui::Shape::drawRect(overlay, td::ColorID::Transparent);
            float overlayFontSize = std::max(6.0f, static_cast<float>(row2H * 0.35));

            td::ColorID overlayTxtColor;
            if (_game.getWinner() == _humanPlayer)
                overlayTxtColor = winLine;
            else if (_game.getWinner() == _AIrole)
                overlayTxtColor = loseLine;
            else
                overlayTxtColor = drawLine;

            gui::Font overlayFont;

            overlayFont.create(CONNECTXO_UI_FONT, overlayFontSize, gui::Font::Style::BoldItalic, gui::Font::Unit::Point);
            gui::DrawableString::draw(lbl, overlay, &overlayFont, overlayTxtColor, td::TextAlignment::Center, td::VAlignment::Center);
        }

        if (_isHintActive)
            reDraw();
    }

    void onPrimaryButtonReleased(const gui::InputDevice& inputDevice) override
    {
        // Early exit checks
        if (_cellSize <= 0) return; // Not yet drawn

        const gui::Point& pt = inputDevice.getModelPoint();

        // helper to test point in rect
        auto pointInRect = [](const gui::Rect& r, const gui::Point& p) {
            return (p.x >= r.left) && (p.x <= r.right) && (p.y >= r.top) && (p.y <= r.bottom);
            };

        // Check placeholder buttons first
        if (pointInRect(_quitBtn, pt))
        {
            // Quit: reset counters and notify parent
            _winCounter = 0;
            _lossCounter = 0;
            _countersUpdated = false;
            _isHintActive = false;
            _hintCell = -1;
            // invalidate any pending AI moves
            ++_aiGen;
            _aiMoveScheduled = false;
            _hoverRow = -1;
            _hoverCol = -1;
            if (_onQuit)
                _onQuit();
            else
            {
                // Try to close/hide parent window if no callback provided
                gui::Window* pWnd = getParentWindow();
                if (pWnd)
                    pWnd->hide(true);
            }
            reDraw();
            return;
        }

        if (pointInRect(_replayBtn, pt))
        {
            // Replay: remember counters, reset game board
            ++_aiGen; // invalidate pending AI
            _aiMoveScheduled = false;
            _hoverRow = -1;
            _hoverCol = -1;
            _isHintActive = false;
            _hintCell = -1;
            // Toggle human/AI roles before starting new game
            _humanPlayer = (_humanPlayer == Player::X) ? Player::O : Player::X;
            if (_humanPlayer == Player::X)
                _AIrole = Player::O;
            else if (_humanPlayer == Player::O)
                _AIrole = Player::X;
            // start a new game with X always starting
            _game.reset(Player::X);
            _countersUpdated = false;
            _winPulseStartTime = std::chrono::steady_clock::now(); // Reset win pulse timer

            // If AI now plays as X (i.e. human is O), schedule AI first move
            if (_humanPlayer != Player::X)
            {
                _aiMoveScheduled = true;
                int gen = ++_aiGen;
                std::thread aiStartThread([this, gen]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    Game::Move aiMove = _aiPlayer.chooseMove(_game, 12);
                    auto* fn = new gui::AsyncFn([this, aiMove, gen]() {
                        if (gen == _aiGen)
                        {
                            if (aiMove >= 0)
                            {
                                _game.makeMove(aiMove);
                                reDraw();
                            }
                        }
                        _aiMoveScheduled = false;
                        });
                    gui::NatObject::asyncCall(fn, true);
                    });
                aiStartThread.detach();
            }

            if (_onReplay)
                _onReplay();
            reDraw();
            return;
        }

        // Check Hint button
        if (pointInRect(_hintBtn, pt))
        {
            if (isHintEnabled())
            {
                int bestMove = calculateOptimalHint();
                if (bestMove >= 0 && bestMove < 9)
                {
                    _hintCell = bestMove;
                    _isHintActive = true;
                    _hintStartTime = std::chrono::steady_clock::now();
                    reDraw();
                }
            }
            return;
        }

        // Otherwise, handle board click (only if game is not over and it's human's turn)
        if (_game.isGameOver() || _aiMoveScheduled) return;
        if (_game.getCurrentPlayer() != _humanPlayer) return; // human's turn

        int col = static_cast<int>((pt.x - _boardLeft) / _cellSize);
        int row = static_cast<int>((pt.y - _boardTop) / _cellSize);

        if (row < 0 || row >= 3 || col < 0 || col >= 3) return;

        int moveIndex = row * 3 + col;
        if (_game.getCell(moveIndex) != Player::None) return;

        _game.makeMove(moveIndex);
        _clickSound.play();

        // Clear hover after making a move
        _hoverRow = -1;
        _hoverCol = -1;

        //reDraw(); // Redraw board after player move immediately

        if (_game.isGameOver() && !_countersUpdated)
        {
            Player winner = _game.getWinner();
            if (winner == _humanPlayer)
            {
                ++_winCounter;
                ++ScoreManager::getInstance().getTTTStats().wins;
                ScoreManager::getInstance().saveTTTStats();
            }
            else if (winner == _AIrole)
            {
                ++_lossCounter;
                ++ScoreManager::getInstance().getTTTStats().losses;
                ScoreManager::getInstance().saveTTTStats();
            }
            _countersUpdated = true;
        }

        reDraw();
        if (_game.isGameOver())
        {
            _aiMoveScheduled = false;
            return;
        }

        // Schedule AI move after a short delay
        scheduleAIMove();
    }

private:
    void scheduleAIMove()
    {
        _aiMoveScheduled = true;
        int gen = ++_aiGen;
        std::thread aiThread([this, gen]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            Game::Move aiMove = _aiPlayer.chooseMove(_game, 10);
            auto* fn = new gui::AsyncFn([this, aiMove, gen]() {
                if (gen != _aiGen)
                {
                    _aiMoveScheduled = false;
                    return;
                }
                if (!_game.isGameOver() && aiMove >= 0)
                {
                    _game.makeMove(aiMove);
                }
                _aiMoveScheduled = false;
                reDraw();
                });
            gui::NatObject::asyncCall(fn, true);
            });
        aiThread.detach();
    }

    bool isHintFeatureEnabled() const
    {
        const gui::Application* app = getApplication();
        if (!app)
            return true;
        auto props = const_cast<gui::Application*>(app)->getProperties();
        if (!props)
            return true;
        return props->getValue("showHint", 1) != 0;
    }

    bool isHintEnabled() const
    {
        return isHintFeatureEnabled() && !_game.isGameOver() && _game.getCurrentPlayer() == _humanPlayer && !_aiMoveScheduled;
    }

    // Calculate optimal hint move using AI's minimax evaluation
    int calculateOptimalHint()
    {
        // Simply use the AI player's own best move calculation
        // from the human player's perspective
        const auto validMoves = _game.getValidMoves();
        if (validMoves.empty())
            return -1;

        // Use AI engine to evaluate - it will find the best move
        // using minimax with alpha-beta pruning at depth 8
        return _aiPlayer.chooseMove(_game, 8);
    }


private:
    TicTacToe _game;
    AIPlayer _aiPlayer;
    gui::Image _imgX;
    gui::Image _imgO;
    gui::Image _imgWood;
    gui::Sound _clickSound;
    double _boardLeft, _boardTop, _cellSize;
    bool _aiMoveScheduled;

    int _winCounter;
    int _lossCounter;
    bool _countersUpdated;

    // Button rects for click detection
    gui::Rect _quitBtn;
    gui::Rect _replayBtn;
    gui::Rect _hintBtn;

    // Callbacks parent can set to handle close/replay actions
    std::function<void()> _onQuit;
    std::function<void()> _onReplay;

    // AI generation counter to invalidate pending AI moves when resetting/closing
    int _aiGen;

    // Current human player, toggles between X and O
    Player _humanPlayer;
    Player _AIrole;

    // Theme-aware image loading state
    ThemeIndex _lastTheme;
    bool _imagesLoaded;

    // Hover preview state
    int _hoverRow;
    int _hoverCol;

    // Hint system
    bool _isHintActive = false;
    int _hintCell = -1;
    std::chrono::steady_clock::time_point _hintStartTime;

    // Hover fade animation
    float _hoverFadeTime = 0.0f;

    // Win pulse animation
    std::chrono::steady_clock::time_point _winPulseStartTime;

    // Button hover states
    bool _quitBtnHovered;
    bool _replayBtnHovered;
    bool _hintBtnHovered;

};