#include "Chess.h"
#include "Evaluate.h"

#include <limits>
#include <cmath>
#include <cctype>
#include <algorithm>

Chess::Chess()
{
    _grid = new Grid(8, 8);

    for(int i = 0; i < 64; i++){
        _knightBitboards[i] = generateKnightMoveBitboard(i);
    }
    for (int i = 0; i < 128; i++){
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
    _moves = generateLegalMoves();

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

    startGame();
}

void Chess::FENtoBoard(const std::string& fen)
{
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
    int savedPlayer = _currentPlayer;
    _currentPlayer = (player == WHITE) ? BLACK : WHITE;

    std::string state = stateString();
    rebuildBitboards(state);

    int kingBBIdx = (player == WHITE) ? W_KING : B_KING;
    uint64_t kingBB = _bitboards[kingBBIdx].getData();
    if (kingBB == 0) { _currentPlayer = savedPlayer; return false; }
    int kingIndex = bitScanForward(kingBB);

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
        char c = s[index];

        // destroy whatever is currently on the square first
        //      (frees the old bit, prevent mem leaks)
        square->destroyBit();

        if (c == '0') return;

        int player = isupper(c) ? WHITE : BLACK;
        char lc = (char)tolower(c);

        ChessPiece piece = NoPiece;
        if      (lc == 'p') piece = Pawn;
        else if (lc == 'n') piece = Knight;
        else if (lc == 'b') piece = Bishop;
        else if (lc == 'r') piece = Rook;
        else if (lc == 'q') piece = Queen;
        else if (lc == 'k') piece = King;

        if (piece != NoPiece) {
            Bit* b = PieceForPlayer(player, piece);
            int tag = (player == BLACK) ? (piece + 128) : piece;
            b->setGameTag(tag);
            b->setPosition(square->getPosition());
            b->setParent(square);
            square->setBit(b);
        }
    });
}

// MOVE GENERATIONS //
void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst){
    int from = static_cast<ChessSquare*>(&src)->getSquareIndex();
    int to   = static_cast<ChessSquare*>(&dst)->getSquareIndex();

    BitMove playedMove(from, to, NoPiece);
    for (auto& m : _moves) {
        if (m.from == from && m.to == to) { playedMove = m; break; }
    }

    _currentPlayer = (_currentPlayer == WHITE) ? BLACK : WHITE;
    _moves = generateLegalMoves();
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

BitboardElement Chess::generateKnightMoveBitboard(int square)
{
    static const std::pair<int,int> offsets[] = {
        {2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}
    };
    
    BitboardElement bb = 0ULL;
    int rank = square / 8, file = square % 8;
    for (auto [dr, df] : offsets) {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8)
            bb |= 1ULL << (r * 8 + f);
    }
    return bb;
}

