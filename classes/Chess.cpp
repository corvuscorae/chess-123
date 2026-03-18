#include "Chess.h"
#include "Evaluate.h"
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
    _moves = generatePseudoLegalMoves(stateString());

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

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

bool Chess::canBitMoveFrom(Bit& bit, BitHolder& src){
    bool pieceIsWhite = (bit.gameTag() & 128) == 0;
    bool currentIsWhite = (_currentPlayer == WHITE);
    if (pieceIsWhite != currentIsWhite) return false;

    ChessSquare* square = static_cast<ChessSquare*>(&src);
    int idx = square->getSquareIndex();
    for (auto& m : _moves)
        if (m.from == idx) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit&, BitHolder& src, BitHolder& dst){
    int srcSquareIndex = static_cast<ChessSquare*>(&src)->getSquareIndex();
    int dstSquareIndex = static_cast<ChessSquare*>(&dst)->getSquareIndex();
    for (auto& move : _moves){
        if (move.from == srcSquareIndex && move.to == dstSquareIndex){ 
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

// Chess.cpp
// TODO: switch to paranoid king?
bool Chess::isKingInCheck(int player) {
    // find the king index
    char kingChar = (player == WHITE) ? 'K' : 'k';
    std::string state = stateString();
    int kingIndex = -1;
    for (int i = 0; i < 64; i++) {
        if (state[i] == kingChar) {
            kingIndex = i;
            break;
        }
    }
    if (kingIndex == -1) return false; // shouldnt happen

    // switch to opponent and generate their moves
    int savedPlayer = _currentPlayer;
    _currentPlayer = (player == WHITE) ? BLACK : WHITE;
    std::vector<BitMove> opponentMoves = generatePseudoLegalMoves(state);
    _currentPlayer = savedPlayer;   // switch back

    // if any opponent move lands on the king's square, king is in check
    for (auto& move : opponentMoves) {
        if (move.to == kingIndex) return true;
    }
    return false;
}

Player* Chess::checkForWinner() {
    // current player has no moves and is in check --> checkmate
    if (_moves.empty() || isKingInCheck(_currentPlayer)) {
        // OTHER player wins
        int winnerIndex = (_currentPlayer == WHITE) ? BLACK : WHITE;
        return getPlayerAt(winnerIndex);
    }
    return nullptr;
}

bool Chess::checkForDraw() {
    // no legal moves but king is NOT in check
    if (_moves.empty() && !isKingInCheck(_currentPlayer)) {
        return true;
    }
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

        // destroy whatever is currently on the square first
        //      (frees the old bit, prevent mem leaks)
        square->destroyBit();
        
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
    _moves = generatePseudoLegalMoves(stateString());
    clearBoardHighlights();
    endTurn();
}

std::vector<BitMove> Chess::generatePseudoLegalMoves(const std::string& state){
    std::vector<BitMove> moves;
    moves.reserve(40);   

    rebuildBitboards(state);

    // offsets into the bitboard array
    int offset      = (_currentPlayer == WHITE) ? 0 : (B_PAWNS - W_PAWNS);
    int enemyOffset = (_currentPlayer == WHITE) ? (B_PAWNS - W_PAWNS) : 0;

    // get friendly and enemy bitboards
    int friendlyAllIndex = (_currentPlayer == WHITE) ? W_ALL : B_ALL;
    int enemyAllIndex    = (_currentPlayer == WHITE) ? B_ALL : W_ALL;
    uint64_t friendlyBB = _bitboards[friendlyAllIndex].getData();
    uint64_t enemyBB    = _bitboards[enemyAllIndex].getData();

    uint64_t occupancy  = _bitboards[OCCUPANCY].getData();  // all occupied squares
    uint64_t empty      = ~occupancy;                       // all empty squares

    // PAWNS
    generatePawnMoves(moves, _bitboards[W_PAWNS + offset], empty, _bitboards[enemyAllIndex], _currentPlayer);

    // KNIGHTS
    BitboardElement knights = _bitboards[W_KNIGHTS + offset];
    knights.forEachBit([&](int from) {
        BitboardElement canMoveTo = _knightBitboards[from].getData() & ~friendlyBB;
        canMoveTo.forEachBit([&](int to) {
            moves.emplace_back(from, to, Knight);
        });
    });

    // KINGS
    uint64_t kingBoard = _bitboards[W_KING + offset].getData();
    while (kingBoard) {
        int sq = bitScanForward(kingBoard);
        generateKingMoves(state.c_str(), moves, sq / 8, sq % 8);
        kingBoard &= kingBoard - 1;
    }
    // BISHOPS
    uint64_t bishopBoard = _bitboards[W_BISHOPS + offset].getData();
    while (bishopBoard) {
        int sq = bitScanForward(bishopBoard);
        generateBishopMoves(state.c_str(), moves, sq / 8, sq % 8);
        bishopBoard &= bishopBoard - 1;
    }

    // ROOKS
    uint64_t rookBoard = _bitboards[W_ROOKS + offset].getData();
    while (rookBoard) {
        int sq = bitScanForward(rookBoard);
        generateRookMoves(state.c_str(), moves, sq / 8, sq % 8);
        rookBoard &= rookBoard - 1;
    }

    // QUEENS
    uint64_t queenBoard = _bitboards[W_QUEENS + offset].getData();
    while (queenBoard) {
        int sq = bitScanForward(queenBoard);
        generateQueenMoves(state.c_str(), moves, sq / 8, sq % 8);
        queenBoard &= queenBoard - 1;
    }

    return moves;
}

void Chess::rebuildBitboards(const std::string& state)
{
    for (int i = 0; i < e_numBitboards; i++) _bitboards[i] = 0;
    for (int i = 0; i < 64; i++) {
        char c = state[i];
        int bbIdx = _bitboardLookup[(unsigned char)c];
        _bitboards[bbIdx] |= 1ULL << i;
        if (c != '0') {
            _bitboards[OCCUPANCY] |= 1ULL << i;
            _bitboards[isupper(c) ? W_ALL : B_ALL] |= 1ULL << i;
        }
    }
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

    generateLinearMoves(state, moves, row, col, diagonals, Bishop);
}

void Chess::generateRookMoves(const char* state, std::vector<BitMove>& moves, int row, int col){
    static const std::vector<std::pair<int, int>> orthogonals = {
        {1,0}, {-1, 0}, {0, 1}, {0, -1}
    };

    generateLinearMoves(state, moves, row, col, orthogonals, Rook);
}

void Chess::generateQueenMoves(const char* state, std::vector<BitMove>& moves, int row, int col){
    static const std::vector<std::pair<int, int>> all_dirs = {
        {1,0}, {-1, 0}, {0, 1}, {0, -1},
        {1,1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    generateLinearMoves(state, moves, row, col, all_dirs, Queen);
}

void Chess::generateLinearMoves(const char* state, std::vector<BitMove>& moves, int row, int col, const std::vector<std::pair<int, int>> directions, ChessPiece pieceType){
    for( auto &dir : directions){
        int currRow = row + dir.first;
        int currCol = col + dir.second;

        while(currRow >= 0 && currRow < 8 && currCol >= 0 && currCol < 8){
            if(pieceNotation(currCol, currRow) != '0'){
                addMoveIfValid(state, moves, row, col, currRow, currCol, pieceType);
                break;
            }

            addMoveIfValid(state, moves, row, col, currRow, currCol, pieceType);
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

void Chess::addMoveIfValid(const char* state, std::vector<BitMove>& moves, int fromRow, int fromCol, int toRow, int toCol, ChessPiece pieceType){
    if(toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8){
        int fromColor = stateColor(state, fromRow, fromCol);
        int toColor = stateColor(state, toRow, toCol);
        if(fromColor != toColor){
            moves.emplace_back(fromRow*8+fromCol, toRow*8+toCol, pieceType);
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
            addMoveIfValid(state, moves, row, col, newRow, newCol, King);
        }
    }
}

// AI
// apply a move to state string
void Chess::applyMoveToState(std::string& state, const BitMove& move) const {
    int from = move.from, to = move.to;
    char piece = state[from];

    state[to]   = piece;
    state[from] = '0';
}

int Chess::negamax(const std::string& state, int depth, int alpha, int beta, int player){
    if (depth == 0) {
        int val = evaluateBoard(state.c_str());
        return (player == WHITE) ? val : -val;
    }

    // set search context so generators work
    int savedPlayer     = _currentPlayer;
    _currentPlayer      = player;
    rebuildBitboards(state);

    std::vector<BitMove> pseudo = generatePseudoLegalMoves(state);

    // filter to legal (don't leave own king in check)
    std::vector<BitMove> moves;
    moves.reserve(pseudo.size());
    int kingBBIndex = (player == WHITE) ? W_KING : B_KING;
    int oppPlayer = (player == WHITE) ? BLACK : WHITE;
    for (auto& move : pseudo) {
        std::string next = state;
        applyMoveToState(next, move);   // simulate move on state string

        // enemy moves
        _currentPlayer = oppPlayer;
        rebuildBitboards(next);
        std::vector<BitMove> oppMoves = generatePseudoLegalMoves(next);


        _currentPlayer = player;
        rebuildBitboards(next);

        uint64_t kbb = _bitboards[kingBBIndex].getData();
        if (!kbb) continue;

        // check if king is in check
        int kIndex = bitScanForward(kbb);
        bool inCheck = false;
        for (auto& om : oppMoves){ 
            if (om.to == kIndex) { 
                inCheck = true; 
                break;                          // illegal move
            }
        }
        
        if (!inCheck) moves.push_back(move);    // legal move
    }

    // RESTORE
    _currentPlayer = savedPlayer;

    if (moves.empty()) {
        _currentPlayer = oppPlayer;
        rebuildBitboards(state);
        std::vector<BitMove> attack = generatePseudoLegalMoves(state);

        _currentPlayer = savedPlayer;
        rebuildBitboards(state);
        
        // check if king in check
        uint64_t kingBB = _bitboards[kingBBIndex].getData();
        int king = kingBB ? bitScanForward(kingBB) : -1;
        bool inCheck = false;
        for (auto& atkMove : attack) {
            if (atkMove.to == king) { 
                inCheck = true; 
                break;                          // BAD
            }
        }
        
        return inCheck ? (-100000 + depth) : 0; // punish king in check
    }

    int best = -200000;
    for (auto& move : moves) {
        std::string next = state;
        applyMoveToState(next, move);

        int score = -negamax(next, depth - 1, -beta, -alpha, oppPlayer);

        // AB pruning
        if (score > best)  best  = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }
    return best;
}

void Chess::updateAI()
{
    constexpr int AI_DEPTH = 4;
    constexpr int NEG_INF  = -200000;
    constexpr int POS_INF  =  200000;

    // snapshot
    std::string rootState = stateString();
    int         rootPlayer = _currentPlayer;

    std::vector<BitMove> moves = generatePseudoLegalMoves(rootState);
    if (moves.empty()) return;

    int     bestScore = NEG_INF;
    BitMove bestMove  = moves[0];
    int     oppPlayer = (rootPlayer == WHITE) ? BLACK : WHITE;

    for (auto& move : moves) {
        std::string next = rootState;
        applyMoveToState(next, move);

        int score = -negamax(next, AI_DEPTH - 1, NEG_INF, POS_INF, oppPlayer);

        if (score > bestScore) {
            bestScore = score;
            bestMove  = move;
        }
    }

    // RESTORE
    _currentPlayer   = rootPlayer;
    setStateString(rootState);

    // APPLY BEST MOVE
    applyMoveToState(rootState, bestMove);
    setStateString(rootState);

    _currentPlayer = (_currentPlayer == WHITE) ? BLACK : WHITE;
    _moves         = generatePseudoLegalMoves(stateString());
    clearBoardHighlights();
    endTurn();
}