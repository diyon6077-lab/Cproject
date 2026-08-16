/* ============================================================
 * players.c
 * MONOPOLY-LK Simulation
 *
 * Player decision-making algorithms (Table 5 requirement). Each of
 * the 4 strategies from Section 3 gets its own small function; a
 * dispatcher (switch on Strategy, same pattern as events.c's switch
 * on NationalEventType) routes to the right one.
 * ============================================================ */

#include "types.h"
#include "players.h"

/* ------------------------------------------------------------
 * PURCHASE DECISIONS (Rule 5) -- one function per Strategy,
 * translating its Section 3 bullet point into an actual condition.
 * ------------------------------------------------------------ */

/* Section 3.1: "Always purchases an unowned property if sufficient
 * funds remain to pay at least one future rent." We don't know what
 * rent someone else would actually charge, so as a stand-in we use
 * the property's OWN currentRent as "a future rent" -- reasonable
 * for colour properties, but weak for railways/utilities, whose
 * currentRent field is always 0 (their rent is computed dynamically
 * from stations-owned / dice roll, not stored). Document this if
 * asked -- it means Aggressive Investor will buy any affordable
 * railway/utility outright, with no rent buffer check at all. */
static bool aggressiveInvestorWantsToBuy(const GameState *game, int playerIndex, int propIndex) {
    const Player *p = &game->players[playerIndex];
    const Property *prop = &game->properties[propIndex];
    return (p->cash - prop->currentPurchasePrice) >= prop->currentRent;
}

/* Section 3.2: "Purchases properties only if at least 50% of
 * current cash remains after purchase." Direct translation. */
static bool conservativeBankerWantsToBuy(const GameState *game, int playerIndex, int propIndex) {
    const Player *p = &game->players[playerIndex];
    const Property *prop = &game->properties[propIndex];
    int cashAfter = p->cash - prop->currentPurchasePrice;
    return cashAfter >= (p->cash / 2);
}

/* Section 3.3: "Purchases every available property whenever legally
 * possible." "Legally possible" just means "can afford it". */
static bool riskTakerWantsToBuy(const GameState *game, int playerIndex, int propIndex) {
    const Player *p = &game->players[playerIndex];
    const Property *prop = &game->properties[propIndex];
    return p->cash >= prop->currentPurchasePrice;
}

/* Section 3.4: "Purchases properties only when projected
 * appreciation exceeds construction costs." The spec never gives a
 * formula for "projected appreciation" -- ASSUMPTION: we use the
 * property's currentValue as a stand-in for its appreciation
 * potential, compared against its house construction cost. Railways
 * and utilities have houseCost == 0 (Sections 1.1.2/1.1.3 -- they
 * can't be developed), so this condition is trivially true for them;
 * worth mentioning if asked. */
static bool opportunisticTraderWantsToBuy(const GameState *game, int playerIndex, int propIndex) {
    const Player *p = &game->players[playerIndex];
    const Property *prop = &game->properties[propIndex];
    if (p->cash < prop->currentPurchasePrice) return false;   /* must afford it at all */
    return prop->currentValue > prop->currentHouseCost;
}

bool playerDecideToPurchase(const GameState *game, int playerIndex, int propIndex) {
    const Player *p = &game->players[playerIndex];

    switch (p->strategy) {
        case STRAT_AGGRESSIVE_INVESTOR:
            return aggressiveInvestorWantsToBuy(game, playerIndex, propIndex);
        case STRAT_CONSERVATIVE_BANKER:
            return conservativeBankerWantsToBuy(game, playerIndex, propIndex);
        case STRAT_RISK_TAKER:
            return riskTakerWantsToBuy(game, playerIndex, propIndex);
        case STRAT_OPPORTUNISTIC_TRADER:
            return opportunisticTraderWantsToBuy(game, playerIndex, propIndex);
    }

    return false;   /* unreachable if every Strategy value is handled above */
}

/* ------------------------------------------------------------
 * JAIL BAIL DECISION
 *
 * NOT described anywhere in the spec -- Rule 13 lists the three exit
 * conditions (pay bail / roll doubles / serve 3 turns) but never
 * says which one each Strategy prefers. This whole function is OUR
 * OWN design choice, made to keep the four strategies visibly
 * different in behaviour, not a rule citation:
 *   - Conservative Banker & Opportunistic Trader: pay bail
 *     immediately if affordable (keeps cash flowing, matches their
 *     generally risk-averse / calculated framing in Section 3).
 *   - Aggressive Investor & Risk Taker: never pay voluntarily --
 *     wait for doubles or the forced 3-turn exit (matches their
 *     "never voluntarily give up cash/assets" framing).
 * If you'd rather justify a different split, this is the one
 * function to change -- it's isolated on purpose.
 * ------------------------------------------------------------ */

bool playerDecideToPayBailVoluntarily(const GameState *game, int playerIndex) {
    const Player *p = &game->players[playerIndex];

    switch (p->strategy) {
        case STRAT_CONSERVATIVE_BANKER:
        case STRAT_OPPORTUNISTIC_TRADER:
            return true;
        case STRAT_AGGRESSIVE_INVESTOR:
        case STRAT_RISK_TAKER:
        default:
            return false;
    }
}
