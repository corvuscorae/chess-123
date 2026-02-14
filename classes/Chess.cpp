#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == WHITE ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    int field = 0;
    int x = 0;
    int y = _gameOptions.rowY - 1;
    for(char c : fen){
        if(c == ' '){
            field++;

            // field 0 = piece placement (handled below)
            // TODO: handling other fields...
            // field 1 = active color
            // field 2 = castling rights
            // field 3 = en passant targets
            // field 4 = halfmove clock
            // field 5 = fullmove number

            continue;
        }

        if(field > 0){ continue; }

        //* PIECE PLACEMENT *//
        if(c >= '0' && c <= '9'){   // numerics indicate empty spaces
            int num_empty = c - '0';
            x += num_empty - 1;
            continue;
        }
        
        if(c == '/'){   // move to next row
            y--;
            x = 0;
            continue;
        }

        ChessSquare* square = _grid->getSquare(x, y);   // get current square
        _grid->setEnabled(x, y, true);

        // convert char to piece
        ChessPiece guy;

        // get color 
        char c_tolower = tolower(c);    // if this is not the same as c, then c is caps (white)
        int player = (c == c_tolower) ? BLACK : WHITE;

        if      (c_tolower == 'p'){ guy = Pawn; }
        else if (c_tolower == 'r'){ guy = Rook; }
        else if (c_tolower == 'n'){ guy = Knight; }
        else if (c_tolower == 'b'){ guy = Bishop; }
        else if (c_tolower == 'q'){ guy = Queen; }
        else if (c_tolower == 'k'){ guy = King; }
        else { 
            // TODO: LOG ERROR
            guy = NoPiece; 
        }

        // place piece
        Bit* piece = PieceForPlayer(player, guy);   // make piece
        int notation = (player == BLACK) ? guy + 128 : guy; // this feels hacky // TODO: fix?
        piece->setGameTag(notation);              // set tag to piece notation
        piece->setPosition(square->getPosition());  // put it on the board
        square->setBit(piece);
        x++;
    }
    
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    return true;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;
}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
