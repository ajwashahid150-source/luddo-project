#include <iostream>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <string>

// ================================================================
//  GLOBAL STATE & CONFIGURATION
// ================================================================

// Player names
std::string playerNames[4];

// Token positions:
// -1 = In Base
//  0 to 50 = On the main track (relative to player start)
// 51 to 55 = In the home path
// 56 = Finished (Goal)
int tokenPos[4][4];

// Game statistics and state
int playerRank[4];          // Rank of each player (0 if not finished, 1-4 once finished)
int score[4];               // Number of tokens finished (0 to 4)
int consecutiveSixes[4];    // Counters for consecutive sixes in a turn
int currentPlayer = 0;      // Active player (0 = Red, 1 = Green, 2 = Yellow, 3 = Blue)
int currentRank = 1;        // Next rank to be assigned (1 to 4)
int diceValue = 0;          // Current dice roll value
bool gameOver = false;      // Game status flag

// Symmetric clockwise 52-cell track coordinates
const int trackR[52] = {
    6, 6, 6, 6, 6,         // Left arm top row: (6,1)..(6,5)
    5, 4, 3, 2, 1, 0,      // Top arm left col: (5,6)..(0,6)
    0,                     // Top middle: (0,7)
    0, 1, 2, 3, 4, 5,      // Top arm right col: (0,8)..(5,8)
    6, 6, 6, 6, 6, 6,      // Right arm top row: (6,9)..(6,14)
    7,                     // Right middle: (7,14)
    8, 8, 8, 8, 8, 8,      // Right arm bottom row: (8,14)..(8,9)
    9, 10, 11, 12, 13, 14, // Bottom arm right col: (9,8)..(14,8)
    14,                    // Bottom middle: (14,7)
    14, 13, 12, 11, 10, 9, // Bottom arm left col: (14,6)..(9,6)
    8, 8, 8, 8, 8, 8,      // Left arm bottom row: (8,5)..(8,0)
    7,                     // Left middle: (7,0)
    6                      // Left corner: (6,0)
};

const int trackC[52] = {
    1, 2, 3, 4, 5,         // Left arm top row
    6, 6, 6, 6, 6, 6,      // Top arm left col
    7,                     // Top middle
    8, 8, 8, 8, 8, 8,      // Top arm right col
    9, 10, 11, 12, 13, 14, // Right arm top row
    14,                    // Right middle
    14, 13, 12, 11, 10, 9, // Right arm bottom row
    8, 8, 8, 8, 8, 8,      // Bottom arm right col
    7,                     // Bottom middle
    6, 6, 6, 6, 6, 6,      // Bottom arm left col
    5, 4, 3, 2, 1, 0,      // Left arm bottom row
    0,                     // Left middle
    0                      // Left corner
};

// Starting track indices for each player
const int startCellIndex[4] = { 0, 13, 26, 39 };

// ================================================================
//  PORTABILITY & SYSTEM UTILITIES (NO WINDOWS APIs)
// ================================================================

// Cross-platform clear screen using ANSI escape sequences
void clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

// Cross-platform sleep/delay using standard <ctime> (busy wait loop)
void portableSleep(int milliseconds) {
    std::clock_t start = std::clock();
    std::clock_t delay = milliseconds * CLOCKS_PER_SEC / 1000;
    while (std::clock() - start < delay) {
        // Active wait loop (no CPU sleep, completely portable standard C++)
    }
}

// ================================================================
//  GAME INITIALIZATION
// ================================================================

void initGame() {
    for (int p = 0; p < 4; ++p) {
        for (int t = 0; t < 4; ++t) {
            tokenPos[p][t] = -1; // Set all tokens to Base
        }
        playerRank[p] = 0;       // Reset ranks
        score[p] = 0;            // Reset scores
        consecutiveSixes[p] = 0; // Reset sixes counter
    }
    currentPlayer = 0;
    currentRank = 1;
    diceValue = 0;
    gameOver = false;
}

// ================================================================
//  REPRESENTATION & RENDER ENGINE (DYNAMIC CELL RESOLUTION)
// ================================================================

