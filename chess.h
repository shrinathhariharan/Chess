#ifndef CHESS_H
#define CHESS_H

#include "raylib.h"

#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <array>
#include <cmath>

namespace Settings
{
    inline constexpr int boardSize{8};
    inline constexpr int tileSize{80};
    inline constexpr int boardOffset{40};   //left/top margin so labels fit
    inline constexpr int windowSize{boardSize * tileSize + boardOffset * 2};
    inline constexpr int statusBarHeight{50};

    inline const Font chessFont{LoadFont("Font/TheQueen-Regular.ttf")};

    inline const std::string pgnFilePath{"Images/game.pgn"};
    inline const std::string boardFile{"Images/chess_board.jpeg"};

    inline const std::string blackKingFile{"Images/black_chess_king.png"};
    inline const std::string blackKnightFile{"Images/black_chess_knight.png"};
    inline const std::string blackBishopFile{"Images/black_chess_bishop.png"};
    inline const std::string blackQueenFile{"Images/black_chess_queen.png"};
    inline const std::string blackRookFile{"Images/black_chess_rook.png"};
    inline const std::string blackPawnFile{"Images/black_chess_pawn.png"};

    inline const std::string whiteKingFile{"Images/white_chess_king.png"};
    inline const std::string whiteKnightFile{"Images/white_chess_knight.png"};
    inline const std::string whiteBishopFile{"Images/white_chess_bishop.png"};
    inline const std::string whiteQueenFile{"Images/white_chess_queen.png"};
    inline const std::string whiteRookFile{"Images/white_chess_rook.png"};
    inline const std::string whitePawnFile{"Images/white_chess_pawn.png"};
}

struct Point
{
    int row{};
    int col{};

    bool outOfBounds() const
    {
        return row < 0 || row >= Settings::boardSize
            || col < 0 || col >= Settings::boardSize;
    }
};

inline Point operator+(Point a, Point b) { return {a.row + b.row, a.col + b.col}; }
inline Point operator*(Point a, int b)   { return {a.row * b,     a.col * b}; }
inline bool  operator==(Point a, Point b){ return a.row == b.row && a.col == b.col; }

namespace Settings
{
    inline constexpr Point diagionalMovements[]{{1,1},{1,-1},{-1,1},{-1,-1}};
    inline constexpr Point verticalMovements[] {{1,0},{-1,0},{0,1},{0,-1}};
}

class Board;

class Piece
{
public:
    enum Type  { pawn, knight, bishop, rook, queen, king };
    enum Color { white, black };

protected:
    Color m_color{};
    Type  m_type{};
    Point m_point{};

public:
    Piece(Color c, Type t, Point p) : m_color{c}, m_type{t}, m_point{p} {}
    virtual ~Piece() {}

    virtual bool isValidMove(Point newPoint, const Board& board) const = 0;
    virtual char getSymbol() const = 0;

    Color getColor() const { return m_color; }
    Type  getType()  const { return m_type; }
    Point getPoint() const { return m_point; }
    void  setPoint(Point p){ m_point = p; }
};

inline constexpr char getPieceChar(Piece::Type type)
{
    switch (type)
    {
    case Piece::knight: return 'N';
    case Piece::bishop: return 'B';
    case Piece::rook:   return 'R';
    case Piece::queen:  return 'Q';
    case Piece::king:   return 'K';
    default:            return '?';
    }
}

struct Move
{
    enum Castle { castle_kingside, castle_queenside };
    enum Attack { check, checkmate };

    Piece::Type type{};
    std::optional<Castle> castleType{std::nullopt};
    std::optional<Attack> attackType{std::nullopt};

    char origCol{};
    int  origRow{};
    char newCol{};
    int  newRow{};

    Point getNewPoint() const { return Point{newRow - 1, static_cast<int>(newCol - 'a')}; }
    Point getOldPoint() const { return Point{origRow - 1, static_cast<int>(origCol - 'a')}; }

    std::string getString() const
    {
        if (castleType)
            return (*castleType == castle_kingside ? "O-O" : "O-O-O");
        std::string s{};
        s.reserve(6);
        if (type != Piece::pawn) s += getPieceChar(type);
        s += newCol;
        s += std::to_string(newRow);
        if (attackType) s += (*attackType == check ? '+' : '#');
        return s;
    }
};

class Board
{
    std::array<std::array<std::unique_ptr<Piece>, Settings::boardSize>, Settings::boardSize> m_board{};

    int  m_currentMove{1};
    int  m_halfMoveClock{0};

    Point m_enPassantTarget{-1, -1};
    bool  m_hasEnPassantTarget{false};

    std::unordered_map<std::string, int> m_positions{};
    std::vector<std::string> m_pgnMoves{};

public:
    Board();

    const Piece* getPieceAt(Point p) const
    {
        if (p.outOfBounds()) return nullptr;
        return m_board[p.row][p.col].get();
    }
    Piece* getPieceAt(Point p)
    {
        if (p.outOfBounds()) return nullptr;
        return m_board[p.row][p.col].get();
    }
    std::unique_ptr<Piece> takePieceAt(Point p)
    {
        if (p.outOfBounds()) return nullptr;
        return std::move(m_board[p.row][p.col]);
    }
    void placePiece(std::unique_ptr<Piece> piece, Point p) { m_board[p.row][p.col] = std::move(piece); }
    void clearSquare(Point p) { if (!p.outOfBounds()) m_board[p.row][p.col] = nullptr; }

