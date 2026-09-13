
// ============================================================
//  Console Ping Pong - Beginner-Friendly Rewrite
//  Fixes:
//    1. Right border no longer erased when ball goes off right.
//    2. Ball artifact cleared when it goes off the left side.
//    3. New Controls menu (view + customize + duplicate check).
//    4. Settings menu actually opens now.
//    5. Ball serve direction (left/right) and vertical angle
//       (up/down) are randomized on every serve.
//    6. Game Over screen with Play Again / Main Menu / Exit.
//    7. Full 4-player knockout Tournament Mode.
//    8. In 1v1 both players' input is captured every frame
//       (previous version only read one key per frame).
//    9. Small extra fixes: safer input, clean redraw, etc.
// ============================================================

#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <cmath>
#include <algorithm>

using namespace std;

// ======================== Colors ========================
const char *RESET   = "\033[0m";
const char *CYAN    = "\033[1;36m";
const char *YELLOW  = "\033[1;33m";
const char *GREEN   = "\033[1;32m";
const char *RED     = "\033[1;31m";
const char *BLUE    = "\033[1;34m";
const char *WHITE   = "\033[1;37m";
const char *MAGENTA = "\033[0;35m";

// ======================== Settings ========================
struct GameSettings
{
    int  difficulty    = 2;       // 1=Easy, 2=Medium, 3=Hard
    int  ballSpeed     = 1;
    int  paddleSpeed   = 2;
    int  winningScore  = 5;
    bool soundON       = true;
    int  aiType        = 2;       // 1=Simple, 2=Predictive

    const char* paddle1Col = CYAN;
    const char* paddle2Col = GREEN;
    const char* ballCol    = RED;
    const char* borderCol  = WHITE;

    // Player 1 controls
    char p1Up   = 'w';
    char p1Down = 's';
    // Player 2 controls
    char p2Up   = 'i';
    char p2Down = 'k';
    // General keys
    char pauseKey   = 'p';
    char restartKey = 'r';
    char exitKey    = 'x';
} settings;

struct MatchStats
{
    string winner;
    int p1Score = 0, p2Score = 0;
    int totalHits = 0, currentRally = 0, longestRally = 0;
    int wallHits = 0;
    int timePlayed = 0;
    int maxSpeedReached = 1;
} stats;

// ======================== Globals ========================
int width = 80, height = 20, paddle_height = 5;
int Ballx, Bally, Ball_velocity_x, Ball_velocity_y;
int speedLevel = 1;
int score1 = 0, score2 = 0;
int paddle1, paddle2;
int prev_Ball_X, prev_Ball_y, prev_paddle1, prev_paddle2;
bool game_runner = true;
bool game_paused = false;
time_t matchStartTime;
string username = "Guest";

// Names used for labels on the scoreboard/winner text.
string p1Name = "Player 1";
string p2Name = "Player 2";

// ======================== Small Helpers ========================
void set_position(int x, int y)
{
    COORD axis = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), axis);
}

