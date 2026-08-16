#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "types.h"
#include "board.h"
#include "events.h"
#include "players.h"
#include "game.h"

int countSolventPlayers(GameState *game) {
    int solventCount = 0;
    
    for (int i = 0; i < NUM_PLAYERS; i++) {
        // If the player is NOT bankrupt, increment the counter
        if (!game->players[i].bankrupt) {
            solventCount++;
        }
    }
    
    return solventCount;
}


void setPlayer(GameState *game, int index, const char *name, Strategy strategy, int cash, int position) {
    if (index < 0 || index >= NUM_PLAYERS) return;

    Player *p = &game->players[index];
    copyName(p->name, sizeof(p->name), name);
    p->strategy = strategy;
    p->cash = cash;
    p->position = position;

    p->inJail = false;
    p->jailTurnsServed = 0;
    p->bankrupt = false;

    for (int i = 0; i < NUM_PROPERTIES; i++) {
        p->ownedProperties[i] = -1;
    }
    p->numOwnedProperties = 0;
    p->loan.active = false;
    p->activeEventEffect= EVENT_NONE;
    p->eventEffectExpiryRound= 0;

    printf("Player %d : %s\n", index + 1, p->name);
}

void initPlayers(GameState *game){
    setPlayer(game, 0, "Aggressive Investor",   STRAT_AGGRESSIVE_INVESTOR,   STARTING_CASH, 0);
    setPlayer(game, 1, "Conservative Banker",   STRAT_CONSERVATIVE_BANKER,   STARTING_CASH, 0);
    setPlayer(game, 2, "Risk Taker",            STRAT_RISK_TAKER,            STARTING_CASH, 0);
    setPlayer(game, 3, "Opportunistic Trader",  STRAT_OPPORTUNISTIC_TRADER,  STARTING_CASH, 0);
}

//rolldice function
int rollDice(int *die1, int *die2) {
    *die1 = (rand() % 6) + 1;   // random number 0-5, shifted to 1-6
    *die2 = (rand() % 6) + 1;
    return *die1 + *die2;
}

//determine turn order
void determineTurnOrder(GameState *game) {
    bool stillContending[NUM_PLAYERS];
    for (int i = 0; i < NUM_PLAYERS; i++) stillContending[i] = true;

    int winner = -1;

    while (winner == -1) {
        int totals[NUM_PLAYERS];
        int highest = -1;
        int numAtHighest = 0;

        // only players still in the running roll this pass
        for (int i = 0; i < NUM_PLAYERS; i++) {
            if (!stillContending[i]) continue;
            int d1, d2;
            totals[i] = rollDice(&d1, &d2);
            printf("%s rolls %d.\n", game->players[i].name, totals[i]);
        }

        // find the highest total among contenders, and how many share it
        for (int i = 0; i < NUM_PLAYERS; i++) {
            if (!stillContending[i]) continue;
            if (totals[i] > highest) {
                highest = totals[i];
                numAtHighest = 1;
            } else if (totals[i] == highest) {
                numAtHighest++;
            }
        }

        if (numAtHighest == 1) {
            // clear winner
            for (int i = 0; i < NUM_PLAYERS; i++) {
                if (stillContending[i] && totals[i] == highest) {
                    winner = i;
                }
            }
        } else {
            // tie: eliminate everyone who did NOT hit the highest total;
            // the tied players loop back around and roll again
            for (int i = 0; i < NUM_PLAYERS; i++) {
                if (stillContending[i] && totals[i] != highest) {
                    stillContending[i] = false;
                }
            }
        }
    }

    printf("%s will begin the game.\n\n", game->players[winner].name);

    // build the clockwise turn order starting from the winner
    for (int i = 0; i < NUM_PLAYERS; i++) {
        game->turnOrder[i] = (winner + i) % NUM_PLAYERS;
    }
}

