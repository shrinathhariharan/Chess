#include "chess.h"

Rectangle tileRect(int row, int col)
{
    int screenRow{Settings::boardSize - 1 - row}; // flip for display
    return Rectangle{
        static_cast<float>(Settings::boardOffset + col * Settings::tileSize),
        static_cast<float>(Settings::boardOffset + screenRow * Settings::tileSize),
        static_cast<float>(Settings::tileSize),
        static_cast<float>(Settings::tileSize)
    };
}

//{-1, 1} is outside board
Point screenToBoard(int screenX, int screenY)
{
    if (screenX < Settings::boardOffset || screenY < Settings::boardOffset) 
        return {-1, -1};

    int col{(screenX - Settings::boardOffset) / Settings::tileSize};
    int screenRow{(screenY - Settings::boardOffset) / Settings::tileSize};
    int row{Settings::boardSize - 1 - screenRow};
    
    Point p{row, col};
    if (p.outOfBounds()) return {-1, -1};
    return p;
}

//draw helpers
void drawPiece(const Textures& tex, const Piece& piece, int row, int col)
{
    Rectangle dest{tileRect(row, col)};
    //scale texture to fill the tile
    Rectangle src{0, 0,
        static_cast<float>(tex.pieces[piece.getColor()][piece.getType()].width),
        static_cast<float>(tex.pieces[piece.getColor()][piece.getType()].height)};
    DrawTexturePro(tex.pieces[piece.getColor()][piece.getType()], src, dest, {0,0}, 0.0f, WHITE);
}

void drawBoard(const Textures& tex, const Board& board, std::optional<Point> selected, const std::vector<Point>& highlights)
{
    float boardDisplaySize = static_cast<float>(Settings::boardSize * Settings::tileSize);

    Rectangle destBounds {
        static_cast<float>(Settings::boardOffset),
        static_cast<float>(Settings::boardOffset),
        boardDisplaySize,
        boardDisplaySize
    };

    //determine shorter side of asset to get square
    float assetSize{(tex.board.width < tex.board.height) ? 
                      static_cast<float>(tex.board.width) : static_cast<float>(tex.board.height)};

    //center source rectangle
    float srcX{(static_cast<float>(tex.board.width) - assetSize) / 2.0f};
    float srcY{(static_cast<float>(tex.board.height) - assetSize) / 2.0f};

    Rectangle boardSrc{srcX, srcY, assetSize, assetSize}; //read a 1:1 image
    
    DrawTexturePro(tex.board, boardSrc, destBounds, {0,0}, 0.0f, WHITE); //draw scaled board

    DrawRectangleLinesEx(destBounds, 2.0f, DARKGRAY); //raw thin reference outer border lines around the play zone

    //draw board indicators and highlights
    for (int r{0}; r < Settings::boardSize; ++r)
    {
        for (int c{0}; c < Settings::boardSize; ++c)
        {
            Point p{r, c};
            Rectangle rRect{tileRect(r, c)};

            if (selected && *selected == p) //highlight selected piece's square
                DrawRectangleRec(rRect, Color{255, 255, 0, 100});
            
            for (const auto& hl : highlights) //highlight legal moves
            {
                if (hl == p)
                {
                    if (board.getPieceAt(p))
                        DrawRectangleRec(rRect, Color{255, 0, 0, 120}); //draw red tint to show capture
                    else
                        DrawCircle(static_cast<int>(rRect.x + rRect.width / 2.0f),
                                   static_cast<int>(rRect.y + rRect.height / 2.0f),
                                   12.0f, Color{0, 0, 0, 60}); //dot to show moves
                }
            }
            const Piece* piece{board.getPieceAt(p)};
            if (piece)
                drawPiece(tex, *piece, r, c);
        }
    }

    for (int i{0}; i < Settings::boardSize; ++i)
    {
        //files 'a' through 'h'
        char fileChar{static_cast<char>('a' + i)};
        std::string fileStr(1, fileChar); //string of file char
        float fileX{static_cast<float>(Settings::boardOffset + i * Settings::tileSize + (Settings::tileSize / 2) - 5)};
        float fileY{static_cast<float>(Settings::boardOffset + static_cast<int>(boardDisplaySize) + 10)};
        
        DrawTextEx(Settings::chessFont, fileStr.c_str(), Vector2{fileX, fileY}, 20.0f, 1.0f, WHITE);
        
        //ranks 1 through 8 (mirrored because white is on bottom (flipping))
        char rankChar{static_cast<char>('1' + i)};
        std::string rankStr(1, rankChar);
        float rankY{static_cast<float>(Settings::boardOffset + (Settings::boardSize - 1 - i) * Settings::tileSize + (Settings::tileSize / 2) - 10)};
        float rankX{static_cast<float>(Settings::boardOffset - 25)};
        
        DrawTextEx(Settings::chessFont, rankStr.c_str(), Vector2{rankX, rankY}, 20.0f, 1.0f, WHITE);
    }
}