// Returns the formatted ASCII representation of a base row for Player p
std::string getBaseRowString(int p, int br) {
    std::string colorCode;
    std::string pName;
    std::string tChar;
    if (p == 0) { colorCode = "\033[91m"; pName = "RED BASE"; tChar = "R"; }
    else if (p == 1) { colorCode = "\033[92m"; pName = "GREEN BASE"; tChar = "G"; }
    else if (p == 2) { colorCode = "\033[93m"; pName = "YELLOW BASE"; tChar = "Y"; }
    else { colorCode = "\033[94m"; pName = "BLUE BASE"; tChar = "B"; }

    std::string reset = "\033[0m";

    if (br == 0 || br == 5) {
        return colorCode + "+----------------+" + reset;
    }
    if (br == 1) {
        std::string inner;
        if (p == 0)      inner = "   RED BASE     ";
        else if (p == 1) inner = "   GREEN BASE   ";
        else if (p == 2) inner = "  YELLOW BASE   ";
        else             inner = "   BLUE BASE    ";
        return colorCode + "|" + reset + "\033[1m" + colorCode + inner + reset + colorCode + "|" + reset;
    }
    if (br == 2) {
        std::string t1 = (tokenPos[p][0] == -1) ? ("[" + tChar + "1]") : "    ";
        std::string t2 = (tokenPos[p][1] == -1) ? ("[" + tChar + "2]") : "    ";
        return colorCode + "|" + reset + "   " + "\033[1m" + colorCode + t1 + reset + "  " + "\033[1m" + colorCode + t2 + reset + "   " + colorCode + "|" + reset;
    }
    if (br == 3) {
        std::string t3 = (tokenPos[p][2] == -1) ? ("[" + tChar + "3]") : "    ";
        std::string t4 = (tokenPos[p][3] == -1) ? ("[" + tChar + "4]") : "    ";
        return colorCode + "|" + reset + "   " + "\033[1m" + colorCode + t3 + reset + "  " + "\033[1m" + colorCode + t4 + reset + "   " + colorCode + "|" + reset;
    }
    if (br == 4) {
        return colorCode + "|                |" + reset;
    }
    return "";
}