void playTurn(GameState *game, int playerIndex) {
    Player *p = &game->players[playerIndex];
 
    if (p->bankrupt) return;   /* safety guard; caller should already skip */
 
    /* --- Step 1: Resolve outstanding penalties (jail, Rule 13) --- */
    if (p->inJail) {

        /* Rule 13 lists three exit conditions but doesn't say which
         * a player prefers -- that choice is Strategy-dependent (our
         * own design, see players.c's playerDecideToPayBailVoluntarily
         * for exactly which strategies pay early and why). Checking
         * this FIRST means a player who wants to pay does so before
         * ever attempting doubles. */
        if (playerDecideToPayBailVoluntarily(game, playerIndex) && p->cash >= BAIL_AMOUNT) {
            p->cash -= BAIL_AMOUNT;
            p->inJail = false;
            p->jailTurnsServed = 0;
            printf("%s pays bail of LKR %d to leave jail early.\n", p->name, BAIL_AMOUNT);
            /* Falls through to the normal roll-and-move below --
             * paying bail doesn't cost the player their turn. */

        } else {
            int d1, d2;
            int total = rollDice(&d1, &d2);
            printf("%s rolled %d and %d while in jail.\n", p->name, d1, d2);

            if (d1 == d2) {
                printf("%s rolled doubles and is released from jail.\n", p->name);
                p->inJail = false;
                p->jailTurnsServed = 0;
                /* Player gets out AND uses this same roll to move (Rule 13). */
                movePlayer(game, playerIndex, total);
                resolveLanding(game, playerIndex, total);
                return;
            }

            p->jailTurnsServed++;

            if (p->jailTurnsServed >= MAX_JAIL_TURNS) {
                printf("%s has served the maximum jail term.\n", p->name);
                if (p->cash >= BAIL_AMOUNT) {
                    p->cash -= BAIL_AMOUNT;
                    printf("%s paid bail of LKR %d.\n", p->name, BAIL_AMOUNT);
                }
                p->inJail = false;
                p->jailTurnsServed = 0;
            } else {
                printf("%s remains in jail.\n", p->name);
            }
            return;   /* did not roll-to-move this turn */
        }
    }
 
    /* --- Steps 2 & 3: Roll dice, move --- */
    int d1, d2;
    int total = rollDice(&d1, &d2);
    printf("%s rolled %d.\n", p->name, total);
    movePlayer(game, playerIndex, total);
 
    /* --- Step 4: Resolve landing action --- */
    resolveLanding(game, playerIndex,total);
 
    /* --- Steps 5-7: purchase / construct / financial transactions ---
     * TODO: these depend on players.c (strategy-driven decisions) and
     * finance.c (loan/insurance actions), which don't exist yet.
     * resolveLanding() will handle the "must happen automatically"
     * parts (paying rent, drawing cards); the "player CHOOSES to do X"
     * parts (buy this property? build a house? repay the loan?) belong
     * here once those systems are built. */
 
    /* --- Step 8: End turn (implicit -- function returns) --- */
}

/* ------------------------------------------------------------
 * RENT CALCULATION (Table 3, Table 6, Table 7, Table 8, Rule 7,
 * Rule-LK 11, Rule-LK 26)
 * ------------------------------------------------------------ */
 
/* Table 6: rent multiplier by development level (indices match the
 * DevelopmentLevel enum order in types.h). */
static const int rentMultiplier[6] = {1, 2, 3, 5, 7, 10};
 
/* Table 2 / Table 7: railway rent by how many stations one player
 * owns. Index 0 is unused (a player never gets rent for owning 0). */
static const int railwayRentByCount[5] = {0, 250, 500, 1000, 2000};
 
/* Table 3: what percentage of full rent a building collects, based
 * on its condition. Returns 0 for "Building Closed" (below 25%). */
static int conditionRentPercent(int condition) {
    if (condition < 25) return 0;
    if (condition < 50) return 50;
    if (condition < 75) return 75;
    if (condition < 90) return 90;
    return 100;
}

/* How many properties of a given category (railway/utility/colour)
 * a specific player owns -- needed for railway/utility rent, which
 * depends on COUNT owned, not on the single property landed on. */
static int countOwnedInCategory(const GameState *game, int ownerIndex,
                                 PropertyCategory category) {
    if (ownerIndex < 0) return 0;   /* Bank owns nothing */
    const Player *owner = &game->players[ownerIndex];
    int count = 0;
    for (int i = 0; i < owner->numOwnedProperties; i++) {
        const Property *prop = &game->properties[owner->ownedProperties[i]];
        if (prop->category == category) count++;
    }
    return count;
}

