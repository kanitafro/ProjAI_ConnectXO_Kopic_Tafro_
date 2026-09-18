#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/Image.h>
#include <gui/Sound.h>
#include <gui/DrawableString.h>
#include <gui/Application.h>
#include <td/Types.h>
#include "ScoreManager.h"
#include <algorithm>
#include <cnt/StringBuilder.h>
#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <chrono>
#include <cmath>
#include <gui/GridLayout.h>
#include <gui/TabView.h>
#include <gui/BaseView.h>
#include "ConnectFour.h"
#include "AIPlayer.h"
#include "Theme.h"

extern "C" int getThemeIndex();

#ifndef CONNECTXO_UI_FONT
#if defined(__APPLE__)
#define CONNECTXO_UI_FONT "Helvetica Neue"
#else
#define CONNECTXO_UI_FONT "Segoe UI"
#endif
#endif


// Connect4 view using natID GUI. Tokens are images ":bluetoken" and ":yellowtoken".
class Connect4View : public gui::Canvas
{
public:
	Connect4View()
		: Canvas({ gui::InputDevice::Event::PrimaryClicks, gui::InputDevice::Event::CursorMove })
		, _game()
		, _aiPlayer()
		, _imgToken1()
		, _imgToken2()
		, _imgWood() // used in NATURE / LOG CABIN theme
		, _fallingSound(":falling")
		, _boardLeft(0)
		, _boardTop(0)
		, _cellSize(0)
		, _aiMoveScheduled(false)
		, _isFalling(false)
		, _hoverCol(-1)
		, _hoverRow(-1)
		, _quitBtnHovered(false)
		, _replayBtnHovered(false)
		, _winCounter(0)
		, _lossCounter(0)
		, _countersUpdated(false)
		, _aiGen(0)
		, _humanPlayer(Player::X)
		, _AIrole(Player::O)
		, _lastTheme(ThemeIndex::Classic)
		, _imagesLoaded(false)
		, _animTimeValid(false)
		, _isHintActive(false)
		, _hintCol(-1)
		, _hintStartTime(std::chrono::steady_clock::now())
		, _hoverFadeTime(0.0f)
		, _winPulseStartTime(std::chrono::steady_clock::now())
	{
		setPreferredFrameRateRange(60, 60);
		enableResizeEvent(true);
		_game.reset(Player::X); // player X goes first (human)

		// Load persisted scores from ScoreManager
		_winCounter = ScoreManager::getInstance().getC4Stats().wins;
		_lossCounter = ScoreManager::getInstance().getC4Stats().losses;
	}

	// Allow parent to handle closing/removing this view
	void onQuit(const std::function<void()>& fn) { _onQuit = fn; }
	void onReplay(const std::function<void()>& fn) { _onReplay = fn; }

protected:
	void onResize(const gui::Size& newSize) override
	{
		reDraw();
	}

	// Helper: find the4-in-a-row winning line (returns linear indices row*WIDTH+col)
	std::vector<int> getWinningLine() const
	{
		std::vector<int> line;
		for (int row = 0; row < ConnectFour::HEIGHT; ++row)
		{
			for (int col = 0; col < ConnectFour::WIDTH; ++col)
			{
				Player p = _game.getCell(row, col);
				if (p == Player::None)
					continue;

				// directions: right, up, up-right, up-left
				const int dirs[4][2] = { {1,0}, {0,1}, {1,1}, {-1,1} };
				for (const auto& d : dirs)
				{
					int dx = d[0];
					int dy = d[1];
					bool ok = true;
					std::vector<int> tmp;
					tmp.push_back(row * ConnectFour::WIDTH + col);
					for (int k = 1; k < 4; ++k)
					{
						int nr = row + dy * k;
						int nc = col + dx * k;
						if (nr < 0 || nr >= ConnectFour::HEIGHT || nc < 0 || nc >= ConnectFour::WIDTH)
						{
							ok = false;
							break;
						}
						if (_game.getCell(nr, nc) != p)
						{
							ok = false;
							break;
						}
						tmp.push_back(nr * ConnectFour::WIDTH + nc);
					}
					if (ok && tmp.size() == 4)
						return tmp;
				}
			}
		}
		return {};
	}