void Chess::generateBishopMoves(const char* state, std::vector<BitMove>& moves, int row, int col)
{
    static const std::pair<int,int> diagonals[] = {
        {1,1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    generateLinearMoves(state, moves, row, col, diagonals, 4, Bishop);
}

void Chess::generateRookMoves(const char* state, std::vector<BitMove>& moves, int row, int col)
{
    static const std::pair<int,int> orthogonals[] = {
        {1,0}, {-1, 0}, {0, 1}, {0, -1}
    };
    generateLinearMoves(state, moves, row, col, orthogonals, 4, Rook);
}

void Chess::generateQueenMoves(const char* state, std::vector<BitMove>& moves, int row, int col)
{
    static const std::pair<int,int> all_dirs[] = {
        {1,0}, {-1, 0}, {0, 1}, {0, -1},
        {1,1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    generateLinearMoves(state, moves, row, col, all_dirs, 8, Queen);
}

void Chess::generateKingMoves(const char* state, std::vector<BitMove>& moves, int row, int col)
{
    static const std::pair<int,int> dirs[] = {
        {1,0}, {-1,0}, {0,1}, {0,-1},
        {1,1}, {1,-1}, {-1,1}, {-1,-1}
    };
    for (auto& [dr, dc] : dirs)
        addMoveIfValid(state, moves, row, col, row + dr, col + dc, King);
}

void Chess::generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawns, const BitboardElement empty, const BitboardElement enemies, char color){
    if(pawns.getData() == 0){
        return;
    }
    
    // constants for ranks and files
    constexpr uint64_t notAFile (0xFEFEFEFEFEFEFEFEULL);
    constexpr uint64_t notHFile (0x7F7F7F7F7F7F7F7FULL);
    constexpr uint64_t rank3    (0x0000000000FF0000ULL);
    constexpr uint64_t rank6    (0x0000FF0000000000ULL);

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

    auto addPawnMoves = [&](BitboardElement bitboard, int shift) {
        if (!bitboard.getData()) return;
        bitboard.forEachBit([&](int to){
            int from = to - shift;
            moves.emplace_back(from, to, Pawn);
        });
    };

    addPawnMoves(singleMoves,   forwardSingleShift);
    addPawnMoves(doubleMoves,   forwardDoubleShift);
    addPawnMoves(capturesLeft,  captureLeftShift);
    addPawnMoves(capturesRight, captureRightShift);
}

void Chess::generateLinearMoves(
    const char* state, std::vector<BitMove>& moves,
    int row, int col, const std::pair<int,int>* directions, 
    int numDirs, ChessPiece pieceType
){
    for (int d = 0; d < numDirs; d++) {
        int dr = directions[d].first, dc = directions[d].second;
        int r = row + dr, c = col + dc;
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            char target = state[r * 8 + c];
            addMoveIfValid(state, moves, row, col, r, c, pieceType);
            if (target != '0') break;
            r += dr; c += dc;
        }
    }
}

int Chess::negamax(const std::string& state, int depth, int alpha, int beta, int player)
{
    if (depth == 0) {
        int raw = evaluateBoard(state.c_str());
        return (player == WHITE) ? raw : -raw;
    }

    int savedPlayer = _currentPlayer;
    _currentPlayer  = player;
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
    int         oppPlayer  = (rootPlayer == WHITE) ? BLACK : WHITE;

    std::vector<BitMove> moves = generateLegalMoves();
    if (moves.empty()) return;

    int     bestScore = NEG_INF;
    BitMove bestMove  = moves[0];

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
    _currentPlayer = rootPlayer;
    setStateString(rootState);

    // APPLY BEST MOVE
    applyMoveToBoard(bestMove);

    _currentPlayer = (_currentPlayer == WHITE) ? BLACK : WHITE;
    _moves         = generateLegalMoves();
    clearBoardHighlights();
    endTurn();
}

std::vector<BitMove> Chess::generateLegalMoves()
{
    std::vector<BitMove> pseudo = generatePseudoLegalMoves(stateString());
    std::vector<BitMove> legal;
    legal.reserve(pseudo.size());
    for (auto& move : pseudo) {
        if (isMoveLegal(move)) legal.push_back(move);
    }
    return legal;
}

bool Chess::isMoveLegal(const BitMove& move)
{
    std::string state = stateString();
    applyMoveToState(state, move);

    int saved = _currentPlayer;
    _currentPlayer = (saved == WHITE) ? BLACK : WHITE;
    rebuildBitboards(state);
    std::vector<BitMove> opMoves = generatePseudoLegalMoves(state);
    _currentPlayer = saved;

    int kingBBIdx = (saved == WHITE) ? W_KING : B_KING;
    rebuildBitboards(state);
    uint64_t kingBB = _bitboards[kingBBIdx].getData();
    if (kingBB == 0) return false;
    int kingIdx = bitScanForward(kingBB);

    for (auto& m : opMoves)
        if (m.to == kingIdx) return false;
    return true;
}

// update state string
void Chess::applyMoveToState(std::string& state, const BitMove& move) const
{
    int from = move.from, to = move.to;
    char piece = state[from];

    state[to]   = piece;
    state[from] = '0';

}

// update game board
void Chess::applyMoveToBoard(const BitMove& move)
{
    std::string state = stateString();
    applyMoveToState(state, move);
    setStateString(state);
}

int Chess::getColorFromState(const char* state, int row, int col)
{
    char piece = state[row * 8 + col];
    if (piece == '0') return -1;
    return isupper(piece) ? WHITE : BLACK;
}

void Chess::addMoveIfValid(
    const char* state, std::vector<BitMove>& moves,
    int fromRow, int fromCol, int toRow, int toCol,
    ChessPiece pieceType
){
    if (toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8) return;
    int fromColor = getColorFromState(state, fromRow, fromCol);
    int toColor   = getColorFromState(state, toRow,   toCol);
    if (fromColor != toColor)
        moves.emplace_back(fromRow * 8 + fromCol, toRow * 8 + toCol, pieceType);
}