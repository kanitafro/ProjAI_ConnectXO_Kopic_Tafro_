#include "AIPlayer.h"
#include <limits>
#include <memory>
#include <cmath>
#include <random>
#include "ConnectFour.h"
#include "TicTacToe.h"
#include "Theme.h"

namespace
{
	//0 = Very Easy, 1 = Easy, 2 = Medium, 3 = Hard, 4 = Very Hard
	static int g_aiDifficultyIndex = 2; // default to Medium
	// Theme index:0=Classic,1=Nature,2=Strawberry,3=Beachy,4=Dark
	static ThemeIndex g_themeIndex = ThemeIndex::Classic; // default to Classic

	double difficultyFactor()
	{
		switch (g_aiDifficultyIndex)
		{
		case 0: return 0.30; // Very Easy
		case 1: return 0.45; // Easy
		case 2: return 0.65; // Medium
		case 3: return 0.80; // Hard
		case 4: // fallthrough
		default: return 1.0; // Very Hard
		}
	}

	bool isBoardEmpty(const Game& game)
	{
		if (auto c4 = dynamic_cast<const ConnectFour*>(&game))
		{
			for (int row = 0; row < ConnectFour::HEIGHT; ++row)
			{
				for (int col = 0; col < ConnectFour::WIDTH; ++col)
				{
					if (c4->getCell(row, col) != Player::None)
						return false;
				}
			}
			return true;
		}

		if (auto ttt = dynamic_cast<const TicTacToe*>(&game))
		{
			for (int i = 0; i < 9; ++i)
			{
				if (ttt->getCell(i) != Player::None)
					return false;
			}
			return true;
		}

		return false;
	}

	Game::Move choosePreferredMove(const Game& game, const std::vector<Game::Move>& bestMoves)
	{
		if (auto c4 = dynamic_cast<const ConnectFour*>(&game))
		{
			const int centerPreference[] = {3, 2, 4, 1, 5, 0, 6};
			for (int col : centerPreference)
			{
				for (auto move : bestMoves)
				{
					if (move == col)
						return move;
				}
			}
		}

		if (auto ttt = dynamic_cast<const TicTacToe*>(&game))
		{
			const int priorityOrder[] = {4, 0, 2, 6, 8, 1, 3, 5, 7};
			for (int pos : priorityOrder)
			{
				for (auto move : bestMoves)
				{
					if (move == pos)
						return move;
				}
			}
		}

		return bestMoves.front();
	}
}

// Setter used by the UI to change difficulty at runtime
extern "C" void setAIDifficultyIndex(int idx)
{
	if (idx < 0) idx = 0;
	if (idx > 4) idx = 4;
	g_aiDifficultyIndex = idx;
}

extern "C" int getAIDifficultyIndex()
{
	return g_aiDifficultyIndex;
}

// Theme setter/getter (used at startup to apply persisted theme before views are created)
extern "C" void setThemeIndex(int idx)
{
	g_themeIndex = clampThemeIndex(idx);
}

extern "C" int getThemeIndex()
{
	return to_int(g_themeIndex);
}

int evaluateHeuristic(const Game& state, Player aiPlayer)
{
	if (auto c4 = dynamic_cast<const ConnectFour*>(&state))
	{
		int score = 0;
		Player opponent = (aiPlayer == Player::X) ? Player::O : Player::X;

		// Scan for threats and opportunities (sequences of 2 and 3)
		for (int row = 0; row < ConnectFour::HEIGHT; ++row)
		{
			for (int col = 0; col < ConnectFour::WIDTH; ++col)
			{
				Player p = c4->getCell(row, col);
				if (p == Player::None) continue;

				// Horizontal sequences
				if (col + 2 < ConnectFour::WIDTH)
				{
					Player p1 = c4->getCell(row, col + 1);
					Player p2 = c4->getCell(row, col + 2);
					
					// 3-in-a-row: critical threat
					if (p == p1 && p1 == p2)
					{
						if (p == aiPlayer)
							score += 50;   // AI winning opportunity
						else
							score -= 50;   // Opponent threat
					}
					// 2-in-a-row
					else if (p == p1 && p2 == Player::None)
					{
						if (p == aiPlayer)
							score += 10;
						else
							score -= 10;
					}
				}

				// Vertical sequences
				if (row + 2 < ConnectFour::HEIGHT)
				{
					Player p1 = c4->getCell(row + 1, col);
					Player p2 = c4->getCell(row + 2, col);
					
					if (p == p1 && p1 == p2)
					{
						if (p == aiPlayer)
							score += 50;
						else
							score -= 50;
					}
					else if (p == p1 && p2 == Player::None)
					{
						if (p == aiPlayer)
							score += 10;
						else
							score -= 10;
					}
				}

				// Diagonal (down-right)
				if (col + 2 < ConnectFour::WIDTH && row + 2 < ConnectFour::HEIGHT)
				{
					Player p1 = c4->getCell(row + 1, col + 1);
					Player p2 = c4->getCell(row + 2, col + 2);
					
					if (p == p1 && p1 == p2)
					{
						if (p == aiPlayer)
							score += 50;
						else
							score -= 50;
					}
					else if (p == p1 && p2 == Player::None)
					{
						if (p == aiPlayer)
							score += 10;
						else
							score -= 10;
					}
				}

				// Diagonal (down-left)
				if (col - 2 >= 0 && row + 2 < ConnectFour::HEIGHT)
				{
					Player p1 = c4->getCell(row + 1, col - 1);
					Player p2 = c4->getCell(row + 2, col - 2);
					
					if (p == p1 && p1 == p2)
					{
						if (p == aiPlayer)
							score += 50;
						else
							score -= 50;
					}
					else if (p == p1 && p2 == Player::None)
					{
						if (p == aiPlayer)
							score += 10;
						else
							score -= 10;
					}
				}
			}
		}

		// Bonus for center positions (helps with strategy when no threats exist)
		for (int row = 0; row < ConnectFour::HEIGHT; ++row)
		{
			Player p = c4->getCell(row, 3);
			if (p == aiPlayer)
				score += 2;
			else if (p == opponent)
				score -= 2;
		}

		return score;
	}

	if (auto ttt = dynamic_cast<const TicTacToe*>(&state))
	{
		int score = 0;
		Player opponent = (aiPlayer == Player::X) ? Player::O : Player::X;

		// Check for 2-in-a-row threats
		const int winningLines[8][3] = {
			{0, 1, 2}, {3, 4, 5}, {6, 7, 8},
			{0, 3, 6}, {1, 4, 7}, {2, 5, 8},
			{0, 4, 8}, {2, 4, 6}
		};

		for (const auto& line : winningLines)
		{
			int aiCount = 0, oppCount = 0;
			for (int pos : line)
			{
				if (ttt->getCell(pos) == aiPlayer) aiCount++;
				else if (ttt->getCell(pos) == opponent) oppCount++;
			}

			if (aiCount == 2 && oppCount == 0)
				score += 50;  // AI can win
			else if (oppCount == 2 && aiCount == 0)
				score -= 50;  // Must block opponent
			else if (aiCount == 1 && oppCount == 0)
				score += 5;
			else if (oppCount == 1 && aiCount == 0)
				score -= 5;
		}

		// Center position bonus for Tic Tac Toe
		if (ttt->getCell(4) == aiPlayer)
			score += 3;
		else if (ttt->getCell(4) == opponent)
			score -= 3;

		return score;
	}

	return 0;
}