static int calculateRent(const GameState *game, int propIndex, int diceTotal) {
    const Property *prop = &game->properties[propIndex];
    const Player *own = &game->players[prop->owner];

    
    if (prop->mortgaged) return 0;         /* Rule 7 */
    if (prop->disasterDamaged) return 0;   /* Rule-LK 11 */
 
    if (prop->category == CAT_RAILWAY) {
        int owned = countOwnedInCategory(game, prop->owner, CAT_RAILWAY);
        int r = railwayRentByCount[owned];
        if (own->activeEventEffect == EVENT_FUEL_SHORTAGE &&
            game->currentRound < own->eventEffectExpiryRound) {
            r *= 2;   /* EVENT_FUEL_SHORTAGE */
            return r;
        }
        return r;
    }
 
    if (prop->category == CAT_UTILITY) {
        int owned = countOwnedInCategory(game, prop->owner, CAT_UTILITY);
        int multiplier = (owned >= 2) ? 10 : 4;   /* Table 8 */
        if (own->activeEventEffect == EVENT_POWER_FAILURE &&
            game->currentRound < own->eventEffectExpiryRound) {
            multiplier /= 2;   /* EVENT_POWER_FAILURE */
        }
        return diceTotal * multiplier;
    }
 
    /* CAT_COLOUR */
    int rent = prop->currentRent * rentMultiplier[prop->development];
 
    if (prop->development != DEV_NONE) {
        int pct = conditionRentPercent(prop->condition);   /* Rule-LK 26 */
        if (pct == 0) return 0;   /* building closed */
        rent = rent * pct / 100;
    }

    
    if (prop->development == DEV_HOTEL && own->activeEventEffect == EVENT_TOURISM_HYPE &&
        game->currentRound < own->eventEffectExpiryRound) {
        rent *= 2;   /* EVENT_TOURISM_HYPE */
    }

    if (prop->development == DEV_HOTEL && own->activeEventEffect == EVENT_FESTIVAL_SEASON &&
        game->currentRound < own->eventEffectExpiryRound) {
        rent = rent * 150 / 100;   /* EVENT_FESTIVAL_SEASON */
    }
 
    return rent;
}

/* Moves rent from payer to owner and prints the Section 5 messages. */
static void payRent(GameState *game, int payerIndex, int propIndex, int diceTotal) {
    Player *payer = &game->players[payerIndex];
    Property *prop = &game->properties[propIndex];
    Player *owner = &game->players[prop->owner];
 
    printf("%s landed on %s.\n", payer->name, prop->name);
 
    int rent = calculateRent(game, propIndex, diceTotal);
    if (rent == 0) {
        printf("No rent collected.\n");
        return;
    }
 
    printf("Rent Paid : LKR %d.\n", rent);
    printf("Owner : %s.\n", owner->name);
 
    /* TODO: if payer->cash - rent goes negative, this should trigger
     * the debt recovery process (Rule 14) -- selling/mortgaging
     * assets, and ultimately bankruptcy if still insufficient. For
     * now cash is simply allowed to go negative as a visible marker
     * that this case needs handling once finance.c exists. */
    payer->cash -= rent;
    owner->cash += rent;
}

/* ------------------------------------------------------------
 * PURCHASING (Rule 5, Rule 6, Rule-LK 19-23)
 * ------------------------------------------------------------ */

/* NOTE: the purchase decision itself now lives in players.c
 * (playerDecideToPurchase), dispatched by Strategy (Section 3).
 * This file only handles the MECHANICS of buying (moving cash,
 * assigning ownership) -- not the decision of whether to. */

static void buyProperty(GameState *game, int playerIndex, int propIndex) {
    Player *p = &game->players[playerIndex];
    Property *prop = &game->properties[propIndex];
 
    p->cash -= prop->currentPurchasePrice;
    prop->owner = playerIndex;
    p->ownedProperties[p->numOwnedProperties] = propIndex;
    p->numOwnedProperties++;
 
    printf("%s purchased %s for LKR %d.\n", p->name, prop->name, prop->currentPurchasePrice);
    printf("Remaining Balance : LKR %d.\n", p->cash);
}

