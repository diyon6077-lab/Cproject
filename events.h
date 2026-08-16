#ifndef EVENTS_H
#define EVENTS_H

#include "types.h"

/* Fills all 20 slots of game->nationalDeck (Appendix A) and resets
 * game->nextCardIndex to 0. Call this once from startGame(), before
 * the main simulation loop begins. */
void initializeNationalDeck(GameState *game);

/* Draws the "top" card, prints it, applies its effect (dispatches
 * internally based on the card's type), then advances the deck
 * pointer so the next card becomes the new top -- see the comment
 * inside drawNationalEventCard() in events.c for why this correctly
 * simulates "return the card to the bottom of the deck" without
 * physically moving any array elements. */
void drawNationalEventCard(GameState *game, int playerIndex);

#endif /* EVENTS_H */