// Determines the cell contents at coordinate (r, c) on the track
std::string getCellString(int r, int c) {
    int count = 0;
    int first_p = -1;
    int first_t = -1;
    bool same_player = true;

    // Scan for any active tokens occupying this coordinate
    for (int p = 0; p < 4; ++p) {
        for (int t = 0; t < 4; ++t) {
            int pos = tokenPos[p][t];
            if (pos >= 0) {
                int tr = -1, tc = -1;
                if (pos <= 50) {
                    int abs_index = (startCellIndex[p] + pos) % 52;
                    tr = trackR[abs_index];
                    tc = trackC[abs_index];
                }
                else if (pos <= 55) {
                    int rel = pos - 51;
                    if (p == 0) { tr = 7; tc = 1 + rel; }
                    else if (p == 1) { tr = 1 + rel; tc = 7; }
                    else if (p == 2) { tr = 7; tc = 13 - rel; }
                    else { tr = 13 - rel; tc = 7; }
                }
                else if (pos == 56) {
                    if (p == 0) { tr = 7; tc = 6; }
                    else if (p == 1) { tr = 6; tc = 7; }
                    else if (p == 2) { tr = 7; tc = 8; }
                    else { tr = 8; tc = 7; }
                }

                if (tr == r && tc == c) {
                    count++;
                    if (first_p == -1) {
                        first_p = p;
                        first_t = t;
                    }
                    else if (p != first_p) {
                        same_player = false;
                    }
                }
            }
        }
    }

    std::string reset = "\033[0m";
    std::string bold = "\033[1m";

    // 1. If tokens are occupying the coordinate
    if (count > 0) {
        std::string bgCode = "";

        // Match background style of the target coordinate
        if (r == 6 && c == 1)        bgCode = "\033[41m"; // Red Start
        else if (r == 1 && c == 8)   bgCode = "\033[42m"; // Green Start
        else if (r == 8 && c == 13)  bgCode = "\033[43m"; // Yellow Start
        else if (r == 13 && c == 6)  bgCode = "\033[44m"; // Blue Start
        else if ((r == 8 && c == 2) || (r == 2 && c == 6) || (r == 6 && c == 12) || (r == 12 && c == 8)) {
            bgCode = "\033[46m"; // Cyan Neutral Safe Cells
        }
        else if (r == 7 && c >= 1 && c <= 5)   bgCode = "\033[41m"; // Red Home Path
        else if (c == 7 && r >= 1 && r <= 5)   bgCode = "\033[42m"; // Green Home Path
        else if (r == 7 && c >= 9 && c <= 13)  bgCode = "\033[43m"; // Yellow Home Path
        else if (c == 7 && r >= 9 && r <= 13)  bgCode = "\033[44m"; // Blue Home Path
        else if (r == 7 && c == 6)  bgCode = "\033[41m"; // Red Finish
        else if (r == 6 && c == 7)  bgCode = "\033[42m"; // Green Finish
        else if (r == 7 && c == 8)  bgCode = "\033[43m"; // Yellow Finish
        else if (r == 8 && c == 7)  bgCode = "\033[44m"; // Blue Finish
        else if (r == 7 && c == 7)  bgCode = "\033[45m"; // Center

        std::string fgCode = "";
        if (same_player) {
            if (first_p == 0)      fgCode = "\033[91m";
            else if (first_p == 1) fgCode = "\033[92m";
            else if (first_p == 2) fgCode = "\033[30m\033[1m"; // Black bold on Yellow
            else                   fgCode = "\033[94m";

            // Color adjustments for matching background cell overlays
            if (first_p == 2 && bgCode == "\033[43m") fgCode = "\033[30m";
            if (first_p == 0 && bgCode == "\033[41m") fgCode = "\033[97m";
            if (first_p == 1 && bgCode == "\033[42m") fgCode = "\033[97m";
            if (first_p == 3 && bgCode == "\033[44m") fgCode = "\033[97m";

            std::string tChar;
            if (first_p == 0)      tChar = "R";
            else if (first_p == 1) tChar = "G";
            else if (first_p == 2) tChar = "Y";
            else                   tChar = "B";

            if (count == 1) {
                return bgCode + fgCode + " " + tChar + std::to_string(first_t + 1) + reset;
            }
            else {
                return bgCode + fgCode + " " + tChar + "+" + reset;
            }
        }
        else {
            // Mixed players on a safe cell
            return bgCode + "\033[97m" + " M+ " + reset;
        }
    }

    // 2. Empty cell formatting
    if (r == 6 && c == 1)   return "\033[41m\033[97m[S]\033[0m"; // Red start
    if (r == 1 && c == 8)   return "\033[42m\033[97m[S]\033[0m"; // Green start
    if (r == 8 && c == 13)  return "\033[43m\033[30m[S]\033[0m"; // Yellow start
    if (r == 13 && c == 6)  return "\033[44m\033[97m[S]\033[0m"; // Blue start

    // Other safe cells
    if ((r == 8 && c == 2) || (r == 2 && c == 6) || (r == 6 && c == 12) || (r == 12 && c == 8)) {
        return "\033[46m\033[30m[S]\033[0m"; // Cyan bg, black text
    }

    // Home paths
    if (r == 7 && c >= 1 && c <= 5)   return "\033[41m\033[97m . \033[0m";
    if (c == 7 && r >= 1 && r <= 5)   return "\033[42m\033[97m . \033[0m";
    if (r == 7 && c >= 9 && c <= 13)  return "\033[43m\033[30m . \033[0m";
    if (c == 7 && r >= 9 && r <= 13)  return "\033[44m\033[97m . \033[0m";

    // Finish cells
    if (r == 7 && c == 6) return "\033[41m\033[97m[F]\033[0m";
    if (r == 6 && c == 7) return "\033[42m\033[97m[F]\033[0m";
    if (r == 7 && c == 8) return "\033[43m\033[30m[F]\033[0m";
    if (r == 8 && c == 7) return "\033[44m\033[97m[F]\033[0m";

    // Center cell
    if (r == 7 && c == 7) return "\033[45m\033[97m[C]\033[0m";

    // Standard track cell
    return " . ";
}

