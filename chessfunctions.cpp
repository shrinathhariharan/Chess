#include "chess.h"
#include <fstream>

std::string getPositionKey(const Board& board)
{
    std::string key{};
    key.reserve(Settings::boardSize * Settings::boardSize + 1);
    for (int r{0}; r < Settings::boardSize; ++r)
        for (int c{0}; c < Settings::boardSize; ++c)
        {
            const Piece* p{board.getPieceAt({r, c})};
            if (!p) { key += '.'; continue; }
            char sym{p->getSymbol()};
            if (p->getColor() == Piece::black) sym = static_cast<char>(std::tolower(sym));
            key += sym;
        }
    key += (board.getCurrentTurnColor() == Piece::white ? 'w' : 'b');
    return key;
}

bool squaresAttacked(const Board& board, int row, int col1, int col2, Piece::Color byEnemy)
{
    for (int c{col1}; c <= col2; ++c)
    {
        Point sq{row, c};
        for (int r{0}; r < Settings::boardSize; ++r)
            for (int cc{0}; cc < Settings::boardSize; ++cc)
            {
                const Piece* p{board.getPieceAt(Point{r, cc})};
                if (p && p->getColor() == byEnemy && p->isValidMove(sq, board))
                    return true;
            }
    }
    return false;
}

bool tryCastle(Board& board, Piece::Color color, Move::Castle castleType)
{
    int row{color == Piece::white ? 0 : 7};
    Piece::Color enemy{color == Piece::white ? Piece::black : Piece::white};
    
    King* king{dynamic_cast<King*>(board.getPieceAt(Point{row, 4}))};
    if (!king || king->hasMoved()) 
        return false;
    int rookCol{castleType == Move::castle_kingside ? 7 : 0};
    Rook* rook{dynamic_cast<Rook*>(board.getPieceAt(Point{row, rookCol}))};

    if (!rook || rook->hasMoved()) 
        return false;
    for (int c{std::min(4, rookCol) + 1}; c <= std::max(4, rookCol) - 1; ++c)
    {
        if (board.getPieceAt(Point{row, c})) 
            return false;
    }

    int kingDest{castleType == Move::castle_kingside ? 6 : 2};
    if (squaresAttacked(board, row, std::min(4, kingDest), std::max(4, kingDest), enemy)) 
        return false;

    int rookDest{castleType == Move::castle_kingside ? 5 : 3};
    Point kingOld{row, 4}, kingNew{row, kingDest}, rookOld{row, rookCol}, rookNew{row, rookDest};

    auto kp{board.takePieceAt(kingOld)}; 
    auto rp{board.takePieceAt(rookOld)};
    kp->setPoint(kingNew); rp->setPoint(rookNew);
    king->markMoved(); rook->markMoved();

    board.placePiece(std::move(kp), kingNew); board.placePiece(std::move(rp), rookNew);
    board.addMove(castleType == Move::castle_kingside ? "O-O" : "O-O-O");
    return true;
}

bool hasLegalMove(Board& board, Piece::Color color)
{
    for (int r{0}; r < Settings::boardSize; ++r)
        for (int c{0}; c < Settings::boardSize; ++c)
        {
            Piece* piece{board.getPieceAt(Point{r, c})};
            if (!piece || piece->getColor() != color) 
                continue;

            for (int tr{0}; tr < Settings::boardSize; ++tr)
                for (int tc{0}; tc < Settings::boardSize; ++tc)
                {
                    Point dest{tr, tc};
                    if (!piece->isValidMove(dest, board)) 
                        continue;
                    bool isEP{false}; Point epCap{-1,-1};
                    if (piece->getType() == Piece::pawn)
                    {
                        int cd{std::abs(dest.col - piece->getPoint().col)};
                        if (cd == 1 && !board.getPieceAt(dest)) { isEP = true; epCap = {piece->getPoint().row, dest.col}; }
                    }

                    Point src{piece->getPoint()};
                    auto mp{board.takePieceAt(src)}; auto cp{board.takePieceAt(dest)};
                    std::unique_ptr<Piece> ep{};
                    if (isEP) 
                        ep = board.takePieceAt(epCap);
                    mp->setPoint(dest); board.placePiece(std::move(mp), dest);
                    bool inCheck{board.isInCheck(color)};
                    mp = board.takePieceAt(dest); mp->setPoint(src); 
                    board.placePiece(std::move(mp), src);

                    if (cp) board.placePiece(std::move(cp), dest);
                    if (isEP && ep) board.placePiece(std::move(ep), epCap);
                    if (!inCheck) return true;
                }
        }

    return false;
}