static void runAuction(GameState *game, int propIndex) {
    const Property *prop = &game->properties[propIndex];
 
    printf("Auction Started.\n");
    printf("Property : %s\n", prop->name);
    printf("Opening Bid : LKR %d.\n", prop->currentPurchasePrice / 2);  /* Rule-LK 19 */
 
    /* TODO: full bidding loop --
     *   - every solvent player may bid, in increments of LKR 250
     *     (Rule 6, Rule-LK 20)
     *   - a player who declines to bid withdraws PERMANENTLY from
     *     this auction (Rule-LK 21)
     *   - the highest remaining bidder wins; nobody may bid more cash
     *     than they currently hold, and loans cannot be taken during
     *     an auction (Rule-LK 22)
     *   - if nobody bids at all, the property stays with the Bank
     *     (Rule-LK 23) -- which is exactly what happens below, since
     *     prop->owner is never reassigned here yet.
     * Whether/how much each player bids is Strategy-dependent
     * (Section 3), so this needs players.c before it can be finished. */
    printf("(Auction bidding not yet implemented -- property remains "
           "unowned by the Bank for now.)\n");
    (void)game;
}

void resolveLanding(GameState *game, int playerIndex, int diceTotal) {
    Player *p = &game->players[playerIndex];
    Square *sq = &game->board[p->position];
 
    switch (sq->type) {
 
        case SQ_PROPERTY:
        case SQ_RAILWAY:
        case SQ_UTILITY: {
            int propIndex = sq->propertyIndex;
            Property *prop = &game->properties[propIndex];
 
            if (prop->owner == -1) {
                /* Unowned -> Rule 5: offer purchase, else Rule 6 auction */
                if (playerDecideToPurchase(game, playerIndex, propIndex)) {
                    buyProperty(game, playerIndex, propIndex);
                } else {
                    runAuction(game, propIndex);
                }
            } else if (prop->owner != playerIndex) {
                /* Owned by someone else -> pay rent (Rule 7) */
                payRent(game, playerIndex, propIndex, diceTotal);
            }
            /* else: landed on own property, nothing required */
            break;
        }
 
        case SQ_TAX: {
            /* Rule 11. See INCOME_TAX_AMOUNT in types.h -- the exact
             * amount is an ASSUMPTION, the spec never states it. */
            printf("%s landed on Income Tax.\n", p->name);
            printf("Tax Paid : LKR %d.\n", INCOME_TAX_AMOUNT);
            p->cash -= INCOME_TAX_AMOUNT;
            /* TODO: "Failure to pay follows the normal debt recovery
             * process" (Rule 11) -- if cash goes negative, trigger
             * Rule 14's debt recovery / bankruptcy check. */
            break;
        }
 
        case SQ_EVENT:
          drawNationalEventCard(game, playerIndex);
            break;
 
        case SQ_GO_TO_JAIL: {
            /* Rule 12: go directly to jail, do NOT collect GO money.
             * We only change p->position directly here (NOT via
             * movePlayer()), so no GO payout is triggered even though
             * this square is conceptually "past" square 39. */
            p->position = 10;   /* Jail / Just Visiting square index */
            p->inJail = true;
            p->jailTurnsServed = 0;
            printf("%s landed on Go To Jail.\n", p->name);
            printf("%s has been sent to Jail.\n", p->name);
            break;
        }
 
        case SQ_BANK: {
            /* TODO: Rule-LK 5 menu -- player may perform ONE of:
             *   - obtain a secured loan (Rule-LK 1-3: needs eligible
             *     collateral; max loan = 75% of total mortgage value
             *     of eligible unmortgaged collateral)
             *   - repay part of the loan
             *   - repay the loan in full
             *   - extend the loan period
             *   - increase the loan amount (subject to collateral)
             * Which action (if any) is Strategy-dependent (Section 3)
             * -- e.g. Conservative Banker repays immediately if funds
             * exist, Risk Taker always maxes out a loan. Belongs in
             * finance.c; see Rule-LK 1-7 for full mechanics
             * (collateral locking, interest, default/foreclosure). */
            printf("%s landed on Bank of Ceylon. "
                   "(Banking system not yet implemented.)\n", p->name);
            break;
        }
 
        case SQ_INSURANCE: {
            /* TODO: Section 1.2 / Rule-LK 8-9 -- player may purchase
             * or renew insurance on ONE owned property. Three policy
             * types (Table 10): Basic (5% premium, 80% compensation),
             * Comprehensive (10% premium, 100% compensation),
             * Business Interruption (15% premium, hotels only). Which
             * property/policy (if any) is Strategy-dependent
             * (Section 3). Belongs in finance.c. */
            printf("%s landed on an Insurance square. "
                   "(Insurance system not yet implemented.)\n", p->name);
            break;
        }
 
        case SQ_START:
        case SQ_JAIL:
        case SQ_FREE_PARKING:
            /* Nothing required -- GO money is already handled inside
             * movePlayer() when passing/landing on square 0; landing
             * on Jail while "Just Visiting" and Free Parking have no
             * effect per the spec. */
            break;
    }
}


