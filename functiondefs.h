#ifndef FUNCTION_DEFS_H
#define FUNCTION_DEFS_H

#include "chess.h"

Rectangle tileRect(int row, int col);

Point screenToBoard(int screenX, int screenY);

void drawPiece(const Textures& tex, const Piece& piece, int row, int col);

void drawBoard(const Textures& tex, const Board& board, std::optional<Point> selected, const std::vector<Point>& highlights);

std::optional<Piece::Type> drawPromotionPanel(const Textures& tex, Piece::Color color);

void drawGameOverOverlay(const std::string& message, bool& savePGNClicked);

void drawStatusBar(const std::string& msg);


std::string getPositionKey(const Board& board);

bool squaresAttacked(const Board& board, int row, int col1, int col2, Piece::Color byEnemy);

bool tryCastle(Board& board, Piece::Color color, Move::Castle castleType);

bool hasLegalMove(Board& board, Piece::Color color);

std::optional<bool> tryMove(Board& board, Point from, Point to);

void savePGN(const Board& board, const std::string& result);

std::vector<Point> legalDestinations(Board& board, Point from);



#endif