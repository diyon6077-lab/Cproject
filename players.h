#ifndef PLAYERS_H
#define PLAYERS_H

#include "types.h"

/* Should playerIndex buy the unowned property at propIndex, if
 * offered at its listed price (Rule 5)? Dispatches based on the
 * player's Strategy (Section 3) -- see players.c for the per-
 * strategy rules and which ones are direct spec quotes vs our own
 * documented assumptions. */
bool playerDecideToPurchase(const GameState *game, int playerIndex, int propIndex);

/* Should a jailed player pay bail RIGHT NOW instead of attempting
 * doubles this turn? NOTE: this decision isn't described anywhere
 * in the spec (Rule 13 lists the three exit conditions but never
 * says which one each Strategy prefers) -- this is our own design
 * choice, documented in players.c, not a rule citation. */
bool playerDecideToPayBailVoluntarily(const GameState *game, int playerIndex);

#endif /* PLAYERS_H */
