/* ============================================================
 * events.c
 * MONOPOLY-LK Simulation
 *
 * Economic events and government regulations (Table 5 requirement).
 * This file currently covers the National Event Card deck
 * (Appendix A). Government regulations, inflation, market booms/
 * declines, and regional development cards (Sections 2.3, 2.7, 2.9,
 * 2.10) will be added here too as they're built.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "events.h"
#include "board.h"

/* ------------------------------------------------------------
 * DECK SETUP
 * ------------------------------------------------------------ */



static void setCard(GameState *game, int index, const char *name,
                     const char *description, NationalEventType type) {
    EventCard *c = &game->nationalDeck[index];
    copyName(c->name, sizeof(c->name), name);
    copyName(c->effectDescription, sizeof(c->effectDescription), description);
    c->type = type;
}

void initializeNationalDeck(GameState *game) {
    setCard(game, 0,  "Tourism Hype",           "Hotels earn double rent for 5 rounds",              EVENT_TOURISM_HYPE);
    setCard(game, 1,  "Fuel Shortage",           "Railway rent doubles for 5 rounds",                 EVENT_FUEL_SHORTAGE);
    setCard(game, 2,  "Heavy Floods",            "Random coastal property damaged",                   EVENT_HEAVY_FLOODS);
    setCard(game, 3,  "Political Rally",         "One random property closed for 2 rounds",           EVENT_POLITICAL_RALLY);
    setCard(game, 4,  "Stock Market Rise",       "All property values increase by 10%",               EVENT_STOCK_MARKET_RISE);
    setCard(game, 5,  "Economic Downturn",       "Property values decrease by 15%",                   EVENT_ECONOMIC_DOWNTURN);
    setCard(game, 6,  "Housing Subsidy",         "House construction cost reduced by 30%",            EVENT_HOUSING_SUBSIDY);
    setCard(game, 7,  "Interest Rate Cut",       "Loan interest reduced by 2%",                        EVENT_INTEREST_RATE_CUT);
    setCard(game, 8,  "Interest Rate Increase",  "Loan interest increased by 2%",                      EVENT_INTEREST_RATE_INCREASE);
    setCard(game, 9,  "Tax Amnesty",             "Each player receives LKR 2,000",                     EVENT_TAX_AMNESTY);
    setCard(game, 10, "Power Failure",           "Utility income halved for 3 rounds",                 EVENT_POWER_FAILURE);
    setCard(game, 11, "Foreign Funding",         "Commercial property values increase by 15%",         EVENT_FOREIGN_FUNDING);
    setCard(game, 12, "Port Expansion",          "Railway station values increase by 20%",             EVENT_PORT_EXPANSION);
    setCard(game, 13, "Festival Season",         "Hotels receive 50% additional rent",                 EVENT_FESTIVAL_SEASON);
    setCard(game, 14, "Labour Strike",           "Construction suspended for 2 rounds",                EVENT_LABOUR_STRIKE);
    setCard(game, 15, "Insurance Discount",      "Premiums reduced by 20%",                            EVENT_INSURANCE_DISCOUNT);
    setCard(game, 16, "Property Revaluation",    "Random property group appreciates by 15%",           EVENT_PROPERTY_REVALUATION);
    setCard(game, 17, "Currency Depreciation",   "Construction costs increase by 10%",                 EVENT_CURRENCY_DEPRECIATION);
    setCard(game, 18, "Government Grant",        "Random player receives LKR 5,000",                   EVENT_GOVERNMENT_GRANT);
    setCard(game, 19, "National Disaster",       "Random developed property damaged",                  EVENT_NATIONAL_DISASTER);

    game->nextCardIndex = 0;
}

/* ------------------------------------------------------------
 * THE DISPATCHER
 *
 * "Dispatching" just means: given some identifier (here, card->type),
 * jump straight to the one block of code that handles THAT case,
 * instead of the caller having to know or care what the 20 different
 * cards even are. drawNationalEventCard() below doesn't know or care
 * what "Tax Amnesty" does -- it just hands the card to this function
 * and trusts the switch to route it correctly. That's the whole
 * pattern; a switch on an enum is one of the simplest ways to build
 * a dispatcher in C (a table of function pointers indexed by enum is
 * a fancier alternative, but a switch is just as correct and much
 * easier to read/explain).
 * ------------------------------------------------------------ */