// Side-by-side terminal screen print: Ludo Board (left) + Turn & Game Status panel (right)
void renderGameScreen(std::string statusMsg, int diceVal, bool diceRolling) {
    clearScreen();
    for (int r = 0; r < 15; ++r) {
        // --- BOARD RENDER ---
        if (r < 6) {
            std::cout << getBaseRowString(0, r);
            std::cout << getCellString(r, 6) << getCellString(r, 7) << getCellString(r, 8);
            std::cout << getBaseRowString(1, r);
        }
        else if (r >= 6 && r <= 8) {
            for (int c = 0; c < 15; ++c) {
                std::cout << getCellString(r, c);
            }
        }
        else {
            std::cout << getBaseRowString(3, r - 9);
            std::cout << getCellString(r, 6) << getCellString(r, 7) << getCellString(r, 8);
            std::cout << getBaseRowString(2, r - 9);
        }

        // --- GAP SEPARATOR ---
        std::cout << "    ";

        // --- STATUS & SCOREBOARD PANEL RENDER ---
        std::string reset = "\033[0m";
        std::string bold = "\033[1m";

        if (r == 0) {
            std::cout << bold << "\033[35m=== LUDO GAME (4 PLAYERS) ===\033[0m";
        }
        else if (r == 1) {
            std::cout << "---------------------------------";
        }
        else if (r == 2) {
            std::cout << bold << "Player Status & Ranks:" << reset;
        }
        else if (r >= 3 && r <= 6) {
            int p = r - 3;
            std::string colorCode;
            if (p == 0)      colorCode = "\033[91m";
            else if (p == 1) colorCode = "\033[92m";
            else if (p == 2) colorCode = "\033[93m";
            else             colorCode = "\033[94m";

            std::cout << colorCode << "P" << p << " (" << playerNames[p] << "): " << reset;
            std::cout << "Score " << score[p] << "/4";
            if (playerRank[p] > 0) {
                std::cout << " | " << bold << "\033[95mRank: " << playerRank[p] << reset;
            }
            else {
                std::cout << " | Active";
            }
        }
        else if (r == 7) {
            std::cout << "---------------------------------";
        }
        else if (r == 8) {
            std::string colorCode;
            if (currentPlayer == 0)      colorCode = "\033[91m";
            else if (currentPlayer == 1) colorCode = "\033[92m";
            else if (currentPlayer == 2) colorCode = "\033[93m";
            else                         colorCode = "\033[94m";
            std::cout << bold << "Active Turn: " << colorCode << playerNames[currentPlayer] << reset;
        }
        else if (r == 9) {
            if (consecutiveSixes[currentPlayer] > 0) {
                std::cout << "\033[93mConsecutive 6s rolled: " << consecutiveSixes[currentPlayer] << reset;
            }
        }
        else if (r == 10) {
            std::cout << "Dice Value: ";
            if (diceRolling) {
                std::cout << "\033[95m[ Rolling... ]\033[0m";
            }
            else if (diceVal > 0) {
                std::cout << bold << "\033[93m[ " << diceVal << " ]\033[0m";
            }
            else {
                std::cout << "[ - ]";
            }
        }
        else if (r == 11) {
            if (diceRolling || diceVal > 0) {
                std::cout << " +---+";
            }
        }
        else if (r == 12) {
            if (diceRolling) {
                std::cout << " | ? |";
            }
            else if (diceVal > 0) {
                std::cout << " | " << diceVal << " |";
            }
        }
        else if (r == 13) {
            if (diceRolling || diceVal > 0) {
                std::cout << " +---+";
            }
        }
        else if (r == 14) {
            std::cout << bold << "\033[96mStatus: " << statusMsg << reset;
        }

        std::cout << "\n";
    }
}

// ================================================================
//  DICE OPERATIONS & SCROLLING ANIMATION
// ================================================================

int rollDice() {
    int val = 1;
    // Fast display of scrolling values to simulate a real rolling sequence
    for (int i = 0; i < 8; ++i) {
        val = (std::rand() % 6) + 1;
        renderGameScreen("Rolling the dice...", val, true);
        std::cout << '\a' << std::flush; // Play rolling beep sound using standard C++ bell character
        portableSleep(80 + i * 20); // Gets slower at the end of the roll
    }
    renderGameScreen("Rolled a " + std::to_string(val) + "!", val, false);
    // Double beep for the final roll result
    std::cout << '\a' << std::flush;
    portableSleep(100);
    std::cout << '\a' << std::flush;
    portableSleep(300); // Pause to reveal the final roll clearly
    return val;
}