    void nextMove()   { ++m_currentMove; }
    int  getCurrentMove() const { return m_currentMove; }
    Piece::Color getCurrentTurnColor() const
    {
        return (m_currentMove % 2 == 1) ? Piece::white : Piece::black;
    }

    void incrementHalfMoveClock() { ++m_halfMoveClock; }
    void resetHalfMoveClock()     { m_halfMoveClock = 0; }
    int  getHalfMoveClock() const { return m_halfMoveClock; }

    void  setEnPassantTarget(Point p) { m_enPassantTarget = p; m_hasEnPassantTarget = true; }
    void  clearEnPassantTarget()      { m_hasEnPassantTarget = false; }
    bool  hasEnPassantTarget() const  { return m_hasEnPassantTarget; }
    Point getEnPassantTarget() const  { return m_enPassantTarget; }

    void recordPosition(const std::string& key) { ++m_positions[key]; }
    int  getPositionCount(const std::string& key) const
    {
        auto it{m_positions.find(key)};
        return (it != m_positions.end()) ? it->second : 0;
    }

    void addMove(const std::string& s) { m_pgnMoves.push_back(s); }
    void addMove(const Move& m)        { m_pgnMoves.push_back(m.getString()); }
    std::size_t getPGNMoveCount() const { return m_pgnMoves.size(); }
    const std::string& operator[](std::size_t i) const { return m_pgnMoves[i]; }

    Point findKing(Piece::Color color) const;
    bool  isInCheck(Piece::Color color) const;
    bool  insufficientMaterial() const;

    auto begin() const { return m_board.begin(); }
    auto end()   const { return m_board.end(); }
};

class Pawn : public Piece
{
    bool m_hasMoved{false};
public:
    Pawn(Color c, Point p) : Piece{c, Piece::pawn, p} {}
    bool isValidMove(Point newPoint, const Board& board) const override;
    char getSymbol() const override { return 'P'; }
    void markMoved() { m_hasMoved = true; }
    bool hasMoved()  const { return m_hasMoved; }
};

class Knight : public Piece
{
public:
    Knight(Color c, Point p) : Piece{c, Piece::knight, p} {}
    bool isValidMove(Point newPoint, const Board& board) const override;
    char getSymbol() const override { return 'N'; }
};

class Bishop : public Piece
{
public:
    Bishop(Color c, Point p) : Piece{c, Piece::bishop, p} {}
    bool isValidMove(Point newPoint, const Board& board) const override;
    char getSymbol() const override { return 'B'; }
};

class Rook : public Piece
{
public:
    enum Side { kingside, queenside };
private:
    bool m_hasMoved{false};
    Side m_side{};
public:
    Rook(Color c, Point p, Side side) : Piece{c, Piece::rook, p}, m_side{side} {}
    bool isValidMove(Point newPoint, const Board& board) const override;
    char getSymbol() const override { return 'R'; }
    bool hasMoved()  const { return m_hasMoved; }
    void markMoved()       { m_hasMoved = true; }
    Side getSide()   const { return m_side; }
};

class Queen : public Piece
{
public:
    Queen(Color c, Point p) : Piece{c, Piece::queen, p} {}
    bool isValidMove(Point newPoint, const Board& board) const override;
    char getSymbol() const override { return 'Q'; }
};

class King : public Piece
{
    bool m_hasMoved{false};
public:
    King(Color c, Point p) : Piece{c, Piece::king, p} {}
    bool isValidMove(Point newPoint, const Board& board) const override;
    char getSymbol() const override { return 'K'; }
    bool hasMoved()  const { return m_hasMoved; }
    void markMoved()       { m_hasMoved = true; }
};

struct Textures
{
    Texture2D board{};
    Texture2D pieces[2][6]{};

    void load()
    {
        board = LoadTexture(Settings::boardFile.c_str());

        pieces[Piece::white][Piece::pawn]   = LoadTexture(Settings::whitePawnFile.c_str());
        pieces[Piece::white][Piece::knight] = LoadTexture(Settings::whiteKnightFile.c_str());
        pieces[Piece::white][Piece::bishop] = LoadTexture(Settings::whiteBishopFile.c_str());
        pieces[Piece::white][Piece::rook]   = LoadTexture(Settings::whiteRookFile.c_str());
        pieces[Piece::white][Piece::queen]  = LoadTexture(Settings::whiteQueenFile.c_str());
        pieces[Piece::white][Piece::king]   = LoadTexture(Settings::whiteKingFile.c_str());

        pieces[Piece::black][Piece::pawn]   = LoadTexture(Settings::blackPawnFile.c_str());
        pieces[Piece::black][Piece::knight] = LoadTexture(Settings::blackKnightFile.c_str());
        pieces[Piece::black][Piece::bishop] = LoadTexture(Settings::blackBishopFile.c_str());
        pieces[Piece::black][Piece::rook]   = LoadTexture(Settings::blackRookFile.c_str());
        pieces[Piece::black][Piece::queen]  = LoadTexture(Settings::blackQueenFile.c_str());
        pieces[Piece::black][Piece::king]   = LoadTexture(Settings::blackKingFile.c_str());
    }

    void unload()
    {
        UnloadTexture(board);
        for (int c{0}; c < 2; ++c)
            for (int t{0}; t < 6; ++t)
                UnloadTexture(pieces[c][t]);
    }
};

#endif