#ifndef BOARD_H
#define BOARD_H

#include "types.h"

/* Sets up all 40 squares and all 28 properties (Table 1, Appendix B).
 * Call this once, right after your GameState is declared. */
void initializeBoard(GameState *game);

/* Moves a player forward by diceTotal squares (wrapping past 39 back
 * to 0), handles GO money (Rule 4), and prints the movement messages.
 * Returns the player's new square index.
 *
 * Does NOT resolve what happens on the landed square -- that's a
 * separate step (see resolveLanding below), because what happens next
 * depends on systems you haven't built yet (buying, rent, banking...). */
int movePlayer(GameState *game, int playerIndex, int diceTotal);

/* Dispatches to the correct handler based on game->board[position].type
 * (buy property / pay rent / draw event card / pay tax / go to jail /
 * bank menu / insurance menu / free parking / just visiting).
 *
 * NOT implemented in board.c -- this belongs in game.c once the other
 * subsystems (players.c, finance.c, events.c) exist to back it up.
 * Declared here so board.c/movePlayer's caller can compile against it;
 * define the real version in game.c. */
void copyName(char *dest, size_t destSize, const char *src);

#endif /* BOARD_H */