// ================================================================
//  GAME MOVEMENT RULES & VALIDATION
// ================================================================

bool canTokenMove(int player, int token, int roll) {
    int pos = tokenPos[player][token];
    if (pos == -1) {
        return (roll == 6); // Need a 6 to deploy from base
    }
    if (pos == 56) {
        return false; // Already reached the finish
    }
    return (pos + roll <= 56); // Must land exactly on finish (56)
}

bool hasValidMoves(int player, int roll) {
    for (int t = 0; t < 4; ++t) {
        if (canTokenMove(player, t, roll)) {
            return true;
        }
    }
    return false;
}

// Move a token step-by-step to show animation
void animateMovement(int player, int token, int roll) {
    int pos = tokenPos[player][token];
    if (pos == -1) {
        if (roll == 6) {
            tokenPos[player][token] = 0;
            std::cout << '\a' << std::flush; // Deploy sound
            renderGameScreen(playerNames[player] + " deployed Token " + std::to_string(token + 1) + "!", diceValue, false);
            portableSleep(200);
        }
    }
    else {
        for (int i = 0; i < roll; ++i) {
            tokenPos[player][token]++;
            std::cout << '\a' << std::flush; // Stepping sound
            renderGameScreen(playerNames[player] + " is moving Token " + std::to_string(token + 1) + "...", diceValue, false);
            portableSleep(150); // Pause on each cell to represent stepping
        }
    }
}

// Check if moving lands on an opponent and cuts them
void checkCutting(int player, int token, bool& earnedExtraTurn) {
    int pos = tokenPos[player][token];
    if (pos < 0 || pos > 50) return; // Cutting is only valid on the main track

    int absIndex = (startCellIndex[player] + pos) % 52;

    // Check if the landing spot is one of the 8 safe zones
    if (absIndex == 0 || absIndex == 8 || absIndex == 13 || absIndex == 21 ||
        absIndex == 26 || absIndex == 34 || absIndex == 39 || absIndex == 47) {
        return; // Safe zone, cutting blocked
    }

    bool cutAny = false;
    for (int oppP = 0; oppP < 4; ++oppP) {
        if (oppP == player) continue; // Can't cut own player tokens
        for (int oppT = 0; oppT < 4; ++oppT) {
            if (tokenPos[oppP][oppT] >= 0 && tokenPos[oppP][oppT] <= 50) {
                int oppAbsIndex = (startCellIndex[oppP] + tokenPos[oppP][oppT]) % 52;
                if (oppAbsIndex == absIndex) {
                    tokenPos[oppP][oppT] = -1; // Send opponent back to base
                    cutAny = true;
                    // Play multiple beeps for dramatic cutting impact
                    std::cout << '\a' << '\a' << '\a' << std::flush;
                    renderGameScreen("SWIPE! " + playerNames[player] + " cut " + playerNames[oppP] + "'s Token " + std::to_string(oppT + 1) + "!", diceValue, false);
                    portableSleep(700);
                }
            }
        }
    }

    if (cutAny) {
        earnedExtraTurn = true; // Landing on opponent grants an extra roll
    }
}

// ================================================================
//  MENU & INFO SCREEN VIEWS
// ================================================================

void showRulesScreen() {
    clearScreen();
    std::cout << "\033[1;36m==================================================\n";
    std::cout << "                 GAME RULES & PLAY                  \n";
    std::cout << "==================================================\033[0m\n";
    std::cout << "1. Each player has 4 tokens in their respective Bases.\n";
    std::cout << "2. You must roll a \033[1;93m6\033[0m to deploy a token onto the track.\n";
    std::cout << "3. Rolling a \033[1;93m6\033[0m grants you an extra roll.\n";
    std::cout << "4. If you roll three \033[1;93m6\033[0m's consecutively, your turn is cancelled\n";
    std::cout << "   immediately and passes to the next player.\n";
    std::cout << "5. If your token lands on an opponent's token on a normal cell,\n";
    std::cout << "   their token is cut (returned to Base) and you get an extra roll.\n";
    std::cout << "6. Safe cells are marked with \033[1;36m[S]\033[0m. Tokens cannot be cut on safe cells.\n";
    std::cout << "7. To finish, you must roll the exact number required to land on \033[1;95m[F]\033[0m.\n";
    std::cout << "8. The game ends when 3 players have completely finished (Score 4/4).\n";
    std::cout << "\033[1;36m==================================================\033[0m\n";
    std::cout << "Press Enter to return to the Main Menu...";
    std::string discard;
    std::getline(std::cin, discard);
}