//true is valid move, false is invalid move, nullopt is draw
std::optional<bool> tryMove(Board& board, Point from, Point to)
{
    Piece* piece{board.getPieceAt(from)};
    if (!piece || piece->getColor() != board.getCurrentTurnColor()) return false;
    if (!piece->isValidMove(to, board)) return false;

    bool isEnPassant{false};
    Point capturedPawnPos{-1, -1};
    if (piece->getType() == Piece::pawn)
    {
        int colDiff{std::abs(to.col - from.col)};
        if (colDiff == 1 && !board.getPieceAt(to))
        { isEnPassant = true; capturedPawnPos = {from.row, to.col}; }
    }

    // Simulate
    auto movingPiece{board.takePieceAt(from)};
    auto capturedPiece{board.takePieceAt(to)};
    std::unique_ptr<Piece> epPawn{};
    if (isEnPassant) epPawn = board.takePieceAt(capturedPawnPos);
    movingPiece->setPoint(to);
    board.placePiece(std::move(movingPiece), to);
    bool leavesInCheck{board.isInCheck(board.getCurrentTurnColor())};
    movingPiece = board.takePieceAt(to); movingPiece->setPoint(from);
    board.placePiece(std::move(movingPiece), from);
    if (capturedPiece)  board.placePiece(std::move(capturedPiece), to);
    if (isEnPassant && epPawn) board.placePiece(std::move(epPawn), capturedPawnPos);
    if (leavesInCheck) return false;

    // Commit
    bool isCapture{board.getPieceAt(to) != nullptr || isEnPassant};
    if (piece->getType() == Piece::pawn || isCapture) board.resetHalfMoveClock();
    else board.incrementHalfMoveClock();

    board.clearEnPassantTarget();
    auto pieceToMove{board.takePieceAt(from)};
    board.clearSquare(to);
    if (isEnPassant) board.clearSquare(capturedPawnPos);
    pieceToMove->setPoint(to);

    if (pieceToMove->getType() == Piece::pawn)
    {
        int rowDiff{to.row - from.row};
        if (std::abs(rowDiff) == 2)
            board.setEnPassantTarget(Point{from.row + rowDiff / 2, from.col});
        dynamic_cast<Pawn*>(pieceToMove.get())->markMoved();
    }
    if (pieceToMove->getType() == Piece::rook)  dynamic_cast<Rook*>(pieceToMove.get())->markMoved();
    if (pieceToMove->getType() == Piece::king)  dynamic_cast<King*>(pieceToMove.get())->markMoved();

    board.placePiece(std::move(pieceToMove), to);

    // Build PGN string for this move
    Move m{};
    m.type   = board.getPieceAt(to)->getType();
    m.origCol = static_cast<char>('a' + from.col);
    m.origRow = from.row + 1;
    m.newCol  = static_cast<char>('a' + to.col);
    m.newRow  = to.row + 1;
    board.addMove(m);

    // Draw checks
    if (board.getHalfMoveClock() >= 100) return std::nullopt;
    std::string posKey{getPositionKey(board)};
    board.recordPosition(posKey);
    if (board.getPositionCount(posKey) >= 3) return std::nullopt;

    return true;
}

void savePGN(const Board& board, const std::string& result)
{
    std::ofstream file{Settings::pgnFilePath};
    if (!file) return;
    std::size_t n{board.getPGNMoveCount()};
    for (std::size_t i{0}; i < n; i += 2)
    {
        file << (i / 2 + 1) << ". " << board[i];
        if (i + 1 < n) file << ' ' << board[i + 1];
        file << "  ";
    }
    if (!result.empty()) file << result;
    file << '\n';
}

//get legal point destinations
std::vector<Point> legalDestinations(Board& board, Point from)
{
    std::vector<Point> dests{};
    Piece* piece{board.getPieceAt(from)};
    if (!piece) return dests;

    for (int tr{0}; tr < Settings::boardSize; ++tr)
        for (int tc{0}; tc < Settings::boardSize; ++tc)
        {
            Point dest{tr, tc};
            if (!piece->isValidMove(dest, board)) 
                continue;
            // Simulate to filter moves that leave king in check
            bool isEP{false}; 
            Point epCap{-1,-1};
            if (piece->getType() == Piece::pawn)
            {
                int cd{std::abs(dest.col - from.col)};
                if (cd == 1 && !board.getPieceAt(dest)) 
                { 
                    isEP = true; 
                    epCap = {from.row, dest.col}; 
                }
            }
            auto mp{board.takePieceAt(from)}; 
            auto cp{board.takePieceAt(dest)};

            std::unique_ptr<Piece> ep{};
            if (isEP) 
                ep = board.takePieceAt(epCap);
            mp->setPoint(dest); board.placePiece(std::move(mp), dest);
            bool inCheck{board.isInCheck(piece->getColor())};
            mp = board.takePieceAt(dest); mp->setPoint(from); board.placePiece(std::move(mp), from);

            if (cp) board.placePiece(std::move(cp), dest);
            if (isEP && ep) board.placePiece(std::move(ep), epCap);
            if (!inCheck) dests.push_back(dest);
        }

    // Add castling destinations for king
    if (piece->getType() == Piece::king)
    {
        int row{piece->getColor() == Piece::white ? 0 : 7};
        Piece::Color enemy{piece->getColor() == Piece::white ? Piece::black : Piece::white};
        King* king{dynamic_cast<King*>(piece)};
        if (king && !king->hasMoved())
        {
            //kingside
            {
                Rook* rook{dynamic_cast<Rook*>(board.getPieceAt(Point{row, 7}))};
                if (rook && !rook->hasMoved() &&
                    !board.getPieceAt({row,5}) && !board.getPieceAt({row,6}) &&
                    !squaresAttacked(board, row, 4, 6, enemy))
                    dests.push_back({row, 6});
            }
            //queenside
            {
                Rook* rook{dynamic_cast<Rook*>(board.getPieceAt(Point{row, 0}))};
                if (rook && !rook->hasMoved() &&
                    !board.getPieceAt({row,1}) && !board.getPieceAt({row,2}) && !board.getPieceAt({row,3}) &&
                    !squaresAttacked(board, row, 2, 4, enemy))
                    dests.push_back({row, 2});
            }
        }
    }

    return dests;
}