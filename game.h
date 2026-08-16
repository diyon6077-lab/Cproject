#ifndef GAME_H
#define GAME_H

#include "types.h"

/* The ONLY function main.c calls. Owns the real GameState (declares
 * it as a local variable), seeds the RNG, initializes the board,
 * runs the whole simulation loop, and prints the final result. */
void startGame(void);

/* Rolls two six-sided dice. Writes each individual die into die1/die2
 * (needed for doubles checks, e.g. Rule 13) and returns the total. */
int rollDice(int *die1, int *die2);

/* Runs one player's full turn: jail check (Rule 13), dice roll,
 * movement, landing resolution. Rule 3's numbered sequence. */
void playTurn(GameState *game, int playerIndex);

/* Dispatches to the correct handler based on game->board[position].type
 * (buy property / pay rent / draw event card / pay tax / go to jail /
 * bank menu / insurance menu / free parking / just visiting). Needs
 * diceTotal because utility rent depends on the roll (Table 8). */
void resolveLanding(GameState *game, int playerIndex, int diceTotal);

/* Runs once per round, after every player has had a turn: loan
 * interest (Rule-LK 4), property aging/depreciation (Rule-LK 15, 16),
 * building condition decay (Rule-LK 25), insurance expiry (Rule-LK 9),
 * and economic timers (inflation, market, regulations, regional cards). */
void processEndOfRound(GameState *game);

/* Prints the "Round N Summary" block for all 4 players (Section 5). */
void printRoundSummary(const GameState *game);

/* Returns how many players are not yet bankrupt -- used as the main
 * loop's stopping condition alongside MAX_ROUNDS (Rule 15). */
int countSolventPlayers(GameState *game);

/* Fills in one player's starting state (Rule 1) and prints the
 * "Player N : Name" line (Section 5, "Before the Game Begins"). */
void setPlayer(GameState *game, int index, const char *name, Strategy strategy, int cash, int position);

/* Calls setPlayer() for all 4 players with their names/strategies. */
void initPlayers(GameState *game);

/* Rule 2: everyone rolls, highest total goes first (ties reroll),
 * then fills in game->turnOrder[] clockwise from the winner. */
void determineTurnOrder(GameState *game);

#endif /* GAME_H */