void showMainMenu() {
    while (true) {
        clearScreen();
        std::cout << "\033[1;95m";
        std::cout << "  _      _    _ _____   ____   _____          __  __ ______ \n";
        std::cout << " | |    | |  | |  __ \\ / __ \\ / ____|   /\\   |  \\/  |  ____|\n";
        std::cout << " | |    | |  | | |  | | |  | | |  __   /  \\  | \\  / | |__   \n";
        std::cout << " | |    | |  | | |  | | |  | | | |_ | / /\\ \\ | |\\/| |  __|  \n";
        std::cout << " | |____| |__| | |__| | |__| | |__| |/ ____ \\| |  | | |____ \n";
        std::cout << " |______|\\____/|_____/ \\____/ \\_____/_/    \\_\\_|  |_|______|\n";
        std::cout << "\033[0m\n";
        std::cout << "=========================================================\n";
        std::cout << "  1. Start New 4-Player Game\n";
        std::cout << "  2. Read Game Rules\n";
        std::cout << "  3. Exit Game\n";
        std::cout << "=========================================================\n";
        std::cout << "Enter Choice (1-3): ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1") {
            break;
        }
        else if (choice == "2") {
            showRulesScreen();
        }
        else if (choice == "3") {
            std::cout << "Exiting Ludo Game. Goodbye!\n";
            std::exit(0);
        }
        else {
            std::cout << "\033[91mInvalid choice. Press Enter to retry...\033[0m";
            std::getline(std::cin, choice);
        }
    }
}

void getPlayerNamesInput() {
    clearScreen();
    std::cout << "==================================================\n";
    std::cout << "              ENTER PLAYER NAMES                  \n";
    std::cout << "==================================================\n";

    std::string colors[4] = { "Red", "Green", "Yellow", "Blue" };
    std::string colorAnsi[4] = { "\033[91m", "\033[92m", "\033[93m", "\033[94m" };
    std::string reset = "\033[0m";

    for (int p = 0; p < 4; ++p) {
        std::cout << "Enter " << colorAnsi[p] << colors[p] << reset << " Player Name: ";
        std::getline(std::cin, playerNames[p]);
        if (playerNames[p].empty()) {
            playerNames[p] = colors[p] + "Player";
        }
        // Truncate names to fit neatly on status panel
        if (playerNames[p].length() > 12) {
            playerNames[p] = playerNames[p].substr(0, 12);
        }
    }
}

void showGameOverScreen() {
    clearScreen();
    std::cout << "\033[1;95m";
    std::cout << "==================================================\n";
    std::cout << "              G A M E   O V E R                  \n";
    std::cout << "==================================================\033[0m\n";
    std::cout << "               🏆 FINAL RANKINGS 🏆               \n\n";

    // Print ranking order
    for (int rank = 1; rank <= 4; ++rank) {
        for (int p = 0; p < 4; ++p) {
            if (playerRank[p] == rank) {
                std::string medal = "";
                if (rank == 1)      medal = "🥇 [1st Place] - ";
                else if (rank == 2) medal = "🥈 [2nd Place] - ";
                else if (rank == 3) medal = "🥉 [3rd Place] - ";
                else                medal = "🎖️  [4th Place] - ";

                std::cout << "   " << medal << playerNames[p] << "\n";
            }
        }
    }
    std::cout << "\n==================================================\n";
    std::cout << "Press Enter to return to the desktop...";
    std::string discard;
    std::getline(std::cin, discard);
}

// ================================================================
//  MAIN CONTROLLER
// ================================================================

