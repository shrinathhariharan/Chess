#include "chess.h"

bool Pawn::isValidMove(Point newPoint, const Board& board) const
{
    if (newPoint.outOfBounds()) return false;
    const Point oldPoint{m_point};
    const int   dir{(m_color == Piece::white) ? 1 : -1};
    const Piece::Color enemy{(m_color == Piece::white) ? Piece::black : Piece::white};
    const int rowDiff{newPoint.row - oldPoint.row};
    const int colDiff{std::abs(newPoint.col - oldPoint.col)};
    const Piece* target{board.getPieceAt(newPoint)};

    if (colDiff == 0)
    {
        if (target) return false;
        if (rowDiff == dir) return true;
        if (rowDiff == 2 * dir && !m_hasMoved)
            return !board.getPieceAt(Point{oldPoint.row + dir, oldPoint.col});
        return false;
    }
    if (colDiff == 1 && rowDiff == dir)
    {
        if (target && target->getColor() == enemy) return true;
        if (!target && board.hasEnPassantTarget())
            return board.getEnPassantTarget() == newPoint;
    }
    return false;
}

bool Knight::isValidMove(Point newPoint, const Board& board) const
{
    if (newPoint.outOfBounds()) return false;
    static constexpr std::array<Point, 8> deltas{{{2,1},{2,-1},{1,2},{1,-2},{-2,1},{-2,-1},{-1,2},{-1,-2}}};
    for (const auto& d : deltas)
        if (m_point + d == newPoint)
        {
            const Piece* dest{board.getPieceAt(newPoint)};
            return !dest || dest->getColor() != m_color;
        }
    return false;
}

bool Bishop::isValidMove(Point newPoint, const Board& board) const
{
    if (newPoint.outOfBounds()) return false;
    for (const auto& dir : Settings::diagionalMovements)
    {
        int scale{1}; Point cur{};
        while (!(cur = m_point + dir * scale++).outOfBounds())
        {
            const Piece* here{board.getPieceAt(cur)};
            if (cur == newPoint) return !here || here->getColor() != m_color;
            if (here) break;
        }
    }
    return false;
}

bool Rook::isValidMove(Point newPoint, const Board& board) const
{
    if (newPoint.outOfBounds()) return false;
    for (const auto& dir : Settings::verticalMovements)
    {
        int scale{1}; Point cur{};
        while (!(cur = m_point + dir * scale++).outOfBounds())
        {
            const Piece* here{board.getPieceAt(cur)};
            if (cur == newPoint) return !here || here->getColor() != m_color;
            if (here) break;
        }
    }
    return false;
}

bool Queen::isValidMove(Point newPoint, const Board& board) const
{
    if (newPoint.outOfBounds()) return false;
    auto slide{[&](const auto& dirs) -> bool {
        for (const auto& dir : dirs)
        {
            int scale{1}; Point cur{};
            while (!(cur = m_point + dir * scale++).outOfBounds())
            {
                const Piece* here{board.getPieceAt(cur)};
                if (cur == newPoint) return !here || here->getColor() != m_color;
                if (here) break;
            }
        }
        return false;
    }};
    return slide(Settings::verticalMovements) || slide(Settings::diagionalMovements);
}

bool King::isValidMove(Point newPoint, const Board& board) const
{
    if (newPoint.outOfBounds()) return false;
    auto checkDir{[&](const auto& dirs) -> bool {
        for (const auto& dir : dirs)
        {
            Point cand{m_point + dir};
            if (cand == newPoint)
            {
                const Piece* dest{board.getPieceAt(newPoint)};
                return !dest || dest->getColor() != m_color;
            }
        }
        return false;
    }};
    return checkDir(Settings::verticalMovements) || checkDir(Settings::diagionalMovements);
}

//board constructor
Board::Board()
{
    for (int i{0}; i < Settings::boardSize; ++i)
        for (int j{0}; j < Settings::boardSize; ++j)
        {
            const Point p{i, j};
            if (i == 1 || i == 6)
                m_board[i][j] = std::make_unique<Pawn>(i == 1 ? Piece::white : Piece::black, p);
            else if (i == 0 || i == 7)
            {
                Piece::Color side{i == 0 ? Piece::white : Piece::black};
                if      (j == 0 || j == 7) m_board[i][j] = std::make_unique<Rook>(side, p, j == 0 ? Rook::queenside : Rook::kingside);
                else if (j == 1 || j == 6) m_board[i][j] = std::make_unique<Knight>(side, p);
                else if (j == 2 || j == 5) m_board[i][j] = std::make_unique<Bishop>(side, p);
                else if (j == 3)           m_board[i][j] = std::make_unique<Queen>(side, p);
                else if (j == 4)           m_board[i][j] = std::make_unique<King>(side, p);
            }
            else m_board[i][j] = nullptr;
        }
}

//board helper functions
Point Board::findKing(Piece::Color color) const
{
    for (int r{0}; r < Settings::boardSize; ++r)
        for (int c{0}; c < Settings::boardSize; ++c)
        {
            const Piece* p{m_board[r][c].get()};
            if (p && p->getType() == Piece::king && p->getColor() == color)
                return {r, c};
        }
    return {-1, -1};
}

bool Board::isInCheck(Piece::Color color) const
{
    Point kingPos{findKing(color)};
    Piece::Color enemy{color == Piece::white ? Piece::black : Piece::white};
    for (int r{0}; r < Settings::boardSize; ++r)
        for (int c{0}; c < Settings::boardSize; ++c)
        {
            const Piece* p{m_board[r][c].get()};
            if (p && p->getColor() == enemy && p->isValidMove(kingPos, *this))
                return true;
        }
    return false;
}

bool Board::insufficientMaterial() const
{
    std::vector<const Piece*> white{}, black{};
    for (int r{0}; r < Settings::boardSize; ++r)
        for (int c{0}; c < Settings::boardSize; ++c)
        {
            const Piece* p{m_board[r][c].get()};
            if (!p) continue;
            if (p->getType() == Piece::pawn || p->getType() == Piece::rook || p->getType() == Piece::queen)
                return false;
            if (p->getColor() == Piece::white) white.push_back(p);
            else                               black.push_back(p);
        }
    if (white.size() == 1 && black.size() == 1) return true;
    auto minorOnly{[](const std::vector<const Piece*>& side){
        return side.size() == 2 &&
               (side[0]->getType() == Piece::bishop || side[0]->getType() == Piece::knight ||
                side[1]->getType() == Piece::bishop || side[1]->getType() == Piece::knight);
    }};
    if ((white.size() == 2 && black.size() == 1 && minorOnly(white)) ||
        (black.size() == 2 && white.size() == 1 && minorOnly(black))) return true;
    if (white.size() == 2 && black.size() == 2)
    {
        const Piece* wb{nullptr}, *bb{nullptr};
        for (auto* p : white) if (p->getType() == Piece::bishop) wb = p;
        for (auto* p : black) if (p->getType() == Piece::bishop) bb = p;
        if (wb && bb)
        {
            bool wLight{(wb->getPoint().row + wb->getPoint().col) % 2 == 0};
            bool bLight{(bb->getPoint().row + bb->getPoint().col) % 2 == 0};
            if (wLight == bLight) return true;
        }
    }
    return false;
}