void processEndOfRound(GameState *game) {
 
    /* --- Per-property aging, depreciation, condition, insurance --- */
    for (int i = 0; i < NUM_PROPERTIES; i++) {
        Property *prop = &game->properties[i];
 
        /* Rule-LK 15: property age increases every complete round. */
        prop->age++;
 
        /* Rule-LK 16: after 50 rounds without renovation, lose 1%
         * every 5 rounds, capped at 30% total depreciation. */
        if (prop->age > 50 && (prop->age - 50) % 5 == 0
            && prop->depreciationPercent < 30) {
            prop->depreciationPercent += 1;
            /* TODO: recompute prop->currentValue / prop->currentRent
             * from base values + ALL active modifiers (inflation,
             * market boom/decline, depreciation, structural damage)
             * once that combined recalculation function exists. */
        }
 
        /* Rule-LK 25, 26, 28: building condition & maintenance neglect. */
        if (prop->development != DEV_NONE) {
            prop->condition -= 2;
            if (prop->condition < 0) prop->condition = 0;
 
            prop->roundsSinceMaintenance++;
            if (prop->roundsSinceMaintenance > 20 && !prop->structurallyDamaged) {
                prop->structurallyDamaged = true;
                prop->currentValue = prop->currentValue * 85 / 100;  /* -15% */
                prop->currentRent  = prop->currentRent  * 75 / 100;  /* -25% */
                /* TODO: also apply the +50% future maintenance cost
                 * wherever maintenance cost gets calculated (Rule-LK 28). */
            }
        }
 
        /* Rule-LK 9: insurance expiry countdown. */
        if (prop->insurance != INS_NONE) {
            prop->insuranceRoundsRemaining--;
            if (prop->insuranceRoundsRemaining == 3) {
                printf("Insurance policy on %s expires in 3 rounds.\n", prop->name);
            }
            if (prop->insuranceRoundsRemaining <= 0) {
                prop->insurance = INS_NONE;
            }
        }
    }
 
    /* --- Loan interest accrual & duration (Rule-LK 4, 6, 7) --- */
    for (int i = 0; i < NUM_PLAYERS; i++) {
        Player *p = &game->players[i];
        if (p->loan.active) {
            int interest = p->loan.principal * p->loan.interestRatePercent / 100;
            p->loan.principal += interest;
 
            p->loan.roundsRemaining--;
            if (p->loan.roundsRemaining <= 0) {
                /* TODO: Rule-LK 6/7 default -- foreclose collateral,
                 * demolish buildings, cancel insurance on them, clear
                 * the debt, and declare bankrupt if nothing remains.
                 * Needs finance.c's foreclosure logic. */
            }
        }
    }
 
    /* --- Tick down any already-active timed economic effects --- */
    if (game->market.boomRoundsRemaining > 0) game->market.boomRoundsRemaining--;
    if (game->market.declineRoundsRemaining > 0) game->market.declineRoundsRemaining--;
    if (game->market.regionalRoundsRemaining > 0) game->market.regionalRoundsRemaining--;
    if (game->market.regulationRoundsRemaining > 0) game->market.regulationRoundsRemaining--;
 
    /* --- Periodic economic reviews --- */
    if (game->currentRound % 10 == 0) {
        /* TODO: regenerate inflation rate (Rule-LK 12) */
        /* TODO: if boomRoundsRemaining/declineRoundsRemaining hit 0,
         * select new groups per Rule-LK 30-34 (respecting the 30-round
         * cooldown in Rule-LK 33) */
        /* TODO: roll for a random disaster (Rule-LK 10) */
    }
    if (game->currentRound % 15 == 0) {
        /* TODO: if regionalRoundsRemaining hit 0, draw a new Regional
         * Development Card (Section 2.10) */
    }
    if (game->currentRound % 20 == 0) {
        /* TODO: if regulationRoundsRemaining hit 0, select a new
         * Government Regulation (Rule-LK 24) */
    }
}