	void onDraw(const gui::Rect& rect) override
	{
		// pick theme
		ThemeIndex themeIdx = clampThemeIndex(getThemeIndex());

		// Define colors
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
			imgXLabel = ":redtoken";
			imgOLabel = ":yellowtoken";
			break;
		case ThemeIndex::Nature:
			bgColor = td::ColorID::Snow;
			accentColor = td::ColorID::ForestGreen;
			secondaryColor = td::ColorID::Rust;
			gridColor = td::ColorID::Tan;
			winLine = td::ColorID::Lime;
			loseLine = td::ColorID::Red;
			highlightCOL = td::ColorID::LightGray;
			imgXLabel = ":token1wood";
			imgOLabel = ":token2wood";
			break;
		case ThemeIndex::Strawberry:
			bgColor = td::ColorID::FloralWhite;
			accentColor = td::ColorID::Crimson;
			secondaryColor = td::ColorID::MediumSpringGreen;
			gridColor = td::ColorID::Salmon;
			winLine = td::ColorID::ForestGreen;
			loseLine = td::ColorID::Red;
			highlightCOL = td::ColorID::LightGray;
			imgXLabel = ":token1strawberry";
			imgOLabel = ":token2strawberry";
			break;
		case ThemeIndex::Beachy:
			bgColor = td::ColorID::SeaShell;
			accentColor = td::ColorID::CadetBlue;
			secondaryColor = td::ColorID::DarkKhaki;
			gridColor = td::ColorID::Gray;
			winLine = td::ColorID::Navy;
			loseLine = td::ColorID::DarkSalmon;
			highlightCOL = td::ColorID::LightGray;
			imgXLabel = ":token1beachy";
			imgOLabel = ":token2beachy";
			break;
		case ThemeIndex::Dark:
			bgColor = td::ColorID::Black;
			accentColor = td::ColorID::DarkOrange;
			secondaryColor = td::ColorID::SteelBlue;
			gridColor = td::ColorID::ObsidianGray;
			winLine = td::ColorID::LimeGreen;
			loseLine = td::ColorID::Red;
			highlightCOL = td::ColorID::SlateGray;
			highlightCOL = td::ColorID::LightGray;
			imgXLabel = ":token1dark";
			imgOLabel = ":token2dark";
			break;
		default:
			bgColor = td::ColorID::White;
			accentColor = td::ColorID::Blue;
			secondaryColor = td::ColorID::Black;
			gridColor = td::ColorID::Black;
			winLine = td::ColorID::LimeGreen;
			loseLine = td::ColorID::Red;
			highlightCOL = td::ColorID::LightGray;
			imgXLabel = ":redtoken";
			imgOLabel = ":yellowtoken";
			break;
		}
		// Load images once per theme change (avoid reloading every frame)
		if (!_imagesLoaded || themeIdx != _lastTheme)
		{
			_imgToken1.load(imgXLabel.c_str());
			_imgToken2.load(imgOLabel.c_str());
			if (themeIdx == ThemeIndex::Nature)
				_imgWood.load(":wood2");
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


		// Draw background
		gui::Shape::drawRect(rect, bgColor);

		// Calculate layout: 3x3 grid with 3 rows and 3 columns
		double row1Height = rect.height() * 2.0 / 10.0;
		double row2Height = rect.height() * 7.0 / 10.0;
		double row3Height = rect.height() * 1.0 / 10.0;

		double col1Width = rect.width() / 3.0;
		double col2Width = rect.width() / 3.0;
		double col3Width = rect.width() / 3.0;

		// ===== ROW 1: Title =====
		gui::Rect titleRect(rect.left, rect.top, rect.right, rect.top + row1Height);
		gui::Shape::drawRect(titleRect, bgColor);

		// Draw centered "connectFour" title
		gui::Font titleFont;
		titleFont.create(CONNECTXO_UI_FONT, 37.0f, gui::Font::Style::Bold, gui::Font::Unit::Point);
		td::String titleText = tr("connectFour");
		gui::DrawableString::draw(titleText, titleRect, &titleFont, textLblColor, td::TextAlignment::Center, td::VAlignment::Center);

		// ===== ROW 2: Game Board + Buttons =====
		double row2Top = rect.top + row1Height;

		// ROW 2, COLUMNS 1 & 2: Game Board Area
		gui::Rect boardAreaRect(rect.left, row2Top, rect.left + 2.0 * col1Width, row2Top + row2Height);

		constexpr int rows = ConnectFour::HEIGHT;
		constexpr int cols = ConnectFour::WIDTH;

		// Calculate board size to fit in the board area
		double maxCellWidth = boardAreaRect.width() / cols;
		double maxCellHeight = boardAreaRect.height() / rows;
		double cell = std::min(maxCellWidth, maxCellHeight);

		double boardW = cell * cols;
		double boardH = cell * rows;

		_boardLeft = boardAreaRect.left + (boardAreaRect.width() - boardW) * 0.5;
		_boardTop = boardAreaRect.top + (boardAreaRect.height() - boardH) * 0.5;
		_cellSize = cell;

		double boardRight = _boardLeft + boardW;
		double boardBottom = _boardTop + boardH;

		const float borderStroke = std::max(1.0f, static_cast<float>(cell * 0.1));

		// Draw background panel for board
		gui::Rect boardPanel(_boardLeft, _boardTop, boardRight, boardBottom);
		gui::Shape::drawRect(boardPanel, bgColor, borderStroke);


		// Draw grid cells (6 rows x 7 columns of circles)
		for (int r = 0; r < rows; ++r)
		{
			for (int c = 0; c < cols; ++c)
			{
				double l = _boardLeft + c * cell;
				double t = _boardTop + (rows - 1 - r) * cell;
				double rgt = l + cell;
				double b = t + cell;

				gui::Rect cellRect(l, t, rgt, b);

				// Fill cell with gridColor to cover corners
				gui::Shape shFill;
				shFill.createRect(cellRect);
				shFill.drawFill(gridColor);

				// Draw circular hole
				auto slotInset = cell * 0.08;
				gui::Rect slotRect(l + slotInset, t + slotInset, rgt - slotInset, b - slotInset);
				{
					gui::Shape shHole;
					shHole.createOval(slotRect, 1.0f);
					shHole.drawFill(bgColor);
				}
			}
		}

		// Draw wood texture behind slots if available
		if (themeIdx == ThemeIndex::Nature && _imgWood.isOK())
		{
			_imgWood.draw(boardPanel, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center, nullptr);
		}

		// Draw tokens already placed on the board
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

		for (int r = 0; r < rows; ++r)
		{
			for (int c = 0; c < cols; ++c)
			{
				Player p = _game.getCell(r, c);
				if (p == Player::None)
					continue;

				// Check if this token is part of winning line
				float tokenScale = 1.0f;
				if (!winningLine.empty())
				{
					int linearIdx = r * ConnectFour::WIDTH + c;
					if (std::find(winningLine.begin(), winningLine.end(), linearIdx) != winningLine.end())
					{
						tokenScale = pulseScale;
					}
				}
				drawToken(getTokenRectForCell(r, c), p, false, highlightCOL, tokenScale);
			}
		}

		// Draw hover preview at landing cell with fade effect
		if (shouldDrawHover())
		{
			float hoverAlpha = std::min(1.0f, _hoverFadeTime / 0.2f); // 0.2 second fade-in
			drawToken(getTokenRectForCell(_hoverRow, _hoverCol), _game.getCurrentPlayer(), true, highlightCOL, 1.0f, hoverAlpha);
		}

		if (!isHintFeatureEnabled())
		{
			_isHintActive = false;
			_hintCol = -1;
		}

		// Draw hint target highlight with circular pulsing glow (balanced visibility)
		if (_isHintActive && _hintCol >= 0 && _hintCol < ConnectFour::WIDTH)
		{
			auto now = std::chrono::steady_clock::now();
			float hintElapsed = std::chrono::duration<float>(now - _hintStartTime).count();
			if (hintElapsed <= 2.0f)
			{
				int hintRow = _game.getLowestEmptyRow(_hintCol);
				if (hintRow >= 0)
				{
					double l = _boardLeft + _hintCol * cell;
					double t = _boardTop + (rows - 1 - hintRow) * cell;
					double rgt = l + cell;
					double b = t + cell;

					// Bright yellow for visibility
					td::ColorID hintColor = td::ColorID::Yellow;

					// Moderate pulse: 1.0 + 0.15 * sin(elapsed * 7)
					const float pi = 3.1415926f;
					float pulseScale = 1.0f + 0.15f * std::sin(hintElapsed * 7.0f * pi);

					// Draw circular glow (oval shape) - more visible
					double centerX = (l + rgt) * 0.5;
					double centerY = (t + b) * 0.5;
					double radius = (rgt - l) * 0.45 * pulseScale;

					gui::Rect glowRect(centerX - radius, centerY - radius, centerX + radius, centerY + radius);
					float glowStrokeWidth = 4.0f;  // Balanced stroke for visibility

					gui::Shape shGlow;
					shGlow.createOval(glowRect, glowStrokeWidth);
					shGlow.drawWire(hintColor, glowStrokeWidth);
				}
			}
			else
			{
				_isHintActive = false;
			}
		}

		// Draw falling token on top of board tokens
		bool finalizeDrop = false;
		if (_isFalling)
		{
			updateFallingTarget(cell);
			advanceFallingToken(cell);
			drawToken(getTokenRectForColumnAtY(_fallingToken.column, _fallingToken.currentY), _fallingToken.type, false, highlightCOL);
			finalizeDrop = _fallingToken.settled;
		}

		// Draw slot outlines last so the tokens look like they are behind the grid
		for (int r = 0; r < rows; ++r)
		{
			for (int c = 0; c < cols; ++c)
			{
				double l = _boardLeft + c * cell;
				double t = _boardTop + (rows - 1 - r) * cell;
				double rgt = l + cell;
				double b = t + cell;
				auto slotInset = cell * 0.08;
				gui::Rect slotRect(l + slotInset, t + slotInset, rgt - slotInset, b - slotInset);
				gui::Shape shOutline;
				shOutline.createOval(slotRect, borderStroke);
				shOutline.drawWire(gridColor, borderStroke);
			}
		}

		// Finalize falling token BEFORE game over check so move is made first
		if (_isFalling && finalizeDrop)
		{
			finalizeFallingToken();
		}

		// ROW 2, COLUMN 3: Buttons (Quit, Hint, Replay)
		// Make buttons the same size as TicTacToe by using fixed proportions
		double btnColLeft = rect.left + 2.0 * col1Width;
		const double colW = rect.width() / 3.0;
		const double totalH = rect.height();
		const double fixedRowH = totalH * 0.70;  // TicTacToe's middle row
		const double gap = fixedRowH * 0.06;
		const double vertMargin = fixedRowH * 0.08;
		const double availH = fixedRowH - vertMargin * 2.0 - gap * 2.0;
		const double rectH = availH / 3.0;
		const double rectW = colW * 0.8;
		const double cx = btnColLeft + (colW - rectW) * 0.5;

		// Center buttons vertically in actual row2Height
		const double usedH = rectH * 3.0 + gap * 2.0;
		const double extraTopPad = (row2Height - usedH) * 0.5;
		const double topY = row2Top + extraTopPad;
		_quitBtn = gui::Rect(cx, topY, cx + rectW, topY + rectH);
		double hintTop = topY + rectH + gap;
		_hintBtn = gui::Rect(cx, hintTop, cx + rectW, hintTop + rectH);
		double replayTop = hintTop + rectH + gap;
		_replayBtn = gui::Rect(cx, replayTop, cx + rectW, replayTop + rectH);

		// Draw buttons as rounded rectangles with fill and border
		gui::Shape shQuit;
		gui::Shape shHint;
		gui::Shape shReplay;
		td::Coord radiusQuit = static_cast<td::Coord>(std::min(_quitBtn.width(), _quitBtn.height()) * 0.5);
		td::Coord radiusHint = static_cast<td::Coord>(std::min(_hintBtn.width(), _hintBtn.height()) * 0.5);
		td::Coord radiusReplay = static_cast<td::Coord>(std::min(_replayBtn.width(), _replayBtn.height()) * 0.5);
		shQuit.createRoundedRect(_quitBtn, radiusQuit, 1.0f);
		shHint.createRoundedRect(_hintBtn, radiusHint, 1.0f);
		shReplay.createRoundedRect(_replayBtn, radiusReplay, 1.0f);

		// Fill with color and draw border
		td::ColorID quitFillColor = quitBtnColor;
		td::ColorID replayFillColor = replayBtnColor;
		const bool hintFeatureEnabled = isHintFeatureEnabled();
		const bool hintEnabled = isHintEnabled();
		td::ColorID hintFillColor = hintEnabled ? td::ColorID::Gold : td::ColorID::Gainsboro;
		float quitBorder = 3.0f;
		float replayBorder = 3.0f;
		float hintBorder = hintEnabled ? 2.0f : 1.0f;

		// Enhance colors on hover
		if (_quitBtnHovered)
		{
			quitFillColor = td::ColorID::LightBlue; // lighter on hover
			quitBorder = 5.0f; // thicker border
		}
		if (hintEnabled && _hintBtnHovered)
		{
			hintFillColor = td::ColorID::Yellow;
			hintBorder = 3.0f;
		}
		if (_replayBtnHovered)
		{
			replayFillColor = td::ColorID::LightGray; // lighter on hover
			replayBorder = 5.0f; // thicker border
		}

		shQuit.drawFillAndWire(quitFillColor, bgColor, quitBorder);
		if (hintFeatureEnabled)
		{
			shHint.drawFillAndWire(hintFillColor, bgColor, hintBorder);
		}
		shReplay.drawFillAndWire(replayFillColor, bgColor, replayBorder);

		// Draw button labels
		gui::Font btnFont;
		float btnFontSize = std::max(1.0f, static_cast<float>(rectH * 0.28));
		btnFont.create(CONNECTXO_UI_FONT, btnFontSize, gui::Font::Style::BoldItalic, gui::Font::Unit::Point);
		td::String quitLbl = tr("quitBtn");
		td::String hintLbl = tr("Hint");
		td::String replayLbl = tr("replayBtn");
		gui::DrawableString::draw(quitLbl, _quitBtn, &btnFont, bgColor, td::TextAlignment::Center, td::VAlignment::Center);
		if (hintFeatureEnabled)
		{
			gui::DrawableString::draw(hintLbl, _hintBtn, &btnFont, hintEnabled ? td::ColorID::Black : td::ColorID::DarkGray, td::TextAlignment::Center, td::VAlignment::Center);
		}
		gui::DrawableString::draw(replayLbl, _replayBtn, &btnFont, bgColor, td::TextAlignment::Center, td::VAlignment::Center);

		// ROW 3: Counters
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
				++ScoreManager::getInstance().getC4Stats().wins;
				ScoreManager::getInstance().saveC4Stats();
			}
			else if (winner == _AIrole)
			{
				++_lossCounter; // human loses
				++ScoreManager::getInstance().getC4Stats().losses;
				ScoreManager::getInstance().saveC4Stats();
			}
			_countersUpdated = true;
		}

		// ===== ROW 3: Counters Display =====
		double row3Top = row2Top + row2Height;

		// Add horizontal padding like in TicTacToe
		const double hPad = std::max(rect.width() * 0.02, col1Width * 0.06); // horizontal padding

		// ROW 3, COLUMN 1: Win Counter
		gui::Rect winCounterRect(rect.left + hPad, row3Top, rect.left + col1Width - hPad, row3Top + row3Height);
		cnt::StringBuilderSmall sbW;
		sbW.appendString(tr("winsCounterLbl"));
		sbW.appendCString(" ");
		std::string tmpW = std::to_string(_winCounter);
		sbW.appendString(tmpW.c_str());
		td::String winText = sbW.toString();
		gui::Font counterFont;
		float counterFontSize = std::max(1.0f, static_cast<float>(row3Height * 0.25));
		counterFont.create(CONNECTXO_UI_FONT, counterFontSize, gui::Font::Style::Bold, gui::Font::Unit::Point);
		gui::DrawableString::draw(winText, winCounterRect, &counterFont, textLblColor, td::TextAlignment::Left, td::VAlignment::Center);

		// ROW 3, COLUMN 3: Loss Counter
		gui::Rect lossCounterRect(rect.left + 2.0 * col1Width + hPad, row3Top, rect.right - hPad, row3Top + row3Height);
		cnt::StringBuilderSmall sbL;
		sbL.appendString(tr("lossesCounterLbl"));
		sbL.appendCString(" ");
		std::string tmpL = std::to_string(_lossCounter);
		sbL.appendString(tmpL.c_str());
		td::String lossText = sbL.toString();
		gui::DrawableString::draw(lossText, lossCounterRect, &counterFont, textLblColor, td::TextAlignment::Right, td::VAlignment::Center);

		// ===== Game Over Overlay (Below Row 2 buttons) =====
		if (_game.isGameOver())
		{
			// Draw winning line if applicable
			auto winningLine = getWinningLine();
			if (winningLine.size() == 4)
			{
				int idx0 = winningLine.front();
				int idx3 = winningLine.back();
				int row0 = idx0 / ConnectFour::WIDTH;
				int col0 = idx0 % ConnectFour::WIDTH;
				int row3 = idx3 / ConnectFour::WIDTH;
				int col3 = idx3 % ConnectFour::WIDTH;

				double cx0 = _boardLeft + (col0 + 0.5) * cell;
				double cy0 = _boardTop + (rows - 1 - row0 + 0.5) * cell;
				double cx3 = _boardLeft + (col3 + 0.5) * cell;
				double cy3 = _boardTop + (rows - 1 - row3 + 0.5) * cell;

				double dx = cx3 - cx0;
				double dy = cy3 - cy0;
				double len = std::hypot(dx, dy);
				if (len > 1e-6)
				{
					double nx = dx / len;
					double ny = dy / len;
					double pad = cell * 0.6;
					double sx = cx0 - nx * pad;
					double sy = cy0 - ny * pad;
					double ex = cx3 + nx * pad;
					double ey = cy3 + ny * pad;

					double margin = cell * 0.06;
					sx = std::clamp(sx, _boardLeft + margin, boardRight - margin);
					ex = std::clamp(ex, _boardLeft + margin, boardRight - margin);
					sy = std::clamp(sy, _boardTop + margin, boardBottom - margin);
					ey = std::clamp(ey, _boardTop + margin, boardBottom - margin);

					float winStroke = std::max(3.0f, static_cast<float>(cell * 0.12));
					td::ColorID lineColor = (_game.getWinner() == _humanPlayer) ? winLine : loseLine;
					gui::Shape::drawLine(gui::Point(sx, sy), gui::Point(ex, ey), lineColor, winStroke);
				}
			}

			// Draw game over text in the bottom row center column
			gui::Rect overlayRect(rect.left + col1Width, row3Top, rect.left + 2.0 * col1Width, row3Top + row3Height);

			td::String overlayLbl;
			if (_game.getWinner() == _humanPlayer)
				overlayLbl = tr("youWin");
			else if (_game.getWinner() == _AIrole)
				overlayLbl = tr("youLose");
			else
				overlayLbl = tr("draw");

			td::ColorID overlayTxtColor;
			if (_game.getWinner() == _humanPlayer)
				overlayTxtColor = winLine;
			else if (_game.getWinner() == _AIrole)
				overlayTxtColor = loseLine;
			else
				overlayTxtColor = drawLine;

			gui::Font overlayFont;
			float overlayFontSize = std::max(6.0f, static_cast<float>(row3Height * 0.35));
			overlayFont.create(CONNECTXO_UI_FONT, overlayFontSize, gui::Font::Style::BoldItalic, gui::Font::Unit::Point);
			gui::DrawableString::draw(overlayLbl, overlayRect, &overlayFont, overlayTxtColor, td::TextAlignment::Center, td::VAlignment::Center);
		}

		// Continue animation while falling or showing hint
		if (_isFalling || _isHintActive)
		{
			reDraw();
		}
	}

	void onCursorMoved(const gui::InputDevice& inputDevice) override
	{
		const gui::Point& pt = inputDevice.getModelPoint();

		// Helper to test point in rect
		auto pointInRect = [](const gui::Rect& r, const gui::Point& p) {
			return (p.x >= r.left) && (p.x <= r.right) && (p.y >= r.top) && (p.y <= r.bottom);
			};

		// Check button hover states
		bool wasQuitHovered = _quitBtnHovered;
		bool wasReplayHovered = _replayBtnHovered;
		bool wasHintHovered = _hintBtnHovered;

		_quitBtnHovered = pointInRect(_quitBtn, pt);
		_replayBtnHovered = pointInRect(_replayBtn, pt);
		const bool hintEnabled = isHintEnabled();
		_hintBtnHovered = hintEnabled && pointInRect(_hintBtn, pt);

		// Redraw if button hover state changed
		if (_quitBtnHovered != wasQuitHovered || _replayBtnHovered != wasReplayHovered || _hintBtnHovered != wasHintHovered)
		{
			reDraw();
		}

		// Early exit if hovering over buttons (don't update board hover)
		if (_quitBtnHovered || _replayBtnHovered || _hintBtnHovered)
		{
			clearHover();
			return;
		}

		if (_cellSize <= 0)
			return;
		if (_game.isGameOver() || _aiMoveScheduled || _isFalling)
		{
			clearHover();
			return;
		}
		if (_game.getCurrentPlayer() != _humanPlayer)
		{
			clearHover();
			return;
		}

		const bool inBoardX = (pt.x >= _boardLeft) && (pt.x <= _boardLeft + _cellSize * ConnectFour::WIDTH);
		const bool inBoardY = (pt.y >= _boardTop) && (pt.y <= _boardTop + _cellSize * ConnectFour::HEIGHT);
		if (!inBoardX || !inBoardY)
		{
			clearHover();
			return;
		}

		int col = static_cast<int>((pt.x - _boardLeft) / _cellSize);
		if (col < 0 || col >= ConnectFour::WIDTH)
		{
			clearHover();
			return;
		}

		int row = _game.getLowestEmptyRow(col);
		if (row < 0)
		{
			clearHover();
			return;
		}

		if (_hoverCol != col || _hoverRow != row)
		{
			_hoverCol = col;
			_hoverRow = row;
			_hoverFadeTime = 0.0f; // Reset fade timer when moving to new cell
			reDraw();
		}
		else if (_hoverCol >= 0 && _hoverRow >= 0)
		{
			// Update fade time while hovering
			auto now = std::chrono::steady_clock::now();
			static auto lastFadeTime = std::chrono::steady_clock::now();
			float dt = std::chrono::duration<float>(now - lastFadeTime).count();
			lastFadeTime = now;
			_hoverFadeTime = std::min(_hoverFadeTime + dt, 0.2f); // Cap at 0.2 seconds
			if (_hoverFadeTime >= 0.2f)
				reDraw(); // Only redraw when fade completes if needed
		}
	}

	void onPrimaryButtonReleased(const gui::InputDevice& inputDevice) override
	{
		// Early exit checks
		if (_cellSize <= 0) return; // Not yet drawn

		const gui::Point& pt = inputDevice.getModelPoint();

		// Helper to test point in rect
		auto pointInRect = [](const gui::Rect& r, const gui::Point& p) {
			return (p.x >= r.left) && (p.x <= r.right) && (p.y >= r.top) && (p.y <= r.bottom);
			};

		// Check Quit button first
		if (pointInRect(_quitBtn, pt))
		{
			// Quit: reset counters and notify parent
			_winCounter = 0;
			_lossCounter = 0;
			_countersUpdated = false;
			_isHintActive = false;
			_hintCol = -1;
			// Invalidate any pending AI moves
			++_aiGen;
			_aiMoveScheduled = false;
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

		// Check Replay button
		if (pointInRect(_replayBtn, pt))
		{
			// Replay: keep counters, reset game board
			++_aiGen; // Invalidate pending AI
			_aiMoveScheduled = false;
			// Toggle human/AI roles before starting new game
			_humanPlayer = (_humanPlayer == Player::X) ? Player::O : Player::X;
			if (_humanPlayer == Player::X)
				_AIrole = Player::O;
			else if (_humanPlayer == Player::O)
				_AIrole = Player::X;
			// Start a new game with X always starting
			_game.reset(Player::X);
			_countersUpdated = false;
			_isHintActive = false;
			_hintCol = -1;
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
								startFallingToken(aiMove, _game.getCurrentPlayer());
							}
						}
						_aiMoveScheduled = false;
						});
					gui::NatObject::asyncCall(fn, true);
					});
				aiStartThread.detach();
			}
			reDraw();
			return;
		}

		// Check Hint button
		if (pointInRect(_hintBtn, pt))
		{
			if (isHintEnabled())
			{
				int bestMove = calculateOptimalHint();
				if (bestMove >= 0 && bestMove < ConnectFour::WIDTH)
				{
					_hintCol = bestMove;
					_isHintActive = true;
					_hintStartTime = std::chrono::steady_clock::now();
					reDraw();
				}
			}
			return;
		}

		// Otherwise, handle board column selection (only if game is not over and it's human's turn)
		if (_game.isGameOver() || _aiMoveScheduled || _isFalling) return;
		if (_game.getCurrentPlayer() != _humanPlayer) return; // human's turn

		int col = static_cast<int>((pt.x - _boardLeft) / _cellSize);
		if (col < 0 || col >= ConnectFour::WIDTH) return;

		if (!startFallingToken(col, _game.getCurrentPlayer()))
			return;

		clearHover();
	}

