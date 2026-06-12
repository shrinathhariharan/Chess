#include "chess.h"
#include "functiondefs.h"

int main()
{
    InitWindow(Settings::windowSize, Settings::windowSize + Settings::statusBarHeight, "Chess");
    SetTargetFPS(60);

    Textures tex{};
    tex.load();

    Board board{};
    board.recordPosition(getPositionKey(board));

    //UI state
    enum class GameState { playing, promotion, gameOver };
    GameState state{GameState::playing};

    std::optional<Point> selected{std::nullopt}; //currently selected square
    std::vector<Point>   legalMoves{}; //highlights for selected piece

    //promotion state
    Point promotionSquare{-1,-1};
    Piece::Color promotionColor{Piece::white};

    std::string statusMsg{"White's move"};
    std::string gameOverMsg{};
    std::string result{};
    bool pgnSaved{false};

    while (!WindowShouldClose())
    {
        if (state == GameState::playing)
        {
            Piece::Color turnColor{board.getCurrentTurnColor()};

            //check game-ending conditions at start of turn
            bool inCheck{board.isInCheck(turnColor)};
            bool canMove{hasLegalMove(board, turnColor)};

            if (!canMove)
            {
                if (inCheck)
                {
                    Piece::Color winner{turnColor == Piece::white ? Piece::black : Piece::white};
                    gameOverMsg = std::string{winner == Piece::white ? "White" : "Black"} + " wins by checkmate!";
                    result      = (winner == Piece::white ? "1-0" : "0-1");
                }
                else
                {
                    gameOverMsg = "Stalemate — draw!";
                    result      = "1/2-1/2";
                }
                state = GameState::gameOver;
            }
            else if (board.insufficientMaterial())
            {
                gameOverMsg = "Draw by insufficient material.";
                result      = "1/2-1/2";
                state       = GameState::gameOver;
            }
            else
            {
                statusMsg = std::string{turnColor == Piece::white ? "White" : "Black"} + "'s move";
                if (inCheck) statusMsg += "  —  CHECK!";
            }

            if (state == GameState::playing && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                int mx{GetMouseX()}, my{GetMouseY()};
                Point clicked{screenToBoard(mx, my)};

                if (!clicked.outOfBounds())
                {
                    if (!selected)
                    {
                        //first click sets the piece
                        const Piece* p{board.getPieceAt(clicked)};
                        if (p && p->getColor() == turnColor)
                        {
                            selected   = clicked;
                            legalMoves = legalDestinations(board, clicked);
                        }
                    }
                    else
                    {
                        //second click does the move
                        Point from{*selected};
                        selected   = std::nullopt;
                        legalMoves.clear();

                        //redo if selcting same
                        const Piece* clickedPiece{board.getPieceAt(clicked)};
                        if (clickedPiece && clickedPiece->getColor() == turnColor)
                        {
                            selected   = clicked;
                            legalMoves = legalDestinations(board, clicked);
                        }
                        else
                        {
                            //castling detection
                            Piece* fromPiece{board.getPieceAt(from)};
                            if (fromPiece && fromPiece->getType() == Piece::king
                                && std::abs(clicked.col - from.col) == 2)
                            {
                                Move::Castle ct{clicked.col > from.col
                                    ? Move::castle_kingside : Move::castle_queenside};
                                if (!tryCastle(board, turnColor, ct))
                                    statusMsg = "Castling not allowed.";
                                else
                                    board.nextMove();
                            }
                            else
                            {
                                auto result_opt{tryMove(board, from, clicked)};
                                if (!result_opt.has_value())
                                {
                                    //draw validation
                                    gameOverMsg = "Draw by " +
                                        std::string{board.getHalfMoveClock() >= 100
                                            ? "fifty-move rule." : "threefold repetition."};
                                    result = "1/2-1/2";
                                    state  = GameState::gameOver;
                                }
                                else if (*result_opt)
                                {
                                    //promotion validation
                                    Piece* landed{board.getPieceAt(clicked)};
                                    if (landed && landed->getType() == Piece::pawn)
                                    {
                                        int promRow{landed->getColor() == Piece::white
                                            ? Settings::boardSize - 1 : 0};
                                        if (clicked.row == promRow)
                                        {
                                            promotionSquare = clicked;
                                            promotionColor  = landed->getColor();
                                            state           = GameState::promotion;
                                        }
                                    }
                                    if (state == GameState::playing)
                                        board.nextMove();
                                }
                                //other moves are illegal; do nothing
                            }
                        }
                    }
                }
            }
        }
        else if (state == GameState::promotion)
        {
            //already handled but waiting for panel click
        }

        //draw
        BeginDrawing();
        ClearBackground({10, 10, 20, 255});

        drawBoard(tex, board, selected, legalMoves);

        if (state == GameState::promotion)
        {
            auto choice{drawPromotionPanel(tex, promotionColor)};
            if (choice)
            {
                //replace pawn with chosen piece
                board.clearSquare(promotionSquare);
                std::unique_ptr<Piece> promoted{};
                switch (*choice)
                {
                case Piece::queen:  promoted = std::make_unique<Queen> (promotionColor, promotionSquare); break;
                case Piece::rook:   promoted = std::make_unique<Rook>  (promotionColor, promotionSquare, Rook::kingside); break;
                case Piece::bishop: promoted = std::make_unique<Bishop>(promotionColor, promotionSquare); break;
                case Piece::knight: promoted = std::make_unique<Knight>(promotionColor, promotionSquare); break;
                default: break;
                }
                board.placePiece(std::move(promoted), promotionSquare);
                board.nextMove();
                state = GameState::playing;
            }
        }

        if (state == GameState::gameOver)
        {
            bool saveClicked{false};
            drawGameOverOverlay(gameOverMsg, saveClicked);
            if (saveClicked && !pgnSaved)
            {
                savePGN(board, result);
                pgnSaved = true;
                gameOverMsg += "  [PGN saved]";
            }
        }
        else
        {
            drawStatusBar(statusMsg);
        }

        EndDrawing();
    }

    tex.unload();
    CloseWindow();
    return 0;
}