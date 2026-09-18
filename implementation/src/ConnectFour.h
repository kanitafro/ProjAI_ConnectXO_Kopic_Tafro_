#pragma once
#include "AIPlayer.h"
#include <array>
#include <vector>

class ConnectFour : public Game
{
public:
 static constexpr int WIDTH =7;
 static constexpr int HEIGHT =6;
 static constexpr int CELLS = WIDTH * HEIGHT;

 ConnectFour() { clearBoard(); }

 std::unique_ptr<Game> clone() const override
 {
 auto copy = std::make_unique<ConnectFour>();
 copy->_board = _board;
 copy->_currentPlayer = _currentPlayer;
 copy->_winner = _winner;
 copy->_gameOver = _gameOver;
 return copy;
 }

 std::vector<Move> getValidMoves() const override
 {
 std::vector<Move> moves;
 for (int col =0; col < WIDTH; ++col)
 {
 // top cell (highest row) empty means column can accept a token
 if (_board[index(HEIGHT -1, col)] == Player::None)
 moves.push_back(col);
 }
 return moves;
 }

 Player checkWin() const override
 {
 // Scan entire board for any4-in-a-row starting at each occupied cell.
 for (int row =0; row < HEIGHT; ++row)
 {
 for (int col =0; col < WIDTH; ++col)
 {
 Player p = _board[index(row, col)];
 if (p == Player::None) continue;

 // horizontal (right)
 if (col +3 < WIDTH)
 {
 bool ok = true;
 for (int k =1; k <4; ++k)
 if (_board[index(row, col + k)] != p) { ok = false; break; }
 if (ok) return p;
 }

 // vertical (up)
 if (row +3 < HEIGHT)
 {
 bool ok = true;
 for (int k =1; k <4; ++k)
 if (_board[index(row + k, col)] != p) { ok = false; break; }
 if (ok) return p;
 }

 // diagonal up-right
 if (col +3 < WIDTH && row +3 < HEIGHT)
 {
 bool ok = true;
 for (int k =1; k <4; ++k)
 if (_board[index(row + k, col + k)] != p) { ok = false; break; }
 if (ok) return p;
 }

 // diagonal up-left
 if (col -3 >=0 && row +3 < HEIGHT)
 {
 bool ok = true;
 for (int k =1; k <4; ++k)
 if (_board[index(row + k, col - k)] != p) { ok = false; break; }
 if (ok) return p;
 }
 }
 }
 return Player::None;
 }

 bool isDraw() const override
 {
 // If any top cell is empty, not a draw
 for (int col =0; col < WIDTH; ++col)
 if (_board[index(HEIGHT -1, col)] == Player::None)
 return false;
 return true;
 }

 // row0 is the bottom row; row increases upwards to HEIGHT-1
 Player getCell(int row, int col) const
 {
 if (!inBounds(row, col)) return Player::None;
 return _board[index(row, col)];
 }

 // Returns lowest empty row for a column, or -1 if column is full/invalid.
 int getLowestEmptyRow(int col) const
 {
 if (col < 0 || col >= WIDTH)
 return -1;
 for (int row = 0; row < HEIGHT; ++row)
 {
 if (_board[index(row, col)] == Player::None)
 return row;
 }
 return -1;
 }

protected:
 // Move is interpreted as column index (0..WIDTH-1). Token falls to lowest available row.
 bool placeMove(Player player, Move move) override
 {
 if (move <0 || move >= WIDTH) return false;
 for (int row =0; row < HEIGHT; ++row)
 {
 if (_board[index(row, move)] == Player::None)
 {
 _board[index(row, move)] = player;
 return true;
 }
 }
 return false; // column full
 }

 void clearBoard() override
 {
 _board.fill(Player::None);
 }

private:
 std::array<Player, CELLS> _board;

 inline int index(int row, int col) const { return row * WIDTH + col; }
 inline bool inBounds(int row, int col) const { return row >=0 && row < HEIGHT && col >=0 && col < WIDTH; }
};
