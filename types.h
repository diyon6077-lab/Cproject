/* ============================================================
 * types.h
 * MONOPOLY-LK Simulation
 *
 * Definitions of structures, enumerations, constants, and data
 * types used throughout the simulation (Table 5 requirement).
 *
 * Comments reference the rule numbers from the assignment spec
 * so you can trace every field back to why it exists.
 * ============================================================ */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

/* ------------------------------------------------------------
 * CONSTANTS
 * ------------------------------------------------------------ */

#define NUM_PLAYERS        4
#define NUM_SQUARES        40
#define NUM_PROPERTIES     28   /* 22 colour props + 4 railways + 2 utilities */
#define NUM_GROUPS         8    /* Brown ... Dark Blue (Section 1.1.1) */
#define MAX_ROUNDS         500  /* Rule 15 */
#define STARTING_CASH      30000
#define GO_MONEY           2000
#define BAIL_AMOUNT        300  /* Rule 13 */
#define MAX_JAIL_TURNS     3    /* Rule 13 */
#define AUCTION_INCREMENT  250  /* Rule 6 / Rule-LK 20 */
#define MAX_HOUSES         4    /* Rule 9 */
#define NUM_NATIONAL_CARDS 20   /* Appendix A */

/* ------------------------------------------------------------
 * ENUMERATIONS
 * ------------------------------------------------------------ */

/* What kind of square this is (Table 1) */
typedef enum {
    SQ_START,
    SQ_PROPERTY,
    SQ_EVENT,
    SQ_TAX,
    SQ_RAILWAY,
    SQ_UTILITY,
    SQ_INSURANCE,
    SQ_BANK,
    SQ_JAIL,          /* Jail / Just Visiting */
    SQ_FREE_PARKING,
    SQ_GO_TO_JAIL
} SquareType;

/* What category of ownable asset a Property struct represents.
 * Colour properties, railways, and utilities all use the same
 * Property struct, but rent/development rules differ (Sections
 * 1.1.1 - 1.1.3), so we tag which kind it is. */
typedef enum {
    CAT_COLOUR,
    CAT_RAILWAY,
    CAT_UTILITY
} PropertyCategory;

/* The eight colour groups (Section 1.1.1). Railways/utilities
 * don't belong to a colour group, so use GROUP_NONE for those. */
typedef enum {
    GROUP_BROWN,
    GROUP_LIGHT_BLUE,
    GROUP_PINK,
    GROUP_ORANGE,
    GROUP_RED,
    GROUP_YELLOW,
    GROUP_GREEN,
    GROUP_DARK_BLUE,
    GROUP_NONE
} ColourGroup;

/* The four autonomous player behaviours (Section 3) */
typedef enum {
    STRAT_AGGRESSIVE_INVESTOR,
    STRAT_CONSERVATIVE_BANKER,
    STRAT_RISK_TAKER,
    STRAT_OPPORTUNISTIC_TRADER
} Strategy;

/* Insurance policy types (Section 1.2 / Table 10). A property
 * can hold at most one active policy at a time. */
typedef enum {
    INS_NONE,
    INS_BASIC,
    INS_COMPREHENSIVE,
    INS_BUSINESS_INTERRUPTION
} InsurancePolicy;

/* Development state of a colour property. Hotels *replace* four
 * houses (Rule 10), so this is one field, not "houses + hotel". */
typedef enum {
    DEV_NONE,
    DEV_ONE_HOUSE,
    DEV_TWO_HOUSES,
    DEV_THREE_HOUSES,
    DEV_FOUR_HOUSES,
    DEV_HOTEL
} DevelopmentLevel;

/* ------------------------------------------------------------
 * BOARD STRUCTURES
 * ------------------------------------------------------------ */

/* A single square on the board (Table 1). This only holds
 * "where am I / what kind of square is this" information.
 * Financial data for ownable squares lives in Property, linked
 * via propertyIndex. */
typedef struct {
    int         index;          /* 0-39, position on the board */
    char        name[40];
    SquareType  type;
    int         propertyIndex;  /* index into properties[], or -1 if not ownable */
} Square;

/* An ownable asset: colour property, railway, or utility.
 * All financial/ownership/condition state lives here (Program
 * Requirements: "Every property shall maintain ownership,
 * development, insurance, mortgage, depreciation, and valuation
 * information."). */