static void applyEventEffect(GameState *game, int playerIndex, const EventCard *card) {
    Player *p = &game->players[playerIndex];
    
    //tweak so that no.2 position won't draw a card but does the calculation 
    //Community development fund will levy a tax on the player whom lands on the cell. 
    // The tax is 10% of the total assets of the player. 
    // For the purpose of assets the current market rate of only the property(not buildings) is considered. 
    // 10% will also be affected by the market fluctuations.

    //additional:Income take base rate at the beinging of the game is 15% and market conditions will affect the rate as well.

    switch (card->type) {

        /* ---- WORKED EXAMPLE 1: one-time payout to EVERY player ---- */
        case EVENT_TAX_AMNESTY:
            for (int i = 0; i < NUM_PLAYERS; i++) {
                game->players[i].cash += 2000;
            }
            printf("Every player receives LKR 2,000.\n");
            break;

        /* ---- WORKED EXAMPLE 2: one-time payout to ONE random player ---- */
        case EVENT_GOVERNMENT_GRANT: {
            int luckyPlayer = rand() % NUM_PLAYERS;
            game->players[luckyPlayer].cash += 5000;//random player receives LKR 5,000, not current
            printf("%s receives a Government Grant of LKR 5,000.\n",
                   game->players[luckyPlayer].name);
            break;
        }

        /* ---- WORKED EXAMPLE 3: one-time GLOBAL value change ---- */
        case EVENT_STOCK_MARKET_RISE:
            for (int i = 0; i < NUM_PROPERTIES; i++) {
                Property *prop = &game->properties[i];
                prop->currentValue = prop->currentValue * 110 / 100;
            }
            printf("All property values increase by 10%%.\n");
            break;

        /* ---- WORKED EXAMPLE 4: TIMED effect scoped to the drawer ---- */
        case EVENT_TOURISM_HYPE:
            
            p->activeEventEffect = EVENT_TOURISM_HYPE;
            p->eventEffectExpiryRound = game->currentRound + 5;
            printf("%s's hotels earn double rent for the next 5 rounds.\n", p->name);
            break;

        /* ---- UNIMPLEMENTED CARDS ---- */
        case EVENT_FUEL_SHORTAGE:
            p->activeEventEffect = EVENT_FUEL_SHORTAGE;
            p->eventEffectExpiryRound = game->currentRound + 5;
            printf("%s's Railways earn double rent for the next 5 rounds.\n", p->name);
            break;

        case EVENT_POWER_FAILURE:
            p->activeEventEffect = EVENT_POWER_FAILURE;
            p->eventEffectExpiryRound = game->currentRound + 3;
            printf("%s's Utilities earn half rent for the next 3 rounds.\n", p->name);
            break;

        case EVENT_FESTIVAL_SEASON:
            p->activeEventEffect = EVENT_FESTIVAL_SEASON;
            p->eventEffectExpiryRound = game->currentRound + 15;
            printf("%s's Hotels earn 50%% additional rent for the next 15 rounds.\n", p->name);
            break;

        case EVENT_ECONOMIC_DOWNTURN:
            for (int i = 0; i < NUM_PROPERTIES; i++) {
                Property *prop = &game->properties[i];
                prop->currentValue = prop->currentValue * 85 / 100;
            }
            printf("All property values decrease by 15%%.\n");
            break;

        case EVENT_FOREIGN_FUNDING:
            for (int i = 0; i < NUM_PROPERTIES; i++) {
                Property *prop = &game->properties[i];
                if (prop->category == CAT_COLOUR) {  /* commercial properties only */
                    prop->currentValue = prop->currentValue * 115 / 100;
                }
            }
            printf("All commercial property values increase by 15%%.\n");
            break;

        case EVENT_PORT_EXPANSION:
            for (int i = 0; i < NUM_PROPERTIES; i++) {
                Property *prop = &game->properties[i];
                if (prop->category == CAT_RAILWAY) {
                    prop->currentValue = prop->currentValue * 120 / 100;
                }
            }
            printf("All railway station values increase by 20%%.\n");
            break;

        case EVENT_CURRENCY_DEPRECIATION:
            for (int i = 0; i < NUM_PROPERTIES; i++) {
                Property *prop = &game->properties[i];
                prop->currentHouseCost = prop->currentHouseCost * 110 / 100;
                prop->currentHotelCost = prop->currentHotelCost * 110 / 100;
            }
            printf("Construction costs increase by 10%%.\n");
            break;

        case EVENT_HOUSING_SUBSIDY:
            for (int i = 0; i < NUM_PROPERTIES; i++) {
                Property *prop = &game->properties[i];
                prop->currentHouseCost = prop->currentHouseCost * 70 / 100;
            }
            printf("House construction costs reduced by 30%%.\n");
            break;

        case EVENT_INTEREST_RATE_CUT:
            game->market.currentLoanInterestRate -= 2;
            if (game->market.currentLoanInterestRate < 0) {
                game->market.currentLoanInterestRate = 0;
            }
            printf("Loan interest rate reduced by 2%%.\n");
            break;

        case EVENT_INTEREST_RATE_INCREASE:
            game->market.currentLoanInterestRate += 2;
            printf("Loan interest rate increased by 2%%.\n");
            break;

        case EVENT_PROPERTY_REVALUATION: {
            ColourGroup randomGroup = rand() % NUM_GROUPS;
            for (int i = 0; i < NUM_PROPERTIES; i++) {
                Property *prop = &game->properties[i];
                if (prop->group == randomGroup) {
                    prop->currentValue = prop->currentValue * 115 / 100;
                }
            }
            printf("All properties in group %d increase in value by 15%%.\n", randomGroup);
            break;
        }

         /*   EVENT_INSURANCE_DISCOUNT     -- -20% insurance premiums;
         *       needs finance.c's insurance-purchase logic to exist
         *       first (there's no premium calculation anywhere yet).
         *
         *   EVENT_HEAVY_FLOODS           -- pick a random developed
         *       "coastal" property (again, spec doesn't define which
         *       properties are coastal -- document your choice) and
         *       apply disaster damage; needs finance.c's insurance
         *       claim logic (Rule-LK 10-11) to pay out correctly.
         *
         *   EVENT_NATIONAL_DISASTER      -- same as HEAVY_FLOODS, but
         *       any random developed property, no coastal filter.
         *
         *   EVENT_POLITICAL_RALLY        -- pick a random property,
         *       mark it "closed" for 2 rounds. Needs a new Property
         *       field (e.g. closedRoundsRemaining) -- same pattern as
         *       structurallyDamaged in types.h, just timed instead of
         *       permanent-until-renovated.
         *
         *   EVENT_LABOUR_STRIKE          -- suspend construction for 2
         *       rounds; needs players.c's construction logic to exist
         *       first, so there's actually something to suspend.
         * ------------------------------------------------------------ */



        default:
            printf("(Effect for \"%s\" not yet implemented.)\n", card->name);
            break;
    }
}

/* ------------------------------------------------------------
 * DRAWING A CARD
 * ------------------------------------------------------------ */

void drawNationalEventCard(GameState *game, int playerIndex) {
    EventCard *card = &game->nationalDeck[game->nextCardIndex];

    printf("Economic Event\n");
    printf("%s\n", card->name);
    printf("%s\n", card->effectDescription);

    applyEventEffect(game, playerIndex, card);

    /* "Return the card to the bottom of the deck" (Appendix A): since
     * the array itself never gets reordered, simply advancing the
     * pointer and wrapping around with % achieves the exact same
     * result -- next draw starts at the next card, and after all 20
     * have been drawn we naturally wrap back to card 0, which is
     * identical to having physically cycled every card through the
     * bottom of a real deck. No array elements ever need to move. */
    game->nextCardIndex = (game->nextCardIndex + 1) % NUM_NATIONAL_CARDS;
}