int main() {
    std::srand((unsigned int)std::time(0));

    // Show menu loops
    showMainMenu();
    getPlayerNamesInput();
    initGame();

    // Core Game Loop
    while (!gameOver) {
        bool earnedExtraTurn = false;
        bool threeSixesCancelled = false;

        renderGameScreen("Wait for turn to roll...", 0, false);
        std::cout << "\033[1;96mIt is " << playerNames[currentPlayer] << "'s turn. Press Enter to Roll... \033[0m";
        std::string rollKey;
        std::getline(std::cin, rollKey);

        // Perform dice roll
        diceValue = rollDice();

        // Standard 3-Sixes cancellation check
        if (diceValue == 6) {
            consecutiveSixes[currentPlayer]++;
            if (consecutiveSixes[currentPlayer] == 3) {
                threeSixesCancelled = true;
                consecutiveSixes[currentPlayer] = 0;
                renderGameScreen("Oh no! 3 Sixes rolled! Turn Cancelled!", diceValue, false);
                portableSleep(1200);
            }
        }
        else {
            consecutiveSixes[currentPlayer] = 0; // Reset consecutive 6s
        }

        if (!threeSixesCancelled) {
            // Check if player can make any move
            if (!hasValidMoves(currentPlayer, diceValue)) {
                renderGameScreen("No valid moves available for " + playerNames[currentPlayer] + "!", diceValue, false);
                portableSleep(1200);
            }
            else {
                // Input selection loop for choosing which token to move
                int selectedToken = -1;
                while (true) {
                    renderGameScreen("Choose token (1-4) to move:", diceValue, false);
                    std::cout << "\033[1;96mEnter Token (1-4): \033[0m";
                    std::string inputStr;
                    std::getline(std::cin, inputStr);

                    if (inputStr.length() == 1 && inputStr[0] >= '1' && inputStr[0] <= '4') {
                        int t = inputStr[0] - '1';
                        if (canTokenMove(currentPlayer, t, diceValue)) {
                            selectedToken = t;
                            break;
                        }
                        else {
                            std::cout << "\033[91mToken " << (t + 1) << " cannot make this move. Try again.\033[0m\n";
                            portableSleep(1000);
                        }
                    }
                    else {
                        std::cout << "\033[91mInvalid input. Enter 1, 2, 3, or 4.\033[0m\n";
                        portableSleep(1000);
                    }
                }

                // Execute move animation
                animateMovement(currentPlayer, selectedToken, diceValue);

                // Check and score completion status
                if (tokenPos[currentPlayer][selectedToken] == 56) {
                    score[currentPlayer]++;
                    // Finish sound
                    std::cout << '\a' << '\a' << '\a' << std::flush;
                    renderGameScreen(playerNames[currentPlayer] + "'s Token " + std::to_string(selectedToken + 1) + " finished!", diceValue, false);
                    portableSleep(700);

                    // Check if player has finished all tokens
                    if (score[currentPlayer] == 4) {
                        playerRank[currentPlayer] = currentRank++;
                        std::cout << '\a' << '\a' << '\a' << '\a' << '\a' << std::flush; // Big completion sound!
                        renderGameScreen(playerNames[currentPlayer] + " finished! Rank: " + std::to_string(playerRank[currentPlayer]), diceValue, false);
                        portableSleep(1200);
                    }
                }

                // Handle token cutting logic
                checkCutting(currentPlayer, selectedToken, earnedExtraTurn);
            }
        }

        // Check if game should end (when 3 players have fully finished)
        int finishedCount = 0;
        int lastActivePlayer = -1;
        for (int p = 0; p < 4; ++p) {
            if (playerRank[p] > 0) {
                finishedCount++;
            }
            else {
                lastActivePlayer = p;
            }
        }

        if (finishedCount == 3) {
            playerRank[lastActivePlayer] = 4;
            currentRank = 5;
            gameOver = true;
        }

        if (gameOver) {
            break;
        }

        // Determine who gets the next turn
        bool keepTurn = false;
        if (diceValue == 6 && !threeSixesCancelled) {
            keepTurn = true; // 6 rolled keeps turn
        }
        if (earnedExtraTurn) {
            keepTurn = true; // Cut opponent keeps turn
        }

        if (!keepTurn) {
            // Pass turn to the next player who has not finished
            do {
                currentPlayer = (currentPlayer + 1) % 4;
            } while (playerRank[currentPlayer] > 0);
        }
    }

    // Display final trophy scores and exit game
    showGameOverScreen();
    return 0;
}