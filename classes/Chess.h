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
    int _currentPlayer;

    Grid* _grid;

    // generating moves
    std::vector<BitMove> generateAllMoves();
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst);

    // knights
    BitboardElement generateKnightMoveBitboard(int square);
    void generateKnightMoves(std::vector<BitMove>& moves, std::string &state);
    void generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knights, uint64_t occupancy);

    // bishops
    void generateBishopMoves(const char* state, std::vector<BitMove>& moves, int row, int col);
    
    std::vector<BitMove> _moves;
    BitboardElement _knightBitboards[64];
    BitboardElement _bitboards[e_numBitboards];
    int _bitboardLookup[128];

};