//promotion UI
std::optional<Piece::Type> drawPromotionPanel(const Textures& tex, Piece::Color color)
{
    //semi-transparent overlay
    const int panelW{4 * Settings::tileSize};
    const int panelH{Settings::tileSize + 20};
    const int panelX{(Settings::windowSize - panelW) / 2};
    const int panelY{(Settings::windowSize - panelH) / 2};

    DrawRectangle(0, 0, Settings::windowSize, Settings::windowSize + Settings::statusBarHeight,
                  {0, 0, 0, 160});
    DrawRectangle(panelX - 4, panelY - 4, panelW + 8, panelH + 8, DARKGRAY);
    DrawRectangle(panelX, panelY, panelW, panelH, {30, 30, 30, 255});
    
    float promoTextX{static_cast<float>(panelX + 4)};
    float promoTextY{static_cast<float>(panelY + panelH + 6)};
    DrawTextEx(Settings::chessFont, "Choose promotion piece", Vector2{promoTextX, promoTextY}, 20.0f, 1.0f, LIGHTGRAY);

    constexpr std::array<Piece::Type, 4> choices{Piece::queen, Piece::rook, Piece::bishop, Piece::knight};

    for (int i{0}; i < 4; ++i)
    {
        Rectangle btn{
            static_cast<float>(panelX + i * Settings::tileSize),
            static_cast<float>(panelY + 10),
            static_cast<float>(Settings::tileSize),
            static_cast<float>(Settings::tileSize)
        };

        bool hover{CheckCollisionPointRec(GetMousePosition(), btn)};
        DrawRectangleRec(btn, hover ? Color{200, 200, 50, 200} : Color{80, 80, 80, 200});

        const Texture2D& t{tex.pieces[color][choices[i]]};
        Rectangle src{0, 0, static_cast<float>(t.width), static_cast<float>(t.height)};
        DrawTexturePro(t, src, btn, {0,0}, 0.0f, WHITE);

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            return choices[i];
    }

    return std::nullopt;
}

//game-over UI
void drawGameOverOverlay(const std::string& message, bool& savePGNClicked)
{
    DrawRectangle(0, Settings::windowSize,
                  Settings::windowSize, Settings::statusBarHeight, {20, 20, 20, 230});
    
    float msgX{10.0f};
    float msgY{static_cast<float>(Settings::windowSize + 14)};
    DrawTextEx(Settings::chessFont, message.c_str(), Vector2{msgX, msgY}, 20.0f, 1.0f, YELLOW);

    Rectangle saveBtn{
        static_cast<float>(Settings::windowSize - 140),
        static_cast<float>(Settings::windowSize + 9),
        120.0f, 32.0f
    };
    bool hover{CheckCollisionPointRec(GetMousePosition(), saveBtn)};
    DrawRectangleRec(saveBtn, hover ? Color{80 ,180 ,80 ,255} : Color{50 ,140 ,50 ,255});
    
    float btnTextX{saveBtn.x + 12.0f};
    float btnTextY{saveBtn.y + 7.0f};
    DrawTextEx(Settings::chessFont, "Save PGN", Vector2{btnTextX, btnTextY}, 16.0f, 1.0f, WHITE);
    
    if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        savePGNClicked = true;
}

//status board
void drawStatusBar(const std::string& msg)
{
    DrawRectangle(0, Settings::windowSize, Settings::windowSize, Settings::statusBarHeight, {25, 25, 35, 255});
    
    float msgX{10.0f};
    float msgY{static_cast<float>(Settings::windowSize + 12)};
    DrawTextEx(Settings::chessFont, msg.c_str(), Vector2{msgX, msgY}, 20.0f, 1.0f, LIGHTGRAY);
}