private:
	struct FallingToken
	{
		float currentY = 0.0f;
		float targetY = 0.0f;
		float startY = 0.0f;
		int column = -1;
		Player type = Player::None;
		int targetRow = -1;
		float elapsed = 0.0f;
		float dropDuration = 0.0f;
		float bounceDuration = 0.0f;
		bool settled = false;
		bool soundPlayed = false;  // Ensure click sound plays only once when token settles
	};

	static constexpr double kTokenInsetScale = 0.12;

	gui::Rect getTokenRectForCell(int row, int col) const
	{
		const double l = _boardLeft + col * _cellSize;
		const double t = _boardTop + (ConnectFour::HEIGHT - 1 - row) * _cellSize;
		const double rgt = l + _cellSize;
		const double b = t + _cellSize;
		const double inset = _cellSize * kTokenInsetScale;
		return gui::Rect(l + inset, t + inset, rgt - inset, b - inset);
	}

	gui::Rect getTokenRectForColumnAtY(int col, float topY) const
	{
		const double inset = _cellSize * kTokenInsetScale;
		const double l = _boardLeft + col * _cellSize + inset;
		const double rgt = l + _cellSize - inset * 2.0;
		const double b = topY + _cellSize - inset * 2.0;
		return gui::Rect(l, topY, rgt, b);
	}

	void drawToken(const gui::Rect& pieceRect, Player p, bool dimmed, td::ColorID hoverOutline, float scale = 1.0f, float alpha = 1.0f)
	{
		gui::Rect drawRect = pieceRect;

		// Apply scale from center
		if (scale != 1.0f && scale > 0.0f)
		{
			const double centerX = (pieceRect.left + pieceRect.right) * 0.5;
			const double centerY = (pieceRect.top + pieceRect.bottom) * 0.5;
			const double halfW = (pieceRect.right - pieceRect.left) * 0.5 * scale;
			const double halfH = (pieceRect.bottom - pieceRect.top) * 0.5 * scale;
			drawRect = gui::Rect(centerX - halfW, centerY - halfH, centerX + halfW, centerY + halfH);
		}

		if (dimmed)
		{
			const double inset = _cellSize * 0.05;
			drawRect = gui::Rect(drawRect.left + inset, drawRect.top + inset, drawRect.right - inset, drawRect.bottom - inset);
		}

		const gui::Image& img = (p == Player::X) ? _imgToken1 : _imgToken2;
		if (img.isOK())
		{
			img.draw(drawRect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center, nullptr);
		}
		else
		{
			const td::ColorID fallback = (p == Player::X) ? td::ColorID::Blue : td::ColorID::Yellow;
			gui::Shape::drawRect(drawRect, dimmed ? td::ColorID::Gainsboro : fallback);
		}

		if (dimmed)
		{
			gui::Shape sh;
			sh.createOval(drawRect, 1.0f);
			const float stroke = std::max(1.0f, static_cast<float>(_cellSize * 0.05));
			sh.drawWire(hoverOutline, stroke);
		}
	}

	bool shouldDrawHover() const
	{
		return !_isFalling && !_aiMoveScheduled && !_game.isGameOver() && _game.getCurrentPlayer() == _humanPlayer && _hoverCol >= 0 && _hoverRow >= 0;
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
		return isHintFeatureEnabled() && !_isFalling && !_aiMoveScheduled && !_game.isGameOver() && _game.getCurrentPlayer() == _humanPlayer;
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

	void clearHover()
	{
		if (_hoverCol != -1 || _hoverRow != -1)
		{
			_hoverCol = -1;
			_hoverRow = -1;
			reDraw();
		}
	}

	bool startFallingToken(int col, Player type)
	{
		if (_cellSize <= 0)
			return false;
		int targetRow = _game.getLowestEmptyRow(col);
		if (targetRow < 0)
			return false;

		_fallingToken.column = col;
		_fallingToken.type = type;
		_fallingToken.targetRow = targetRow;
		_fallingToken.elapsed = 0.0f;
		_fallingToken.dropDuration = 0.0f;
		_fallingToken.bounceDuration = 0.0f;
		_fallingToken.settled = false;
		_fallingToken.soundPlayed = false;  // Reset sound flag for new token
		_animTimeValid = false;
		_fallingToken.startY = static_cast<float>(_boardTop - _cellSize);
		_fallingToken.currentY = _fallingToken.startY;
		_fallingToken.targetY = static_cast<float>(_boardTop + (ConnectFour::HEIGHT - 1 - targetRow) * _cellSize + _cellSize * kTokenInsetScale);
		{
			const float distance = std::max(1.0f, _fallingToken.targetY - _fallingToken.startY);
			const float base = static_cast<float>(_cellSize) * 10.0f;
			_fallingToken.dropDuration = std::clamp(distance / base, 0.24f, 0.48f);
			_fallingToken.bounceDuration = 0.18f;
		}
		_isFalling = true;
		startAnimation();
		reDraw();
		return true;
	}

	void updateFallingTarget(double cell)
	{
		_fallingToken.targetY = static_cast<float>(_boardTop + (ConnectFour::HEIGHT - 1 - _fallingToken.targetRow) * cell + cell * kTokenInsetScale);
		if (_fallingToken.currentY < _boardTop - cell)
			_fallingToken.currentY = static_cast<float>(_boardTop - cell);
	}

	void advanceFallingToken(double cell)
	{
		auto now = std::chrono::steady_clock::now();
		float dt = 1.0f / 60.0f;
		if (_animTimeValid)
		{
			dt = std::chrono::duration<float>(now - _lastAnimTime).count();
			dt = std::clamp(dt, 0.001f, 0.05f);
		}
		_lastAnimTime = now;
		_animTimeValid = true;

		_fallingToken.elapsed += dt;
		if (_fallingToken.elapsed <= _fallingToken.dropDuration)
		{
			float t = _fallingToken.elapsed / std::max(0.001f, _fallingToken.dropDuration);
			t = std::clamp(t, 0.0f, 1.0f);
			const float ease = t * t;
			_fallingToken.currentY = _fallingToken.startY + (_fallingToken.targetY - _fallingToken.startY) * ease;
			return;
		}

		const float bounceElapsed = _fallingToken.elapsed - _fallingToken.dropDuration;
		if (bounceElapsed <= _fallingToken.bounceDuration)
		{
			const float t = std::clamp(bounceElapsed / std::max(0.001f, _fallingToken.bounceDuration), 0.0f, 1.0f);
			const float amplitude = static_cast<float>(cell) * 0.08f * (1.0f - t);
			const float pi = 3.1415926f;
			_fallingToken.currentY = _fallingToken.targetY - amplitude * std::sin(t * pi);
			return;
		}

		_fallingToken.currentY = _fallingToken.targetY;
		_fallingToken.settled = true;

		// Play falling sound precisely when token settles (only once per drop)
		if (!_fallingToken.soundPlayed && _playSound)
		{
			_fallingToken.soundPlayed = true;
			_fallingSound.play();
		}
	}

	void finalizeFallingToken()
	{
		_isFalling = false;
		stopAnimation();

		// Sound already played in animate() when token settled
		_game.makeMove(_fallingToken.column);

		if (_game.isGameOver() && !_countersUpdated)
		{
			Player winner = _game.getWinner();
			if (winner == _humanPlayer)
			{
				++_winCounter;
				++ScoreManager::getInstance().getC4Stats().wins;
				ScoreManager::getInstance().saveC4Stats();
			}
			else if (winner == _AIrole)
			{
				++_lossCounter;
				++ScoreManager::getInstance().getC4Stats().losses;
				ScoreManager::getInstance().saveC4Stats();
			}
			_countersUpdated = true;
		}

		reDraw();
		if (_game.isGameOver())
		{
			_aiMoveScheduled = false;
			return;
		}

		if (_fallingToken.type == _humanPlayer)
		{
			scheduleAIMove();
			return;
		}

		_aiMoveScheduled = false;
	}

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
					if (!startFallingToken(aiMove, _game.getCurrentPlayer()))
						_aiMoveScheduled = false;
				}
				else
				{
					_aiMoveScheduled = false;
				}
				});
			gui::NatObject::asyncCall(fn, true);
			});
		aiThread.detach();
	}

	ConnectFour _game;
	AIPlayer _aiPlayer;
	bool _playSound = true;
	gui::Image _imgToken1;
	gui::Image _imgToken2;
	gui::Image _imgWood;
	gui::Sound _fallingSound;
	double _boardLeft, _boardTop, _cellSize;
	bool _aiMoveScheduled;
	bool _isFalling;
	FallingToken _fallingToken;
	int _hoverCol;
	int _hoverRow;
	bool _quitBtnHovered;
	bool _replayBtnHovered;

	int _winCounter;
	int _lossCounter;
	bool _countersUpdated;

	// Button rects for click detection
	gui::Rect _replayBtn;
	gui::Rect _quitBtn;

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
	std::chrono::steady_clock::time_point _lastAnimTime;
	bool _animTimeValid;

	// Hint system
	bool _isHintActive = false;
	int _hintCol = -1;
	std::chrono::steady_clock::time_point _hintStartTime;

	// Hover fade animation
	float _hoverFadeTime = 0.0f;

	// Win pulse animation
	std::chrono::steady_clock::time_point _winPulseStartTime;

	// Hint button
	gui::Rect _hintBtn;
	bool _hintBtnHovered = false;
};