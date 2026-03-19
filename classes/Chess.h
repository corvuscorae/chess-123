#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

constexpr int pieceSize = 80;
constexpr int WHITE = 0;
constexpr int BLACK = 1;

enum AllBitBoards {
    W_PAWNS,
    W_KNIGHTS,
    W_BISHOPS,
    W_ROOKS,
    W_QUEENS,
    W_KING,
    B_PAWNS,
    B_KNIGHTS,
    B_BISHOPS,
    B_ROOKS,
    B_QUEENS,
    B_KING,
    W_ALL,
    B_ALL,
    OCCUPANCY,
    EMPTY_SQUARES,
    e_numBitboards
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    // player colors
    int stateColor(const char* state, int row, int col);
    int _currentPlayer;
    Grid* _grid;

    // generating moves
    void rebuildBitboards(const std::string& state);
    std::vector<BitMove> generatePseudoLegalMoves(const std::string& state);
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst);
    void addMoveIfValid(const char* state, std::vector<BitMove>& moves, int fromRow, int fromCol, int toRow, int toCol, ChessPiece pieceType);


    // knights
    BitboardElement generateKnightMoveBitboard(int square);

    // bishops
    // TODO: improve
    // use templates?
    void generateBishopMoves(const char* state, std::vector<BitMove>& moves, int row, int col);
    void generateRookMoves(const char* state, std::vector<BitMove>& moves, int row, int col);
    void generateQueenMoves(const char* state, std::vector<BitMove>& moves, int row, int col);
    void generateLinearMoves(const char* state, std::vector<BitMove>& moves,
                                    int row, int col,
                                    const std::pair<int,int>* directions, int numDirs,
                                    ChessPiece pieceType);

    // pawns
    void generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawns, const BitboardElement empty, const BitboardElement enemies, char col);
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves, const BitboardElement bitboard, const int shift);

    // kings
    void generateKingMoves(const char *state, std::vector<BitMove> &moves, int row, int col);
    bool isKingInCheck(int player);


    std::vector<BitMove> _moves;
    BitboardElement _knightBitboards[64];
    BitboardElement _bitboards[e_numBitboards];
    int _bitboardLookup[128];

    inline int bitScanForward(uint64_t bb) const {
        #if defined(_MSC_VER) && !defined(__clang__)
                unsigned long index;
                _BitScanForward64(&index, bb);
                return index;
        #else
                return __builtin_ffsll(bb) - 1;
        #endif
    };

    // AI
    bool gameHasAI() override { return true; }

    void updateAI();
    int negamax(const std::string& state, int depth, int alpha, int beta, int player);
    void applyMoveToState(std::string& state, const BitMove& move) const;
    int evaluateBoard(const char* state);
    std::vector<BitMove> generateLegalMoves();
    bool isMoveLegal(const BitMove& move);

    int getColorFromState(const char* state, int row, int col);
    void applyMoveToBoard(const BitMove& move);

};

