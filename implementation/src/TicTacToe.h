#pragma once
#include "AIPlayer.h"
#include <array>
#include <vector>

class TicTacToe : public Game
{
public:
    TicTacToe() : _board{} {}

    std::unique_ptr<Game> clone() const override
    {
        auto copy = std::make_unique<TicTacToe>();
        copy->_board = _board;
        copy->_currentPlayer = _currentPlayer;
        copy->_winner = _winner;
        copy->_gameOver = _gameOver;
        return copy;
    }

    std::vector<Move> getValidMoves() const override
    {
        std::vector<Move> validMoves;
        for (int i = 0; i < 9; ++i)
        {
            if (_board[i] == Player::None)
                validMoves.push_back(i);
        }
        return validMoves;
    }

    Player checkWin() const override
    {
        // Winning lines: 3 rows, 3 columns, 2 diagonals
        const int winningLines[8][3] = {
            // Rows
            {0, 1, 2},
            {3, 4, 5},
            {6, 7, 8},
            // Columns
            {0, 3, 6},
            {1, 4, 7},
            {2, 5, 8},
            // Diagonals
            {0, 4, 8},
            {2, 4, 6}
        };

        for (const auto& line : winningLines)
        {
            if (_board[line[0]] != Player::None &&
                _board[line[0]] == _board[line[1]] &&
                _board[line[1]] == _board[line[2]])
            {
                return _board[line[0]];
            }
        }

        return Player::None;
    }

    bool isDraw() const override
    {
        for (const auto& cell : _board)
        {
            if (cell == Player::None)
                return false;
        }
        return true;
    }

    // Getter for board cell at index (for UI rendering)
    Player getCell(int index) const
    {
        if (index < 0 || index >= 9)
            return Player::None;
        return _board[index];
    }

protected:
    bool placeMove(Player player, Move move) override
    {
        if (move < 0 || move > 8)
            return false;

        if (_board[move] != Player::None)
            return false;

        _board[move] = player;
        return true;
    }

    void clearBoard() override
    {
        _board.fill(Player::None);
    }

private:
    std::array<Player, 9> _board;
};
