#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);

    // bitboards
    for(int i = 0; i < 64; i++){
        _knightBitboards[i] = generateKnightMoveBitboard(i);
    }

    // initMagicBitboards();

    for(int i = 0; i < 128; i++){
        _bitboardLookup[i] = 0;
    }

    _bitboardLookup['P'] = W_PAWNS;
    _bitboardLookup['N'] = W_KNIGHTS;
    _bitboardLookup['B'] = W_BISHOPS;
    _bitboardLookup['R'] = W_ROOKS;
    _bitboardLookup['Q'] = W_QUEENS;
    _bitboardLookup['K'] = W_KING;
    _bitboardLookup['p'] = B_PAWNS;
    _bitboardLookup['n'] = B_KNIGHTS;
    _bitboardLookup['b'] = B_BISHOPS;
    _bitboardLookup['r'] = B_ROOKS;
    _bitboardLookup['q'] = B_QUEENS;
    _bitboardLookup['k'] = B_KING;
    _bitboardLookup['0'] = EMPTY_SQUARES;
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

    _currentPlayer = WHITE;
    _moves = generateAllMoves();
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
        piece->setParent(square);
        int notation = (player == BLACK) ? (guy + 128) : guy; // this feels hacky // TODO: fix?
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
    if (pieceColor != currentPlayer) return false;

    ChessSquare *square = (ChessSquare *)&src;
    int squareIndex = square->getSquareIndex();
    for(auto move : _moves){
        if(move.from == squareIndex){
            return true;
        }
    }
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare *srcSquare = (ChessSquare *)&src;
    ChessSquare *dstSquare = (ChessSquare *)&dst;

    int srcSquareIndex = srcSquare->getSquareIndex();
    int dstSquareIndex = dstSquare->getSquareIndex();
    for(auto move : _moves){
        if(move.from == srcSquareIndex && move.to == dstSquareIndex){
            return true;
        }
    }
    return false;
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

// MOVE GENERATIONS //
void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst){
    _currentPlayer = (_currentPlayer == WHITE) ? BLACK : WHITE;
    _moves = generateAllMoves();
    clearBoardHighlights();
    endTurn();
}

std::vector<BitMove> Chess::generateAllMoves(){
    std::vector<BitMove> moves;
    moves.reserve(32);
    std::string state = stateString();

    for(int i = 0; i < e_numBitboards; i++){
        _bitboards[i] = 0;
    }

    for(int i = 0; i < 64; i++){
        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
        if(state[i] != '0'){
            _bitboards[OCCUPANCY] |= 1ULL << i;
            _bitboards[isupper(state[i]) ? W_ALL : B_ALL] |= 1ULL << i;
        }
    }

    int bitIndex = _currentPlayer == WHITE ? W_PAWNS : B_PAWNS;
    int oppBitIndex = _currentPlayer == BLACK ? W_PAWNS : B_PAWNS;

    int enemyBoardIndex = _currentPlayer == WHITE ? B_ALL : W_ALL;

    // pawns
    generatePawnMoves(moves, _bitboards[W_PAWNS + bitIndex], ~_bitboards[OCCUPANCY].getData(), _bitboards[enemyBoardIndex], _currentPlayer);

    // knights
    generateKnightMoves(moves, _bitboards[W_KNIGHTS + bitIndex], ~_bitboards[OCCUPANCY].getData());

    // kings
    uint64_t kingBoard = _bitboards[W_KING + bitIndex].getData();
    while(kingBoard){
        int sq = bitScanForward(kingBoard);
        generateKingMoves(state.c_str(), moves, sq / 8, sq & 7);
        kingBoard &= (kingBoard - 1);
    }

    return moves;
}

void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knights, uint64_t occupancy){
    knights.forEachBit([&](int from){
        BitboardElement canMoveTo = _knightBitboards[from].getData() & occupancy;
        canMoveTo.forEachBit([&](int to){
            moves.emplace_back(from, to, Knight);
        });
    });
}

void Chess::generateKnightMoves(std::vector<BitMove>& moves, std::string &state){
    // L-shape offsets
    std::pair<int, int> offsets[] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };

    char knightPiece = _currentPlayer == WHITE ? 'N' : 'n';
    int index = 0;
    for(char sq : state){
        if(sq == knightPiece){
            int rank = index / 8;
            int file = index % 8;

            constexpr uint64_t oneBit = 1;
            for(auto [dr, df] : offsets){
                int r = rank + dr;
                int f = file + df;

                if(r >= 0 && r < 8 && f >=0 && f < 8){
                    moves.emplace_back(index, r * 8 + f, Knight);
                }
            }
        }
        index++;
    }
}