static int calcNetWorth(const GameState *game, int playerIndex) {
    const Player *p = &game->players[playerIndex];
    int netWorth = p->cash;
 
    for (int i = 0; i < p->numOwnedProperties; i++) {
        const Property *prop = &game->properties[p->ownedProperties[i]];
        netWorth += prop->currentValue;
        /* TODO: Rule 15's full formula separately adds Building Value,
         * Railway Value, Utility Value, and Insurance Claims Receivable,
         * and subtracts Accrued Interest and Taxes Due. currentValue is
         * a simplified stand-in until finance.c tracks those pieces
         * individually. */
    }
 
    if (p->loan.active) {
        netWorth -= p->loan.principal;
    }
 
    return netWorth;
}

static int countHotels(const GameState *game, int playerIndex) {
    const Player *p = &game->players[playerIndex];
    int hotels = 0;
    for (int i = 0; i < p->numOwnedProperties; i++) {
        const Property *prop = &game->properties[p->ownedProperties[i]];
        if (prop->development == DEV_HOTEL) hotels++;
    }
    return hotels;
}

void printRoundSummary(const GameState *game) {
    printf("=============================================\n");
    printf("Round %d Summary\n", game->currentRound);
    printf("=============================================\n");
 
    for (int i = 0; i < NUM_PLAYERS; i++) {
        const Player *p = &game->players[i];
 
        printf("%s\n", p->name);
        printf("Cash : LKR %d\n", p->cash);
        printf("Net Worth : LKR %d\n", calcNetWorth(game, i));
        printf("Properties : %d\n", p->numOwnedProperties);
        printf("Hotels : %d\n", countHotels(game, i));
        if (p->loan.active) {
            printf("Outstanding Loan : LKR %d\n", p->loan.principal);
        } else {
            printf("Outstanding Loan : None\n");
        }
 
        if (i < NUM_PLAYERS - 1) {
            printf("---------------------------------------------\n");
        }
    }
 
    printf("=============================================\n\n");
}

void startGame() {
      GameState game;

      //has 45 needed to change to time(NULL) change to unsigned int
      srand(45);

      // 1. Initialize board and players
      initializeBoard(&game);

      printf("MONOPOLY-LK Simulation\n\n");

      initPlayers(&game);
      
      printf("\nEach player begins with LKR %d.\n\n", STARTING_CASH);
      
    //Determine turn order
    determineTurnOrder(&game);

    game.currentPlayerIndex = 0;

    game.currentRound = 0;
    game.market.boomRoundsRemaining = 0;
    game.market.declineRoundsRemaining = 0;
    game.market.regionalRoundsRemaining = 0;
    game.market.regulationRoundsRemaining = 0;
    game.market.regulationIndex = -1;
    game.market.cardIndex = -1;
    /* TODO: initialize the rest of game.market (Section 2.3, 2.9, 2.10)
     * and game.nationalDeck (Appendix A) here too. */

   initializeNationalDeck(&game);
    /* TODO: initialize inflationRatePercent and currentLoanInterestRate
     * (Appendix D) here too. */

//Main loop
    while (game.currentRound < MAX_ROUNDS && countSolventPlayers(&game) > 1) {

    for (int i = 0; i < NUM_PLAYERS; i++) {
        int actualPlayer = game.turnOrder[i];
        if (!game.players[actualPlayer].bankrupt) {
            playTurn(&game, actualPlayer);   // ONE turn each,roll,move,resolve landing.
        }
    }

    game.currentRound++;   // the round only advances ONCE per full cycle

    processEndOfRound(&game);  // interest accrual, property aging,
                                // check inflation/market/event/regulation
                                // timers against game.currentRound
    printRoundSummary(&game);
    }



    /* TODO: determine and print the winner (Rule 15, Section 5 "End of Game") */




}
