/* ============================================================
 * board.c
 * MONOPOLY-LK Simulation
 *
 * Board initialization and movement logic (Table 5 requirement).
 *
 * DESIGN NOTE: every square and every property is set up with an
 * explicit, individual function call below (no lookup tables, no
 * loops that dispatch based on square type). This is more lines of
 * code than a table-driven version, but every value is typed
 * exactly where it is used, so you can point to any single line
 * and explain precisely what it does and where the number came
 * from -- useful for the viva.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"
#include "board.h"

/* ------------------------------------------------------------
 * INTERNAL HELPERS (static = only visible inside this file)
 *
 * Both helpers just copy values into an already-allocated struct
 * that lives inside GameState. Without them, every one of the 40
 * setSquare() calls and 28 setProperty() calls below would have to
 * manually write 4-15 lines of "game->board[i].something = ..."
 * -- these just save repetition, they don't add any logic.
 * ------------------------------------------------------------ */

/* Safely copies a name into a fixed-size buffer, guaranteeing
 * null-termination even if the source is too long. */
void copyName(char *dest, size_t destSize, const char *src) {
    strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}

/* Fills in one Square. propertyIndex is -1 for non-ownable squares. */
static void setSquare(GameState *game, int index, const char *name,
                       SquareType type, int propertyIndex) {
    Square *s = &game->board[index];
    s->index = index;
    copyName(s->name, sizeof(s->name), name);
    s->type = type;
    s->propertyIndex = propertyIndex;
}

/* Fills in one Property with its starting values. Every property
 * begins unowned (owner = -1, i.e. the Bank -- Rule 1), undeveloped,
 * unmortgaged, uninsured, and in perfect condition. */
static void setProperty(GameState *game, int index, const char *name,
                         PropertyCategory category, ColourGroup group,
                         int purchasePrice, int mortgageValue,
                         int baseRent, int houseCost, int hotelCost) {
    Property *p = &game->properties[index];

    copyName(p->name, sizeof(p->name), name);
    p->category = category;
    p->group = group;

    p->basePurchasePrice = purchasePrice;
    p->baseMortgageValue = mortgageValue;
    p->baseRent = baseRent;
    p->baseHouseCost = houseCost;
    p->baseHotelCost = hotelCost;

    /* Nothing has modified prices yet, so current == base initially. */
    p->currentPurchasePrice = purchasePrice;
    p->currentMortgageValue = mortgageValue;
    p->currentRent = baseRent;
    p->currentHouseCost = houseCost;
    p->currentHotelCost = hotelCost;
    p->currentValue = purchasePrice;

    p->owner = -1;
    p->development = DEV_NONE;
    p->mortgaged = false;
    p->loanLocked = false;

    p->insurance = INS_NONE;
    p->insuranceRoundsRemaining = 0;

    p->age = 0;
    p->depreciationPercent = 0;
    p->condition = 100;
    p->roundsSinceMaintenance = 0;
    p->structurallyDamaged = false;
    p->disasterDamaged = false;
}

/* Every railway station and every utility purchases for LKR 1,500
 * with a mortgage value of LKR 750 (confirmed values, supplementary
 * property data sheet). Defined once here so the six calls below
 * don't repeat the same two magic numbers six times. */
#define RAILWAY_PURCHASE_PRICE   1500
#define RAILWAY_MORTGAGE_VALUE    750
#define UTILITY_PURCHASE_PRICE   1500
#define UTILITY_MORTGAGE_VALUE    750

/* ------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * ----------------------------------------- ------------------- */

