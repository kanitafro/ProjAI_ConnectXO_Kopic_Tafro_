#pragma once
#include <memory>
#include <vector>

enum class Player
{
    None,
    X,
    O
};

class Game
{
public:
    using Move = int;
    virtual ~Game() = default;

    virtual std::unique_ptr<Game> clone() const = 0;
    virtual std::vector<Move> getValidMoves() const = 0;
    virtual Player checkWin() const = 0;
    virtual bool isDraw() const = 0;

    Player getCurrentPlayer() const { return _currentPlayer; }
    Player getWinner() const { return _winner; }
    bool isGameOver() const { return _gameOver; }

    bool makeMove(Move move)
    {
        if (_gameOver)
            return false;

        if (!placeMove(_currentPlayer, move))
            return false;

        _winner = checkWin();
        if (_winner != Player::None)
        {
            _gameOver = true;
            return true;
        }

        if (isDraw())
        {
            _gameOver = true;
            return true;
        }

        switchPlayer();
        return true;
    }

    void reset(Player startingPlayer = Player::X)
    {
        _currentPlayer = startingPlayer;
        _winner = Player::None;
        _gameOver = false;
        clearBoard();
    }

protected:
    virtual bool placeMove(Player player, Move move) = 0;
    virtual void clearBoard() = 0;

    void switchPlayer()
    {
        _currentPlayer = (_currentPlayer == Player::X) ? Player::O : Player::X;
    }

    Player _currentPlayer{ Player::X };
    Player _winner{ Player::None };
    bool _gameOver{ false };
};

class AIPlayer
{
public:
    // Returns the chosen move for the current player of the provided game state.
    Game::Move chooseMove(const Game& game, int maxDepth = 8);
};