void show_cursor(bool visible)
{
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    ci.bVisible = visible;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

void playBeep()
{
    if (settings.soundON) Beep(500, 50);
}
void playScoreBeep()
{
    if (settings.soundON) Beep(800, 150);
}

// Small helper: is a character printable & typeable?
bool isValidKey(char c)
{
    return c >= 33 && c <= 126; // any printable non-space
}

// ======================== High Score Storage ========================
struct ScoreRecord
{
    string name;
    int score;
    int diff;
    string date;
    int time;
};

vector<ScoreRecord> loadScores()
{
    vector<ScoreRecord> records;
    ifstream fin("highscores.txt");
    string name, date;
    int score, diff, time_p;
    while (fin >> name >> score >> diff >> date >> time_p)
        records.push_back({name, score, diff, date, time_p});
    return records;
}

void saveScore(int score)
{
    vector<ScoreRecord> records = loadScores();
    records.push_back({username, score, settings.difficulty, "Recent", stats.timePlayed});
    stable_sort(records.begin(), records.end(),
                [](const ScoreRecord &a, const ScoreRecord &b)
    {
        return a.score > b.score;
    });
    ofstream fout("highscores.txt");
    for (int i = 0; i < min(10, (int)records.size()); i++)
        fout << records[i].name << " " << records[i].score << " "
             << records[i].diff << " " << records[i].date << " "
             << records[i].time << "\n";
}

// ======================== Ball & AI ========================
// FIX #4: ball serve is now fully random in both X and Y directions.
void ball_spawn()
{
    speedLevel = settings.ballSpeed;
    Ballx = width / 2;
    Bally = height / 2;

    // Random horizontal direction: left (-1) or right (+1)
    int dirX = (rand() % 2 == 0) ? -1 : 1;
    // Random vertical direction: up (-1) or down (+1)
    int dirY = (rand() % 2 == 0) ? -1 : 1;

    Ball_velocity_x = dirX * speedLevel;
    Ball_velocity_y = dirY; // start with a gentle angle

    stats.currentRally = 0;
}

int predictBallY(int startX, int startY, int vx, int vy, int targetX)
{
    if (vx == 0) return height / 2;
    if ((targetX > startX && vx < 0) || (targetX < startX && vx > 0)) return height / 2;
    int x = startX, y = startY, velY = vy;
    while (x != targetX)
    {
        x += vx;
        y += velY;
        if (y <= 0)
        {
            y = 1;
            velY =  abs(velY);
        }
        else if (y >= height - 1)
        {
            y = height - 2;
            velY = -abs(velY);
        }
    }
    return y;
}

void aiController()
{
    int targetY = height / 2;
    if (settings.aiType == 1)
        targetY = Bally - paddle_height / 2;
    else
        targetY = predictBallY(Ballx, Bally, Ball_velocity_x, Ball_velocity_y, width - 3)
                  - paddle_height / 2;

    if (settings.difficulty == 1 && (rand() % 100 < 30))
        targetY += (rand() % 5 - 2);
    targetY = max(0, min(height - paddle_height, targetY));

    int moveStep = settings.paddleSpeed;
    if (settings.difficulty == 1) moveStep = max(1, moveStep - 1);
    if (settings.difficulty == 3) moveStep += 1;

    if (paddle2 < targetY) paddle2 = min(paddle2 + moveStep, targetY);
    else if (paddle2 > targetY) paddle2 = max(paddle2 - moveStep, targetY);
}

// ======================== Rendering ========================
void draw_interface()
{
    system("cls");
    cout << settings.borderCol;
    for (int i = 0; i < width + 2; i++) cout << "#";
    for (int j = 0; j < height; j++)
    {
        set_position(0, j + 1);
        cout << settings.borderCol << "#";
        set_position(width + 1, j + 1);
        cout << settings.borderCol << "#";
        set_position(width / 2 + 1, j + 1);
        cout << RESET << "|";
    }
    set_position(0, height + 1);
    for (int i = 0; i < width + 2; i++) cout << "#";

    set_position(2, height + 3);
    cout << CYAN << p1Name << ": " << score1
         << " | " << p2Name << ": " << score2
         << "   " << YELLOW << "Target: " << settings.winningScore;
    set_position(2, height + 4);
    cout << MAGENTA << "Speed: " << speedLevel << "x | Rally: "
         << stats.currentRally << RESET;
}

// FIX #1 & #2: always redraw center line AND the side borders where
// the ball used to be, so no border character is ever erased and no
// leftover "O" is left behind after a goal.
void in_game()
{
    // Clear previous ball position.
    if (prev_Ball_y > 0 && prev_Ball_y < height + 1)
    {
        set_position(prev_Ball_X + 1, prev_Ball_y + 1);
        if (prev_Ball_X + 1 == 0 || prev_Ball_X + 1 == width + 1)
            cout << settings.borderCol << "#" << RESET;   // side border
        else if (prev_Ball_X == width / 2)
            cout << RESET << "|";                         // center line
        else
            cout << " ";
    }

    // Also make sure both side borders are still present on the row
    // where the ball currently is (defensive redraw – covers the case
    // where the ball flew off screen and prev_Ball_X was outside).
    for (int row = 1; row <= height; row++)
    {
        set_position(0, row);
        cout << settings.borderCol << "#";
        set_position(width + 1, row);
        cout << settings.borderCol << "#" << RESET;
    }

    // Clear previous paddles.
    for (int i = 0; i < paddle_height; i++)
    {
        set_position(1, prev_paddle1 + i + 1);
        cout << " ";
        set_position(width - 1, prev_paddle2 + i + 1);
        cout << " ";
    }

    // Draw ball (only if it is on-screen).
    if (Ballx >= 0 && Ballx <= width && Bally >= 0 && Bally < height)
    {
        set_position(Ballx + 1, Bally + 1);
        cout << settings.ballCol << "O" << RESET;
    }

    // Draw paddles.
    for (int i = 0; i < paddle_height; i++)
    {
        set_position(1, paddle1 + i + 1);
        cout << settings.paddle1Col << "|";
        set_position(width - 1, paddle2 + i + 1);
        cout << settings.paddle2Col << "|";
    }

    // Refresh score line.
    set_position(2, height + 3);
    cout << CYAN << p1Name << ": " << score1
         << " | " << p2Name << ": " << score2
         << "   " << YELLOW << "Target: " << settings.winningScore
         << "     " << RESET;
    set_position(2, height + 4);
    cout << MAGENTA << "Speed: " << speedLevel
         << "x | Rally: " << stats.currentRally << "   " << RESET;

    prev_Ball_X = Ballx;
    prev_Ball_y = Bally;
    prev_paddle1 = paddle1;
    prev_paddle2 = paddle2;
}

// ======================== Physics ========================
void main_logic()
{
    Ballx += Ball_velocity_x;
    Bally += Ball_velocity_y;

    if (Bally <= 0)
    {
        Bally = 1;
        Ball_velocity_y = abs(Ball_velocity_y);
        stats.wallHits++;
    }
    else if (Bally >= height - 1)
    {
        Bally = height - 2;
        Ball_velocity_y = -abs(Ball_velocity_y);
        stats.wallHits++;
    }

    // Left paddle collision
    if (Ballx <= 2 && prev_Ball_X >= 2 &&
            Bally >= paddle1 && Bally < paddle1 + paddle_height)
    {
        Ballx = 3;
        Ball_velocity_x = abs(Ball_velocity_x);
        int hitPoint = Bally - paddle1;
        if (hitPoint == 0) Ball_velocity_y = -2;
        else if (hitPoint == paddle_height - 1) Ball_velocity_y = 2;

        if (speedLevel < 5) speedLevel++;
        stats.totalHits++;
        stats.currentRally++;
        stats.maxSpeedReached = max(stats.maxSpeedReached, speedLevel);
        stats.longestRally    = max(stats.longestRally,    stats.currentRally);
        playBeep();
    }

    // Right paddle collision
    if (Ballx >= width - 3 && prev_Ball_X <= width - 3 &&
            Bally >= paddle2 && Bally < paddle2 + paddle_height)
    {
        Ballx = width - 4;
        Ball_velocity_x = -abs(Ball_velocity_x);
        int hitPoint = Bally - paddle2;
        if (hitPoint == 0) Ball_velocity_y = -2;
        else if (hitPoint == paddle_height - 1) Ball_velocity_y = 2;

        if (speedLevel < 5) speedLevel++;
        stats.totalHits++;
        stats.currentRally++;
        stats.maxSpeedReached = max(stats.maxSpeedReached, speedLevel);
        stats.longestRally    = max(stats.longestRally,    stats.currentRally);
        playBeep();
    }

    // Goals — clear ball before it respawns (FIX #2)
    if (Ballx < 0)
    {
        score2++;
        playScoreBeep();
        // Erase ball's last drawn cell so no artifact is left.
        if (prev_Ball_y > 0 && prev_Ball_y < height + 1 &&
                prev_Ball_X > 0 && prev_Ball_X < width)
        {
            set_position(prev_Ball_X + 1, prev_Ball_y + 1);
            cout << (prev_Ball_X == width / 2 ? "|" : " ");
        }
        ball_spawn();
        draw_interface();
    }
    else if (Ballx > width)
    {
        score1++;
        playScoreBeep();
        if (prev_Ball_y > 0 && prev_Ball_y < height + 1 &&
                prev_Ball_X > 0 && prev_Ball_X < width)
        {
            set_position(prev_Ball_X + 1, prev_Ball_y + 1);
            cout << (prev_Ball_X == width / 2 ? "|" : " ");
        }
        ball_spawn();
        draw_interface();
    }
}

// ======================== Input ========================
// FIX #8: drain the input buffer every frame so BOTH players'
// key presses are processed on the same frame in 1v1 mode.
void controller(bool isTwoPlayer)
{
    while (_kbhit())
    {
        char key = tolower(_getch());

        if (key == settings.p1Up)
            paddle1 = max(0, paddle1 - settings.paddleSpeed);
        else if (key == settings.p1Down)
            paddle1 = min(height - paddle_height, paddle1 + settings.paddleSpeed);

        if (isTwoPlayer)
        {
            if (key == settings.p2Up)
                paddle2 = max(0, paddle2 - settings.paddleSpeed);
            else if (key == settings.p2Down)
                paddle2 = min(height - paddle_height, paddle2 + settings.paddleSpeed);
        }

        if (key == settings.pauseKey)  game_paused = !game_paused;
        if (key == settings.restartKey)
        {
            score1 = 0;
            score2 = 0;
            ball_spawn();
            draw_interface();
        }
        if (key == settings.exitKey) game_runner = false;
    }
}

// ======================== Game Over Screen ========================
// FIX #5: after a match ends, wait for R / M / E.
// Returns: 'r' = play again, 'm' = main menu, 'e' = exit program.
char game_over_screen(const string &winnerName)
{
    system("cls");
    cout << YELLOW << "\n\n\t================= GAME OVER =================\n\n" << RESET;
    cout << "\t" << GREEN  << "Winner: " << winnerName << RESET << "\n";
    cout << "\t" << CYAN   << "Final Score: " << p1Name << " " << score1
         << " - " << score2 << " " << p2Name << RESET << "\n\n";
    cout << "\tTotal Hits   : " << stats.totalHits    << "\n";
    cout << "\tLongest Rally: " << stats.longestRally << "\n";
    cout << "\tTime Played  : " << stats.timePlayed   << "s\n\n";
    cout << "\t" << MAGENTA << "Press R to Play Again\n";
    cout << "\tPress M to Return to Main Menu\n";
    cout << "\tPress E to Exit" << RESET << "\n";

    while (true)
    {
        char key = tolower(_getch());
        if (key == 'r' || key == 'm' || key == 'e') return key;
    }
}

// ======================== Match Runner ========================
// Returns final result key from game_over_screen, OR '\0' when the
// caller (e.g. tournament) doesn't want the game-over screen shown.
char run_match(bool isTwoPlayer, bool showGameOver = true)
{
    score1 = 0;
    score2 = 0;
    stats  = MatchStats();
    paddle1 = height / 2 - paddle_height / 2;
    paddle2 = paddle1;
    prev_paddle1 = paddle1;
    prev_paddle2 = paddle2;
    prev_Ball_X  = width / 2;
    prev_Ball_y  = height / 2;
    ball_spawn();
    draw_interface();
    matchStartTime = time(0);
    game_runner = true;
    game_paused = false;

    while (game_runner)
    {
        if (!game_paused)
        {
            in_game();
            controller(isTwoPlayer);
            if (!isTwoPlayer) aiController();
            main_logic();

            if (score1 >= settings.winningScore || score2 >= settings.winningScore)
            {
                game_runner = false;
                stats.p1Score = score1;
                stats.p2Score = score2;
                stats.timePlayed = (int)difftime(time(0), matchStartTime);
                stats.winner = (score1 > score2) ? p1Name : p2Name;
                saveScore(max(score1, score2));
            }
        }
        else
        {
            set_position(width / 2 - 4, height / 2);
            cout << RED << "[ PAUSED ]" << RESET;
            controller(isTwoPlayer);
            if (!game_paused) draw_interface();
        }
        Sleep(50);
    }

    if (!showGameOver) return '\0';
    return game_over_screen(stats.winner);
}

// ======================== Tournament (FIX #6) ========================
// 4-player knockout: (P1 vs P2) & (P3 vs P4) -> Final.
void tournament_mode()
{
    system("cls");
    show_cursor(true);
    cout << YELLOW << "\n\t============ TOURNAMENT MODE ============\n\n" << RESET;

    string players[4];
    for (int i = 0; i < 4; i++)
    {
        cout << "\tEnter name for Player " << (i + 1) << ": ";
        cin  >> players[i];
    }
    show_cursor(false);

    // Show bracket
    system("cls");
    cout << YELLOW << "\n\tTOURNAMENT BRACKET\n\n" << RESET;
    cout << "\t" << players[0] << "  \\\n";
    cout << "\t              >-- Winner 1 --\\\n";
    cout << "\t" << players[1] << "  /               \\\n";
    cout << "\t                                  FINAL --> Champion\n";
    cout << "\t" << players[2] << "  \\               /\n";
    cout << "\t              >-- Winner 2 --/\n";
    cout << "\t" << players[3] << "  /\n\n";
    cout << "\tPress any key to start Match 1...";
    _getch();

    int savedWinningScore = settings.winningScore;

    // --- Match 1 ---
    p1Name = players[0];
    p2Name = players[1];
    run_match(true, false);
    string winner1 = (score1 > score2) ? players[0] : players[1];

    system("cls");
    cout << GREEN << "\n\tMatch 1 Winner: " << winner1 << RESET << "\n\n";
    cout << "\tPress any key to start Match 2...";
    _getch();

    // --- Match 2 ---
    p1Name = players[2];
    p2Name = players[3];
    run_match(true, false);
    string winner2 = (score1 > score2) ? players[2] : players[3];

    system("cls");
    cout << GREEN << "\n\tMatch 2 Winner: " << winner2 << RESET << "\n\n";

    // --- Final target score ---
    show_cursor(true);
    int finalTarget = 5;
    cout << YELLOW << "\tBefore the Final: choose target score (1-20): " << RESET;
    cin  >> finalTarget;
    if (finalTarget < 1)  finalTarget = 1;
    if (finalTarget > 20) finalTarget = 20;
    settings.winningScore = finalTarget;
    show_cursor(false);

    cout << "\n\tThe Final: " << winner1 << " vs " << winner2 << "\n";
    cout << "\tPress any key to begin...";
    _getch();

    // --- Final ---
    p1Name = winner1;
    p2Name = winner2;
    run_match(true, false);
    string champion = (score1 > score2) ? winner1 : winner2;

    system("cls");
    cout << YELLOW << "\n\t======================================\n";
    cout << "\t     TOURNAMENT CHAMPION: " << champion << "\n";
    cout << "\t======================================\n\n" << RESET;
    cout << "\tPress any key to return to menu...";
    _getch();

    // Restore defaults for regular modes
    settings.winningScore = savedWinningScore;
    p1Name = "Player 1";
    p2Name = "Player 2";
}

// ======================== Controls Menu (FIX #3 helper) ========================
// Displays and lets the user customize controls.
// Duplicate keys are rejected.
void controls_menu()
{
    while (true)
    {
        system("cls");
        cout << YELLOW << "\n\t========== CONTROLS ==========\n\n" << RESET;
        cout << "\t" << CYAN  << "Player 1\n" << RESET;
        cout << "\t  Move Up   : " << (char)toupper(settings.p1Up)   << "\n";
        cout << "\t  Move Down : " << (char)toupper(settings.p1Down) << "\n\n";
        cout << "\t" << GREEN << "Player 2\n" << RESET;
        cout << "\t  Move Up   : " << (char)toupper(settings.p2Up)   << "\n";
        cout << "\t  Move Down : " << (char)toupper(settings.p2Down) << "\n\n";
        cout << "\t" << MAGENTA << "General\n" << RESET;
        cout << "\t  Pause     : " << (char)toupper(settings.pauseKey)   << "\n";
        cout << "\t  Restart   : " << (char)toupper(settings.restartKey) << "\n";
        cout << "\t  Exit Game : " << (char)toupper(settings.exitKey)    << "\n\n";
        cout << "\tPress " << YELLOW << "C" << RESET << " to change controls.\n";
        cout << "\tPress " << YELLOW << "B" << RESET << " to go back.\n";

        char key = tolower(_getch());
        if (key == 'b') return;
        if (key != 'c') continue;

        // Ask for each key, one by one.
        const char* labels[] =
        {
            "Player 1 Up", "Player 1 Down",
            "Player 2 Up", "Player 2 Down",
            "Pause", "Restart", "Exit Game"
        };
        char* targets[] =
        {
            &settings.p1Up, &settings.p1Down,
            &settings.p2Up, &settings.p2Down,
            &settings.pauseKey, &settings.restartKey, &settings.exitKey
        };
        const int N = 7;

        // Work on a temporary copy — only commit if the whole set is valid.
        char newKeys[N];
        for (int i = 0; i < N; i++) newKeys[i] = *targets[i];

        bool cancelled = false;
        for (int i = 0; i < N && !cancelled; i++)
        {
            while (true)
            {
                system("cls");
                cout << YELLOW << "\n\tSet key for: " << RESET << labels[i] << "\n";
                cout << "\t(Current: " << (char)toupper(newKeys[i]) << ")\n";
                cout << "\tPress the new key, or ESC to cancel: ";
                char k = _getch();
                if (k == 27)
                {
                    cancelled = true;    // ESC
                    break;
                }
                k = tolower(k);
                if (!isValidKey(k))
                {
                    cout << "\n\t" << RED << "Invalid key. Try again..." << RESET;
                    Sleep(900);
                    continue;
                }
                // Duplicate check against already-set new keys.
                bool duplicate = false;
                for (int j = 0; j < i; j++)
                    if (newKeys[j] == k)
                    {
                        duplicate = true;
                        break;
                    }
                if (duplicate)
                {
                    cout << "\n\t" << RED << "That key is already used. Choose another." << RESET;
                    Sleep(1200);
                    continue;
                }
                newKeys[i] = k;
                break;
            }
        }

        if (cancelled)
        {
            cout << "\n\t" << RED << "Cancelled. Controls unchanged." << RESET;
            Sleep(1000);
            continue;
        }

        // Commit new keys.
        for (int i = 0; i < N; i++) *targets[i] = newKeys[i];
        cout << "\n\t" << GREEN << "Controls saved!" << RESET;
        Sleep(1000);
    }
}

// ======================== Settings Menu (FIX #3) ========================
void settings_menu()
{
    while (true)
    {
        system("cls");
        cout << YELLOW << "\n\t========== SETTINGS ==========\n\n" << RESET;
        cout << "\t1. Difficulty     : "
             << (settings.difficulty == 1 ? "Easy" :
                 settings.difficulty == 2 ? "Medium" : "Hard") << "\n";
        cout << "\t2. Ball Speed     : " << settings.ballSpeed   << "\n";
        cout << "\t3. Paddle Speed   : " << settings.paddleSpeed << "\n";
        cout << "\t4. Winning Score  : " << settings.winningScore << "\n";
        cout << "\t5. Sound          : " << (settings.soundON ? "ON" : "OFF") << "\n";
        cout << "\t6. AI Type        : "
             << (settings.aiType == 1 ? "Simple" : "Predictive") << "\n";
        cout << "\n\tPress 1-6 to change, B to go back.\n";

        char key = tolower(_getch());
        if (key == 'b') return;

        show_cursor(true);
        if (key == '1')
        {
            cout << "\n\tEnter difficulty (1=Easy, 2=Medium, 3=Hard): ";
            int v;
            cin >> v;
            if (v >= 1 && v <= 3) settings.difficulty = v;
        }
        else if (key == '2')
        {
            cout << "\n\tEnter ball speed (1-3): ";
            int v;
            cin >> v;
            if (v >= 1 && v <= 3) settings.ballSpeed = v;
        }
        else if (key == '3')
        {
            cout << "\n\tEnter paddle speed (1-4): ";
            int v;
            cin >> v;
            if (v >= 1 && v <= 4) settings.paddleSpeed = v;
        }
        else if (key == '4')
        {
            cout << "\n\tEnter winning score (1-20): ";
            int v;
            cin >> v;
            if (v >= 1 && v <= 20) settings.winningScore = v;
        }
        else if (key == '5')
        {
            settings.soundON = !settings.soundON;
        }
        else if (key == '6')
        {
            cout << "\n\tEnter AI type (1=Simple, 2=Predictive): ";
            int v;
            cin >> v;
            if (v == 1 || v == 2) settings.aiType = v;
        }
        show_cursor(false);
    }
}

// ======================== Stats / Highscores ========================
void display_stats()
{
    system("cls");
    cout << YELLOW << "\n\n\t================ LAST MATCH STATS ================\n" << RESET;
    if (stats.winner.empty())
    {
        cout << "\n\tNo match played yet.\n";
    }
    else
    {
        cout << "\tWinner            : " << stats.winner       << "\n";
        cout << "\tFinal Score       : " << stats.p1Score << " - " << stats.p2Score << "\n";
        cout << "\tTotal Paddle Hits : " << stats.totalHits    << "\n";
        cout << "\tLongest Rally     : " << stats.longestRally << "\n";
        cout << "\tWall Bounces      : " << stats.wallHits     << "\n";
        cout << "\tMax Ball Speed    : " << stats.maxSpeedReached << "x\n";
        cout << "\tTime Played       : " << stats.timePlayed   << "s\n";
    }
    cout << "\n\tPress any key to return to Main Menu...";
    _getch();
}

void show_highscores()
{
    system("cls");
    cout << YELLOW << "\n\t=== HIGH SCORES ===\n" << RESET;
    vector<ScoreRecord> recs = loadScores();
    if (recs.empty())
        cout << "\n\tNo scores yet!";
    else
    {
        cout << "\n\tName\tScore\tDiff\tTime\n";
        cout << "\t----------------------------------\n";
        for (auto &r : recs)
            cout << "\t" << r.name << "\t" << r.score << "\t"
                 << r.diff << "\t" << r.time << "s\n";
    }
    cout << "\n\tPress any key to return...";
    _getch();
}

void algorithm_demo()
{
    system("cls");
    cout << YELLOW << "\n\t=== AI ALGORITHM DEMO ===\n\n" << RESET;
    cout << CYAN  << "1. Simple AI (Reactive)\n" << RESET;
    cout << "   - Follows the ball's current Y position.\n";
    cout << "   - Very lightweight, feels human on easy mode.\n\n";
    cout << GREEN << "2. Predictive AI (Proactive)\n" << RESET;
    cout << "   - Simulates bounces to predict where the ball will arrive.\n";
    cout << "   - Very hard to beat.\n\n";
    cout << "\tPress any key to return...";
    _getch();
}

// ======================== Main Menu ========================
int Main_Menu()
{
    int button = 0;
    string options[] =
    {
        "Single Player",
        "Two Player (1v1)",
        "Tournament Mode",
        "Controls",
        "Settings",
        "High Scores",
        "Statistics (Last Match)",
        "Algorithm Demo",
        "Exit"
    };
    const int N = 9;

    while (true)
    {
        system("cls");
        cout << YELLOW << "\n\n\t================= PING PONG MASTER =================\n\n" << RESET;
        cout << "\tWelcome, " << CYAN << username << RESET << "\n\n";

        for (int i = 0; i < N; i++)
        {
            if (button == i) cout << BLUE << "\t-> " << options[i] << " <-\n" << RESET;
            else             cout << "\t   " << options[i] << "\n";
        }
        cout << "\n\t(Use W/S to move, ENTER to select)\n";

        char key = tolower(_getch());
        if (key == 'w' && button > 0)     button--;
        if (key == 's' && button < N - 1) button++;
        if (key == 13 || key == ' ')      return button + 1;
    }
}

// ======================== Main ========================
int main()
{
    srand((unsigned)time(0));
    show_cursor(false);

    system("cls");
    show_cursor(true);
    set_position(20, 5);
    cout << "Enter your Player Name: ";
    cin  >> username;
    show_cursor(false);

    p1Name = username;
    p2Name = "Player 2";

    bool running = true;
    while (running)
    {
        int choice = Main_Menu();

        if (choice == 1) // Single Player
        {
            p1Name = username;
            p2Name = "AI";
            char again;
            do
            {
                again = run_match(false, true);
                if (again == 'e')
                {
                    running = false;
                    break;
                }
                if (again == 'm') break;
            }
            while (again == 'r');
        }
        else if (choice == 2) // Two Player
        {
            p1Name = username;
            p2Name = "Player 2";
            char again;
            do
            {
                again = run_match(true, true);
                if (again == 'e')
                {
                    running = false;
                    break;
                }
                if (again == 'm') break;
            }
            while (again == 'r');
        }
        else if (choice == 3) tournament_mode();
        else if (choice == 4) controls_menu();
        else if (choice == 5) settings_menu();          // FIX #3
        else if (choice == 6) show_highscores();
        else if (choice == 7) display_stats();
        else if (choice == 8) algorithm_demo();
        else if (choice == 9) running = false;
    }

    show_cursor(true);
    system("cls");
    cout << "\n\tThanks for playing!\n\n";
    return 0;
}