BitboardElement Chess::generateKnightMoveBitboard(int square){
    // L-shape offsets
    std::pair<int, int> offsets[] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };

    BitboardElement bb = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    constexpr uint64_t oneBit = 1;
    for(auto [dr, df] : offsets){
        int r = rank + dr;
        int f = file + df;

        if(r >= 0 && r < 8 && f >=0 && f < 8){
            bb |= oneBit << (r * 8 + f);
        }
    }

    return bb;
}


void Chess::generateBishopMoves(const char* state, std::vector<BitMove>& moves, int row, int col){
    static const std::vector<std::pair<int, int>> diagonals = {
        {1,1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    generateLinearMoves(state, moves, row, col, diagonals);
}

void Chess::generateLinearMoves(const char* state, std::vector<BitMove>& moves, int row, int col, const std::vector<std::pair<int, int>> directions){
    for( auto &dir : directions){
        int currRow = row + dir.first;
        int currCol = col + dir.second;

        while(currRow >= 0 && currRow < 8 && currCol >= 0 && currCol < 8){
            if(pieceNotation(currCol, currRow) != '0'){
                addMoveIfValid(state, moves, row, col, currRow, currCol);
                break;
            }

            addMoveIfValid(state, moves, row, col, currRow, currCol);
            currRow += dir.first;
            currCol += dir.second;
        }
    }

}

int Chess::stateColor(const char* state, int row, int col){
    char piece = pieceNotation(col, row);
    if(piece == '0') return -2;
    return (piece < 'a') ? WHITE : BLACK;
}

void Chess::addMoveIfValid(const char* state, std::vector<BitMove>& moves, int fromRow, int fromCol, int toRow, int toCol){
    if(toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8){
        int fromColor = stateColor(state, fromRow, fromCol);
        int toColor = stateColor(state, toRow, toCol);
        if(fromColor != toColor){
            moves.emplace_back(fromRow*8+fromCol, toRow*8+toCol, Knight);
        }
    }
}


// TODO: replace ternaries with template (isWhite)
void Chess::generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawns, const BitboardElement empty, const BitboardElement enemies, char color){
    if(pawns.getData() == 0){
        return;
    }

    // constants for ranks and files
    constexpr uint64_t notAFile (0xFEFEFEFEFEFEFEFEULL);
    constexpr uint64_t notHFile (0x7F7F7F7F7F7F7F7FULL);
    constexpr uint64_t rank3    (0x0000000000FF0000ULL);
    constexpr uint64_t rank6    (0x0000FF0000000000ULL);

    BitboardElement demoRight(notAFile);
    BitboardElement demoLeft(notHFile);

    BitboardElement singleMoves = (color == WHITE) ? 
        (pawns.getData() << 8) & empty.getData() :
        (pawns.getData() >> 8) & empty.getData() ;
    BitboardElement doubleMoves = (color == WHITE) ? 
        ((singleMoves.getData() & rank3) << 8) & empty.getData() :
        ((singleMoves.getData() & rank6) >> 8) & empty.getData() ;

    BitboardElement capturesLeft = (color == WHITE) ? 
        ((pawns.getData() & notHFile) << 7) & enemies.getData() :
        ((pawns.getData() & notAFile) >> 9) & enemies.getData() ;
    BitboardElement capturesRight = (color == WHITE) ? 
        ((pawns.getData() & notAFile) << 9) & enemies.getData() :
        ((pawns.getData() & notHFile) >> 7) & enemies.getData() ;

    int forwardSingleShift  = (color == WHITE) ? 8 : -8;
    int forwardDoubleShift  = (color == WHITE) ? 16 : -16;
    int captureLeftShift    = (color == WHITE) ? 7 : -9;
    int captureRightShift   = (color == WHITE) ? 9 : -7;

    addPawnBitboardMovesToList(moves, singleMoves, forwardSingleShift);
    addPawnBitboardMovesToList(moves, doubleMoves, forwardDoubleShift);
    addPawnBitboardMovesToList(moves, capturesLeft, captureLeftShift);
    addPawnBitboardMovesToList(moves, capturesRight, captureRightShift);
}

void Chess::addPawnBitboardMovesToList(std::vector<BitMove>& moves, const BitboardElement bitboard, const int shift){
    if(bitboard.getData() == 0) return;
    bitboard.forEachBit([&](int to){
        int from = to - shift;
        moves.emplace_back(from, to, Pawn);
    });
}

void Chess::generateKingMoves(const char *state, std::vector<BitMove> &moves, int row, int col){
    std::vector<std::pair<int, int>> directions = {
        {1,0}, {-1,0}, {0,1}, {0, -1}, {1,1}, {1,-1}, {-1,1}, {-1,-1}
    };

    for(auto &dir : directions){
        int newRow = row + dir.first;
        int newCol = col + dir.second;

        if(newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8){
            addMoveIfValid(state, moves, row, col, newRow, newCol);
        }
    }
}