int evaluateTerminal(const Game& state, Player aiPlayer, int depthRemaining)
{
	const Player winner = state.getWinner();
	if (winner == aiPlayer)
		return 1000 + depthRemaining; // prefer faster wins
	if (winner != Player::None)
		return -1000 - depthRemaining; // prefer slower losses
	if (state.isDraw())
		return 0;
	
	// If game is not over, use heuristic evaluation
	return evaluateHeuristic(state, aiPlayer);
}

int minimax(std::unique_ptr<Game> node, int depth, bool maximizing, int alpha, int beta, Player aiPlayer)
{
	if (depth == 0 || node->isGameOver())
		return evaluateTerminal(*node, aiPlayer, depth);

	const auto moves = node->getValidMoves();
	if (moves.empty())
		return evaluateTerminal(*node, aiPlayer, depth);

	if (maximizing)
	{
		int best = std::numeric_limits<int>::min();
		for (auto move : moves)
		{
			auto next = node->clone();
			if (!next->makeMove(move))
				continue;

			const int score = minimax(std::move(next), depth - 1, false, alpha, beta, aiPlayer);
			if (score > best)
				best = score;

			if (score > alpha)
				alpha = score;

			if (beta <= alpha)
				break;
		}
		return best;
	}
	else
	{
		int best = std::numeric_limits<int>::max();
		for (auto move : moves)
		{
			auto next = node->clone();
			if (!next->makeMove(move))
				continue;

			const int score = minimax(std::move(next), depth - 1, true, alpha, beta, aiPlayer);
			if (score < best)
				best = score;

			if (score < beta)
				beta = score;

			if (beta <= alpha)
				break;
		}
		return best;
	}
}

Game::Move AIPlayer::chooseMove(const Game& game, int maxDepth)
{
	if (maxDepth < 1)
		maxDepth = 1;

	// Compute effective search depth based on difficulty
	const double factor = difficultyFactor();
	int effectiveDepth = static_cast<int>(std::round(maxDepth * factor));
	if (effectiveDepth < 1)
		effectiveDepth = 1;

	// The existing logic calls minimax with (depth -1) after making the candidate move
	const int minimaxDepth = std::max(1, effectiveDepth);

	const Player aiPlayer = game.getCurrentPlayer();
	const auto moves = game.getValidMoves();
	if (moves.empty())
		return -1;

	int bestScore = std::numeric_limits<int>::min();
	std::vector<Game::Move> bestMoves;
	int alpha = std::numeric_limits<int>::min();
	int beta = std::numeric_limits<int>::max();

	for (auto move : moves)
	{
		auto next = game.clone();
		if (!next->makeMove(move))
			continue;

		const int score = minimax(std::move(next), minimaxDepth - 1, false, alpha, beta, aiPlayer);
		if (score > bestScore)
		{
			bestScore = score;
			bestMoves.clear();
			bestMoves.push_back(move);
		}
		else if (score == bestScore)
		{
			bestMoves.push_back(move);
		}

		if (score > alpha)
			alpha = score;
	}

	if (bestMoves.empty())
		return moves.front();

	// Randomize only the opening move for variety, then stay deterministic
	if (bestMoves.size() > 1 && isBoardEmpty(game))
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, static_cast<int>(bestMoves.size()) - 1);
		return bestMoves[dis(gen)];
	}

	return choosePreferredMove(game, bestMoves);
}