void initializeBoard(GameState *game) {

    /* Square 0: GO (Rule 4 -- passing/landing here pays LKR 2,000) */
    setSquare(game, 0, "GO", SQ_START, -1);

    /* Square 1: Pettah -- Brown group, property index 0 */
    setSquare(game, 1, "Pettah", SQ_PROPERTY, 0);
    setProperty(game, 0, "Pettah", CAT_COLOUR, GROUP_BROWN,
                1500, 750, 100, 500, 2000);

    /* Square 2: Community Development Fund (National Event Card) */
    setSquare(game, 2, "Community Development Fund", SQ_EVENT, -1);

    /* Square 3: Maradana -- Brown group, property index 1 */
    setSquare(game, 3, "Maradana", SQ_PROPERTY, 1);
    setProperty(game, 1, "Maradana", CAT_COLOUR, GROUP_BROWN,
                1800, 750, 120, 500, 2000);

    /* Square 4: Income Tax (Rule 11) */
    setSquare(game, 4, "Income Tax", SQ_TAX, -1);

    /* Square 5: Colombo Fort Railway Station -- property index 2 */
    setSquare(game, 5, "Colombo Fort Railway Station", SQ_RAILWAY, 2);
    setProperty(game, 2, "Colombo Fort Railway Station", CAT_RAILWAY, GROUP_NONE,
                RAILWAY_PURCHASE_PRICE, RAILWAY_MORTGAGE_VALUE, 0, 0, 0);

    /* Square 6: Bambalapitiya -- Light Blue group, property index 3 */
    setSquare(game, 6, "Bambalapitiya", SQ_PROPERTY, 3);
    setProperty(game, 3, "Bambalapitiya", CAT_COLOUR, GROUP_LIGHT_BLUE,
                2500, 1250, 180, 750, 3000);

    /* Square 7: National Event Card */
    setSquare(game, 7, "National Event Card", SQ_EVENT, -1);

    /* Square 8: Wellawatte -- Light Blue group, property index 4 */
    setSquare(game, 8, "Wellawatte", SQ_PROPERTY, 4);
    setProperty(game, 4, "Wellawatte", CAT_COLOUR, GROUP_LIGHT_BLUE,
                2700, 1250, 200, 750, 3000);

    /* Square 9: Mount Lavinia -- Light Blue group, property index 5 */
    setSquare(game, 9, "Mount Lavinia", SQ_PROPERTY, 5);
    setProperty(game, 5, "Mount Lavinia", CAT_COLOUR, GROUP_LIGHT_BLUE,
                3000, 1250, 220, 750, 3000);

    /* Square 10: Jail / Just Visiting */
    setSquare(game, 10, "Jail / Just Visiting", SQ_JAIL, -1);

    /* Square 11: Nugegoda -- Pink group, property index 6 */
    setSquare(game, 11, "Nugegoda", SQ_PROPERTY, 6);
    setProperty(game, 6, "Nugegoda", CAT_COLOUR, GROUP_PINK,
                3500, 1750, 260, 1000, 4000);

    /* Square 12: Ceylon Electricity Board -- property index 7 */
    setSquare(game, 12, "Ceylon Electricity Board", SQ_UTILITY, 7);
    setProperty(game, 7, "Ceylon Electricity Board", CAT_UTILITY, GROUP_NONE,
                UTILITY_PURCHASE_PRICE, UTILITY_MORTGAGE_VALUE, 0, 0, 0);

    /* Square 13: Maharagama -- Pink group, property index 8 */
    setSquare(game, 13, "Maharagama", SQ_PROPERTY, 8);
    setProperty(game, 8, "Maharagama", CAT_COLOUR, GROUP_PINK,
                3800, 1750, 280, 1000, 4000);

    /* Square 14: Kottawa -- Pink group, property index 9 */
    setSquare(game, 14, "Kottawa", SQ_PROPERTY, 9);
    setProperty(game, 9, "Kottawa", CAT_COLOUR, GROUP_PINK,
                4000, 1750, 300, 1000, 4000);

    /* Square 15: Kandy Railway Station -- property index 10 */
    setSquare(game, 15, "Kandy Railway Station", SQ_RAILWAY, 10);
    setProperty(game, 10, "Kandy Railway Station", CAT_RAILWAY, GROUP_NONE,
                RAILWAY_PURCHASE_PRICE, RAILWAY_MORTGAGE_VALUE, 0, 0, 0);

    /* Square 16: Negombo -- Orange group, property index 11 */
    setSquare(game, 16, "Negombo", SQ_PROPERTY, 11);
    setProperty(game, 11, "Negombo", CAT_COLOUR, GROUP_ORANGE,
                4500, 2250, 350, 1250, 5000);

    /* Square 17: Sri Lanka Insurance */
    setSquare(game, 17, "Sri Lanka Insurance", SQ_INSURANCE, -1);

    /* Square 18: Katunayake -- Orange group, property index 12 */
    setSquare(game, 18, "Katunayake", SQ_PROPERTY, 12);
    setProperty(game, 12, "Katunayake", CAT_COLOUR, GROUP_ORANGE,
                4700, 2250, 370, 1250, 5000);

    /* Square 19: Ja-Ela -- Orange group, property index 13 */
    setSquare(game, 19, "Ja-Ela", SQ_PROPERTY, 13);
    setProperty(game, 13, "Ja-Ela", CAT_COLOUR, GROUP_ORANGE,
                5000, 2250, 400, 1250, 5000);

    /* Square 20: Free Parking */
    setSquare(game, 20, "Free Parking", SQ_FREE_PARKING, -1);

    /* Square 21: Kandy City -- Red group, property index 14 */
    setSquare(game, 21, "Kandy City", SQ_PROPERTY, 14);
    setProperty(game, 14, "Kandy City", CAT_COLOUR, GROUP_RED,
                5500, 2750, 450, 1500, 6000);

    /* Square 22: National Event Card */
    setSquare(game, 22, "National Event Card", SQ_EVENT, -1);

    /* Square 23: Peradeniya -- Red group, property index 15 */
    setSquare(game, 23, "Peradeniya", SQ_PROPERTY, 15);
    setProperty(game, 15, "Peradeniya", CAT_COLOUR, GROUP_RED,
                5800, 2750, 480, 1500, 6000);

    /* Square 24: Katugastota -- Red group, property index 16 */
    setSquare(game, 24, "Katugastota", SQ_PROPERTY, 16);
    setProperty(game, 16, "Katugastota", CAT_COLOUR, GROUP_RED,
                6000, 2750, 500, 1500, 6000);

    /* Square 25: Galle Railway Station -- property index 17 */
    setSquare(game, 25, "Galle Railway Station", SQ_RAILWAY, 17);
    setProperty(game, 17, "Galle Railway Station", CAT_RAILWAY, GROUP_NONE,
                RAILWAY_PURCHASE_PRICE, RAILWAY_MORTGAGE_VALUE, 0, 0, 0);

    /* Square 26: Galle Fort -- Yellow group, property index 18 */
    setSquare(game, 26, "Galle Fort", SQ_PROPERTY, 18);
    setProperty(game, 18, "Galle Fort", CAT_COLOUR, GROUP_YELLOW,
                6500, 3250, 600, 2000, 8000);

    /* Square 27: Unawatuna -- Yellow group, property index 19 */
    setSquare(game, 27, "Unawatuna", SQ_PROPERTY, 19);
    setProperty(game, 19, "Unawatuna", CAT_COLOUR, GROUP_YELLOW,
                6800, 3250, 620, 2000, 8000);

    /* Square 28: National Water Supply and Drainage Board -- property index 20 */
    setSquare(game, 28, "National Water Supply and Drainage Board", SQ_UTILITY, 20);
    setProperty(game, 20, "National Water Supply and Drainage Board", CAT_UTILITY, GROUP_NONE,
                UTILITY_PURCHASE_PRICE, UTILITY_MORTGAGE_VALUE, 0, 0, 0);

    /* Square 29: Hikkaduwa -- Yellow group, property index 21 */
    setSquare(game, 29, "Hikkaduwa", SQ_PROPERTY, 21);
    setProperty(game, 21, "Hikkaduwa", CAT_COLOUR, GROUP_YELLOW,
                7000, 3250, 650, 2000, 8000);

    /* Square 30: Go To Jail (Rule 12) */
    setSquare(game, 30, "Go To Jail", SQ_GO_TO_JAIL, -1);

    /* Square 31: Jaffna Town -- Green group, property index 22 */
    setSquare(game, 31, "Jaffna Town", SQ_PROPERTY, 22);
    setProperty(game, 22, "Jaffna Town", CAT_COLOUR, GROUP_GREEN,
                8000, 4000, 750, 2500, 10000);

    /* Square 32: Nallur -- Green group, property index 23 */
    setSquare(game, 32, "Nallur", SQ_PROPERTY, 23);
    setProperty(game, 23, "Nallur", CAT_COLOUR, GROUP_GREEN,
                8300, 4000, 780, 2500, 10000);

    /* Square 33: Ceylinco Insurance */
    setSquare(game, 33, "Ceylinco Insurance", SQ_INSURANCE, -1);

    /* Square 34: Trincomalee -- Green group, property index 24 */
    setSquare(game, 34, "Trincomalee", SQ_PROPERTY, 24);
    setProperty(game, 24, "Trincomalee", CAT_COLOUR, GROUP_GREEN,
                8500, 4000, 800, 2500, 10000);

    /* Square 35: Jaffna Railway Station -- property index 25 */
    setSquare(game, 35, "Jaffna Railway Station", SQ_RAILWAY, 25);
    setProperty(game, 25, "Jaffna Railway Station", CAT_RAILWAY, GROUP_NONE,
                RAILWAY_PURCHASE_PRICE, RAILWAY_MORTGAGE_VALUE, 0, 0, 0);

    /* Square 36: National Event Card */
    setSquare(game, 36, "National Event Card", SQ_EVENT, -1);

    /* Square 37: Nuwara Eliya -- Dark Blue group, property index 26 */
    setSquare(game, 37, "Nuwara Eliya", SQ_PROPERTY, 26);
    setProperty(game, 26, "Nuwara Eliya", CAT_COLOUR, GROUP_DARK_BLUE,
                10000, 5000, 1000, 3000, 12000);

    /* Square 38: Bank of Ceylon (Section 1.1.4 / Rule-LK 5) */
    setSquare(game, 38, "Bank of Ceylon", SQ_BANK, -1);

    /* Square 39: Galle Face -- Dark Blue group, property index 27 */
    setSquare(game, 39, "Galle Face", SQ_PROPERTY, 27);
    setProperty(game, 27, "Galle Face", CAT_COLOUR, GROUP_DARK_BLUE,
                12000, 5000, 1200, 3000, 12000);

    /* All 40 squares (0-39) and all 28 properties (0-27) are now set up. */
}

int movePlayer(GameState *game, int playerIndex, int diceTotal) {
    Player *p = &game->players[playerIndex];
    int oldPosition = p->position;
    int newPosition = (oldPosition + diceTotal) % NUM_SQUARES;

    /* Rule 4: passing OR landing on GO awards LKR 2,000.
     * If old + dice reaches 40 or more, we wrapped past square 0. */
    bool passedGo = (oldPosition + diceTotal) >= NUM_SQUARES;

    p->position = newPosition;

    printf("%s moves from Square %d to Square %d.\n",
           p->name, oldPosition, newPosition);

    if (passedGo) {
        p->cash += GO_MONEY;
        printf("%s passed GO.\n", p->name);
        printf("Collected LKR %d.\n", GO_MONEY);
        printf("Current Balance : LKR %d.\n", p->cash);
    }

    /* NOTE: Rule 12 (Go To Jail sends the player to jail WITHOUT GO
     * money) is a LANDING effect, not a movement effect -- it depends
     * on what square you ended up on, so it belongs in resolveLanding(),
     * not here. Don't special-case it in this function. */

    return newPosition;
}