typedef struct {
    char             name[40];
    PropertyCategory category;
    ColourGroup      group;          /* GROUP_NONE for railway/utility */

    /* --- base values (Appendix B), before inflation/market adjustment --- */
    int  basePurchasePrice;
    int  baseMortgageValue;
    int  baseRent;
    int  baseHouseCost;
    int  baseHotelCost;

    /* --- current, effective values after inflation / market boom or
     * decline / regional cards / events have been applied
     * (Rule-LK 14, 31, 32, 34) --- */
    int  currentPurchasePrice;
    int  currentMortgageValue;
    int  currentRent;
    int  currentHouseCost;
    int  currentHotelCost;
    int  currentValue;               /* used for net worth + insurance premiums */

    /* --- ownership & development --- */
    int              owner;          /* -1 = Bank, else index into players[] */
    DevelopmentLevel development;
    bool             mortgaged;
    bool             loanLocked;     /* Rule-LK 3: pledged as loan collateral */

    /* --- insurance (Section 1.2, Rule-LK 8-11) --- */
    InsurancePolicy  insurance;
    int              insuranceRoundsRemaining; /* counts down from 20 */

    /* --- depreciation & maintenance (Sections 2.4, 2.8) --- */
    int   age;                       /* rounds since last renovation, Rule-LK 15 */
    int   depreciationPercent;       /* up to 30%, Rule-LK 16 */
    int   condition;                 /* 0-100%, buildings only, Rule-LK 25 */
    int   roundsSinceMaintenance;    /* Rule-LK 28 trigger at 20 */
    bool  structurallyDamaged;       /* Rule-LK 28 */
    bool  disasterDamaged;           /* Rule-LK 11: can't collect rent until repaired */
} Property;

/* ------------------------------------------------------------
 * LOAN STRUCTURE (Section 2.1)
 * ------------------------------------------------------------ */

typedef struct {
    bool active;
    int  principal;              /* current outstanding amount, incl. accrued interest */
    int  interestRatePercent;    /* fixed at issue, Rule-LK 13: existing loans unaffected by later inflation */
    int  roundsRemaining;        /* counts down from 20, Rule-LK 4 */

    /* collateral: indices into properties[] that are Loan Locked */
    int  collateral[NUM_PROPERTIES];
    int  numCollateral;
} Loan;

/* ------------------------------------------------------------
 * PLAYER STRUCTURE
 * ------------------------------------------------------------ */

typedef struct {
    char     name[40];
    Strategy strategy;

    int  cash;
    int  position;          /* current square index, 0-39 */

    bool inJail;
    int  jailTurnsServed;

    bool bankrupt;

    /* indices into properties[] currently owned by this player */
    int  ownedProperties[NUM_PROPERTIES];
    int  numOwnedProperties;

    Loan loan;               /* only one active loan at a time (Rule-LK) */
} Player;

/* ------------------------------------------------------------
 * ECONOMY / EVENT STATE (Sections 2.3, 2.5, 2.7, 2.9, 2.10)
 *
 * These track "what's currently active and how long it lasts".
 * Each one is just a percentage + a countdown, so once you code
 * the first (e.g. inflation), the rest follow the same pattern.
 * ------------------------------------------------------------ */

typedef struct {
    /* Inflation - Rule-LK 12-14, refreshed every 10 rounds */
    int inflationRatePercent;

    /* Property market boom / decline - Rule-LK 30-34, every 10 rounds */
    ColourGroup boomGroup;
    int         boomRoundsRemaining;
    ColourGroup declineGroup;
    int         declineRoundsRemaining;

    /* Regional development card - Section 2.10, every 15 rounds */
    int cardIndex;                 /* which card is active, -1 if none */
    int regionalRoundsRemaining;

    /* Government regulation - Rule-LK 24, every 20 rounds */
    int regulationIndex;           /* -1 if none currently active */
    int regulationRoundsRemaining;

    /* Current loan interest rate offered by the bank (Appendix D),
     * adjusted by economic condition / regulations, but NOT
     * retroactively applied to already-issued loans (Rule-LK 13). */
    int currentLoanInterestRate;
} MarketState;

/* National Event Card deck (Appendix A) */
typedef struct {
    char name[40];
    char effectDescription[100];
    /* You can add typed fields here later once you decide how
     * you want to apply each effect programmatically, e.g.:
     * int  durationRounds;
     * ... */
} EventCard;

/* ------------------------------------------------------------
 * MASTER GAME STATE
 *
 * Bundling everything into one struct lets you avoid globals
 * (Program Requirements) -- pass a GameState* into every
 * function instead.
 * ------------------------------------------------------------ */

typedef struct {
    Square      board[NUM_SQUARES];
    Property    properties[NUM_PROPERTIES];
    Player      players[NUM_PLAYERS];

    EventCard   nationalDeck[NUM_NATIONAL_CARDS];
    int         nextCardIndex;      /* top of deck, wraps around (Appendix A) */

    MarketState market;

    int         currentRound;
    int         turnOrder[NUM_PLAYERS];  /* player indices, in play order (Rule 2) */
    int         currentPlayerIndex;      /* whose turn within turnOrder[] */
} GameState;

#endif /* TYPES_H */
