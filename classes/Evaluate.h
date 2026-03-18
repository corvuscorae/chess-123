#pragma once
#include "Chess.h"

// mirroring 
#ifndef FLIP
#define FLIP(i) ((7 - (i)/8)*8 + (i)%8)
#endif

/* piece/sq tables */
// piece square tables for every piece (from chess programming wiki)
const int pawnTable[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5, 5, 10, 25, 25, 10, 5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, -5, -10, 0, 0, -10, -5, 5,
    5, 10, 10, -20, -20, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0};
const int knightTable[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 0, 15, 20, 20, 15, 0, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50};
const int rookTable[64] = {
    0, 0, 0, 5, 5, 0, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    5, 10, 10, 10, 10, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0};
const int queenTable[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 5, 5, 5, 0, -10,
    -5, 0, 5, 5, 5, 5, 0, -5,
    0, 0, 5, 5, 5, 5, 0, -5,
    -10, 5, 5, 5, 5, 5, 0, -10,
    -10, 0, 5, 0, 0, 0, 0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20};
const int kingTable[64] = {
    20, 30, 10, 0, 0, 10, 30, 20,
    20, 20, 0, 0, 0, 0, 20, 20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30};
const int bishopTable[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 10, 10, 5, 0, -10,
    -10, 5, 5, 10, 10, 5, 5, -10,
    -10, 0, 10, 10, 10, 10, 0, -10,
    -10, 10, 10, 10, 10, 10, 10, -10,
    -10, 5, 0, 0, 0, 0, 5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20};


static int materialValue(char piece) {
    switch (piece) {
        case 'P': return  100;
        case 'N': return  320;
        case 'B': return  330;
        case 'R': return  500;
        case 'Q': return  900;
        case 'K': return 20000;
        case 'p': return -100;
        case 'n': return -320;
        case 'b': return -330;
        case 'r': return -500;
        case 'q': return -900;
        case 'k': return -20000;
        default:  return  0;
    }
}

int Chess::evaluateBoard(const char* state) {
    int score = 0;

    for (int i = 0; i < 64; i++) {
        char piece = state[i];
        if (piece == '0') continue;

        // ── Material ──────────────────────────────────────────
        score += materialValue(piece);

        // ── Piece-square table bonus ──────────────────────────
        // j = index into the PST table.
        // White pieces look up [i] directly (a1-centric).
        // Black pieces mirror vertically via FLIP so that
        // "advancing toward the opponent" still means higher bonus.
        int j = isupper(piece) ? i : FLIP(i);
        int sign = isupper(piece) ? 1 : -1;

        switch (tolower(piece)) {
            case 'p': score += sign * pawnTable[j];   break;
            case 'n': score += sign * knightTable[j]; break;
            case 'b': score += sign * bishopTable[j]; break;
            case 'r': score += sign * rookTable[j];   break;
            case 'q': score += sign * queenTable[j];  break;
            case 'k': score += sign * kingTable[j];   break;
            default: break;
        }
    }
    return score;
}
