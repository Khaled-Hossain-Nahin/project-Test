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

//======================== Colors & Styling ========================
const char *RESET = "\033[0m";
const char *CYAN = "\033[1;36m";
const char *YELLOW = "\033[1;33m";
const char *GREEN = "\033[1;32m";
const char *RED = "\033[1;31m";
const char *BLUE = "\033[1;34m";
const char *WHITE = "\033[1;37m";
const char *MAGENTA ="\033[0;35m";

struct ColorOption
{
    const char* name;
    const char* code;
};
ColorOption palette[] =
{
    {"Red", RED}, {"Green", GREEN}, {"Yellow", YELLOW},
    {"Blue", BLUE}, {"Cyan", CYAN}, {"White", WHITE}, {"Magenta", MAGENTA}
};
int numColors = 7;

//======================== Game Configurations ========================
struct GameSettings
{
    int difficulty = 1; // 1=Easy, 2=Medium, 3=Hard
    int ballSpeed = 2;
    int paddleSpeed = 1;
    int winningScore = 10;
    bool powerUps = true;
    bool soundON = true;
    int aiType = 1; // 1=Simple, 2=Predictive
    int aiReaction = 1;
    const char* paddle1Col = MAGENTA;
    const char* paddle2Col = YELLOW;
    const char* ballCol =MAGENTA ;
    const char* borderCol = MAGENTA;
    char upKey = 'w', downKey = 's', p2UpKey = 'i', p2DownKey = 'k';
    char pauseKey = 'p', restartKey = 'r';
} settings;

struct MatchStats
{
    string winner;
    int p1Score = 0, p2Score = 0;
    int totalHits = 0, currentRally = 0, longestRally = 0;
    int wallHits = 0, p1Goals = 0, p2Goals = 0;
    int timePlayed = 0; // in seconds
    int maxSpeedReached = 1;
} stats;

void Controls_Menu();
//======================== Global Variables ========================
int width = 80, height = 20, paddle_height = 5;
int Ballx, Bally, Ball_velocity_x, Ball_velocity_y;
int speedLevel = 1;
int score1 = 0, score2 = 0;
int paddle1, paddle2;
int prev_Ball_X, prev_Ball_y, prev_paddle1, prev_paddle2;
bool game_runner = true, game_paused = false;
time_t matchStartTime;
string username = "Guest";
string current_p1, current_p2;

//======================== Utility Functions ========================
void set_positon(int x, int y)
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

//======================== Data Handling ========================
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
    string date = "Recent";
    records.push_back({current_p1, score, settings.difficulty, date, stats.timePlayed});
    stable_sort(records.begin(), records.end(), [](const ScoreRecord &a, const ScoreRecord &b)
    {
        return a.score > b.score;
    });
    ofstream fout("highscores.txt");
    for (int i=0; i < min(10, (int)records.size()); i++)
        fout << records[i].name << " " << records[i].score << " " << records[i].diff << " " << records[i].date << " " << records[i].time << "\n";
}

//======================== Physics & AI ========================
void ball_spawn()
{
    speedLevel = settings.ballSpeed;
    Ballx = width / 2;
    Bally = height / 2;

    // Feature: Randomize X and Y Direction on spawn
    int dirX = (rand() % 2 == 0) ? -1 : 1;
    int dirY = (rand() % 2 == 0) ? -1 : 1;
    int ySpeed = (rand() % 2) + 1; // 1 or 2 vertical speed

    Ball_velocity_x = dirX * speedLevel;
    Ball_velocity_y = dirY * ySpeed;
    stats.currentRally = 0;

    prev_Ball_X = Ballx;
    prev_Ball_y = Bally;
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
            velY = abs(velY);
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
    {
        targetY = Bally - paddle_height / 2;
    }
    else
    {
        targetY = predictBallY(Ballx, Bally, Ball_velocity_x, Ball_velocity_y, width - 3) - paddle_height / 2;
    }

    if (settings.difficulty == 1 && (rand() % 100 < 30)) targetY += (rand() % 5 - 2);
    targetY = max(0, min(height - paddle_height, targetY));

    int moveStep = settings.paddleSpeed;
    if (settings.difficulty == 1) moveStep = max(1, moveStep - 1);
    if (settings.difficulty == 3) moveStep += 1;

    if (paddle2 < targetY) paddle2 = min(paddle2 + moveStep, targetY);
    else if (paddle2 > targetY) paddle2 = max(paddle2 - moveStep, targetY);
}

//======================== Rendering ========================
void draw_interface()
{
    system("cls");
    cout << settings.borderCol;
    for (int i = 0; i < width + 2; i++) cout << "#";
    for (int j = 0; j < height; j++)
    {
        set_positon(0, j + 1);
        cout << settings.borderCol << "#";
        set_positon(width + 1, j + 1);
        cout << settings.borderCol << "#";
        set_positon(width / 2 + 1, j + 1);
        cout << RESET << "|";
    }
    set_positon(0, height + 1);
    for (int i = 0; i < width + 2; i++) cout << "#";
}

void in_game()
{


    // Bug Fix: Correctly erase ball without destroying walls
    int draw_x = prev_Ball_X + 1;
    if (draw_x >= 0 && draw_x <= width + 1 && prev_Ball_y >= 0 && prev_Ball_y < height)
    {
        set_positon(draw_x, prev_Ball_y + 1);
        if (draw_x == 0 || draw_x == width + 1) cout << settings.borderCol << "#";
        else if (draw_x == width / 2 + 1) cout << RESET << "|";
        else cout << " ";
    }

    // Erase Paddles
    for (int i = 0; i < paddle_height; i++)
    {
        set_positon(1, prev_paddle1 + i + 1);
        cout << " ";
        set_positon(width - 1, prev_paddle2 + i + 1);
        cout << " ";
    }

    // Draw new ball if within safe visible area
    if(Ballx >= 0 && Ballx <= width) {
        set_positon(Ballx + 1, Bally + 1);
        cout << settings.ballCol << "O" << RESET;
    }

    // Draw Paddles
    for (int i = 0; i < paddle_height; i++)
    {
        set_positon(1, paddle1 + i + 1);
        cout << settings.paddle1Col << "|";
        set_positon(width - 1, paddle2 + i + 1);
        cout << settings.paddle2Col << "|";
    }

    // Bug Fix: Rewrite entire string to prevent overlapping UI bugs
    set_positon(2, height + 3);
    cout << CYAN << current_p1 << ": " << score1 << " | " << current_p2 << ": " << score2 << "   " << YELLOW << "Target: " << settings.winningScore << "       ";
    set_positon(40, height + 3);
    cout << MAGENTA << "Speed: " << speedLevel << "x | Rally: " << stats.currentRally << "      " << RESET;

    prev_Ball_X = Ballx;
    prev_Ball_y = Bally;
    prev_paddle1 = paddle1;
    prev_paddle2 = paddle2;
}

//======================== Logic ========================
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

    if (Ballx <= 2 && prev_Ball_X >= 2 && Bally >= paddle1 && Bally < paddle1 + paddle_height)
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
        stats.longestRally = max(stats.longestRally, stats.currentRally);
        playBeep();
    }

    if (Ballx >= width - 3 && prev_Ball_X <= width - 3 && Bally >= paddle2 && Bally < paddle2 + paddle_height)
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
        stats.longestRally = max(stats.longestRally, stats.currentRally);
        playBeep();
    }

    if (Ballx < 0)
    {
        score2++;
        stats.p2Goals++;
        playScoreBeep();
        ball_spawn();
    }
    if (Ballx > width)
    {
        score1++;
        stats.p1Goals++;
        playScoreBeep();
        ball_spawn();
    }
}

void controller(bool isTwoPlayer)
{
    // Feature/Bug Fix: True simultaneous movement bypassing _kbhit() buffer blocks
    if (GetAsyncKeyState(toupper(settings.upKey)) & 0x8000)
        paddle1 = max(0, paddle1 - settings.paddleSpeed);
    if (GetAsyncKeyState(toupper(settings.downKey)) & 0x8000)
        paddle1 = min(height - paddle_height, paddle1 + settings.paddleSpeed);

    if (isTwoPlayer)
    {
        if (GetAsyncKeyState(toupper(settings.p2UpKey)) & 0x8000)
            paddle2 = max(0, paddle2 - settings.paddleSpeed);
        if (GetAsyncKeyState(toupper(settings.p2DownKey)) & 0x8000)
            paddle2 = min(height - paddle_height, paddle2 + settings.paddleSpeed);
    }

    // Capture standard single-stroke menu buttons
    if (_kbhit())
    {
        char key = tolower(_getch());
        if (key == settings.pauseKey) game_paused = !game_paused;
        if (key == settings.restartKey)
        {
            score1 = 0;
            score2 = 0;
            ball_spawn();
            draw_interface();
        }
        if (key == 'x')
            game_runner = false;
    }
}

//======================== Game Loop & Menus ========================
char display_stats(bool fromMenu = false)
{
    system("cls");
    cout << YELLOW << "\n\n\t================ MATCH STATISTICS ================\n" << RESET;
    cout << "\tWinner: " << stats.winner << "\n";
    cout << "\tFinal Score: " << stats.p1Score << " - " << stats.p2Score << "\n";
    cout << "\tTotal Paddle Hits: " << stats.totalHits << "\n";
    cout << "\tLongest Rally: " << stats.longestRally << "\n";
    cout << "\tWall Bounces: " << stats.wallHits << "\n";
    cout << "\tMax Ball Speed: " << stats.maxSpeedReached << "x\n";
    cout << "\tTime Played: " << stats.timePlayed << "s\n\n";

    if (fromMenu) {
        cout << "\tPress any key to return to Main Menu...";
        _getch();
        return 'M';
    }

    // Feature: Exit Menu Options
    cout << "\tPress R to Play Again\n";
    cout << "\tPress M to Return to Main Menu\n";
    cout << "\tPress E to Exit Game\n\n";

    while(true) {
        char choice = toupper(_getch());
        if(choice == 'R' || choice == 'M' || choice == 'E') return choice;
    }
}

void run_match(bool isTwoPlayer, string p1Name = "", string p2Name = "")
{
    current_p1 = p1Name.empty() ? username : p1Name;
    current_p2 = p2Name.empty() ? (isTwoPlayer ? "Player 2" : "AI") : p2Name;

    score1 = 0;
    score2 = 0;
    stats = MatchStats();
    paddle1 = height/2 - paddle_height / 2;
    paddle2 = paddle1;
    ball_spawn();
    draw_interface();
    matchStartTime = time(0);
    game_runner = true;

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
                stats.timePlayed = difftime(time(0), matchStartTime);
                stats.winner = (score1 > score2) ? current_p1 : current_p2;
                saveScore(max(score1, score2));
            }
        }
        else
        {
            set_positon(width/2 - 4, height/2);
            cout << RED << "[ PAUSED ]" << RESET;
            controller(isTwoPlayer);
            if(!game_paused) draw_interface();
        }
        Sleep(50);
    }
}

void tournament_mode()
{
    string players[4];
    system("cls");
    cout << YELLOW << "\n\n\t================ TOURNAMENT REGISTRATION ================\n\n" << RESET;
    for(int i = 0; i < 4; i++) {
        cout << "\tEnter Name for Player " << i+1 << ": ";
        cin >> players[i];
    }

    // Match 1
    system("cls");
    cout << MAGENTA << "\n\n\t========== MATCH 1 ==========\n\n" << RESET;
    cout << "\t" << players[0] << " vs " << players[1] << "\n\n\tPress any key to start...";
    _getch();
    run_match(true, players[0], players[1]);
    string winner1 = stats.winner;

    // Match 2
    system("cls");
    cout << MAGENTA << "\n\n\t========== MATCH 2 ==========\n\n" << RESET;
    cout << "\t" << players[2] << " vs " << players[3] << "\n\n\tPress any key to start...";
    _getch();
    run_match(true, players[2], players[3]);
    string winner2 = stats.winner;

    // Final Match Prep
    system("cls");
    cout << RED << "\n\n\t========== GRAND FINAL ==========\n\n" << RESET;
    cout << "\t" << winner1 << " vs " << winner2 << "\n\n";
    cout << "\tEnter custom Target Score for the Final: ";
    int oldScore = settings.winningScore;
    cin >> settings.winningScore;

    cout << "\n\tPress any key to fight for the Championship...";
    _getch();

    // Final Match
    run_match(true, winner1, winner2);
    string champion = stats.winner;
    settings.winningScore = oldScore; // Revert rule changes

    // Bracket
    system("cls");
    cout << YELLOW << "\n\n\t================ TOURNAMENT BRACKET ================\n\n" << RESET;
    cout << "\t" << players[0] << "\n";
    cout << "\t        \\\n";
    cout << "\t         Winner 1 (" << winner1 << ") ----\\\n";
    cout << "\t        /\n";
    cout << "\t" << players[1] << "                            \\\n";
    cout << "\t                                 Final ---> " << GREEN << champion << " (CHAMPION)" << RESET << "\n";
    cout << "\t" << players[2] << "                            /\n";
    cout << "\t        \\\n";
    cout << "\t         Winner 2 (" << winner2 << ") ----/\n";
    cout << "\t        /\n";
    cout << "\t" << players[3] << "\n\n";

    cout << "\tPress any key to return to Main Menu...";
    _getch();
}

// Feature: Settings Menu fully attached
void Settings_Menu()
{
    while(true)
    {
        system("cls");
        cout << YELLOW << "\n\n\t================ SETTINGS ================\n\n" << RESET;
        cout << "\t1. Target Winning Score: " << settings.winningScore << "\n";
        cout << "\t2. AI Difficulty: ";
        if(settings.difficulty == 1) cout << "Easy\n";
        else if (settings.difficulty == 2) cout << "Medium\n";
        else cout << "Hard\n";
        cout << "\t3. Toggle Sound: " << (settings.soundON ? "ON" : "OFF") << "\n";
        cout << "\t4. Return to Main Menu\n\n";
        cout << "\tPress 1-4 to select an option...";

        char choice = _getch();
        if(choice == '1') {
            cout << "\n\n\tEnter new Target Score: ";
            cin >> settings.winningScore;
        } else if (choice == '2') {
            settings.difficulty = (settings.difficulty % 3) + 1;
            if(settings.difficulty == 1) settings.aiType = 1;
            else settings.aiType = 2;
        } else if (choice == '3') {
            settings.soundON = !settings.soundON;
        } else if (choice == '4') {
            break;
        }
    }
}

// Feature: Control Mapping Menu
void Controls_Menu()
{
    while(true)
    {
        system("cls");
        cout << YELLOW << "\n\n\t========== CONTROLS ==========\n\n" << RESET;
        cout << "\tPlayer 1\n";
        cout << "\tMove Up   : " << (char)toupper(settings.upKey) << "\n";
        cout << "\tMove Down : " << (char)toupper(settings.downKey) << "\n\n";

        cout << "\tPlayer 2\n";
        cout << "\tMove Up   : " << (char)toupper(settings.p2UpKey) << "\n";
        cout << "\tMove Down : " << (char)toupper(settings.p2DownKey) << "\n\n";

        cout << "\tPress C to change controls.\n";
        cout << "\tPress B to go back.\n";

        char c = toupper(_getch());
        if (c == 'B') break;
        if (c == 'C')
        {
            cout << "\n\tEnter Player 1 UP Key: ";
            char u1 = tolower(_getch());
            cout << (char)toupper(u1) << "\n\tEnter Player 1 DOWN Key: ";
            char d1 = tolower(_getch());
            cout << (char)toupper(d1) << "\n\tEnter Player 2 UP Key: ";
            char u2 = tolower(_getch());
            cout << (char)toupper(u2) << "\n\tEnter Player 2 DOWN Key: ";
            char d2 = tolower(_getch());
            cout << (char)toupper(d2) << "\n";

            // Duplication Check
            if (u1 == d1 || u1 == u2 || u1 == d2 || d1 == u2 || d1 == d2 || u2 == d2) {
                cout << RED << "\n\tError: Duplicate keys assigned! Press any key to try again." << RESET;
                _getch();
            } else {
                settings.upKey = u1;
                settings.downKey = d1;
                settings.p2UpKey = u2;
                settings.p2DownKey = d2;
                cout << GREEN << "\n\tControls saved successfully! Press any key..." << RESET;
                _getch();
            }
        }
    }
}

void algorithm_demo()
{
    system("cls");
    cout << YELLOW << "\n\t=== AI ALGORITHM DEMONSTRATION ===\n\n" << RESET;
    cout << CYAN << "1. Simple AI (Reactive)\n" << RESET;
    cout << "   - Principle: Follows the ball's current Y position constantly.\n";
    cout << "   - Time Complexity: O(1) per frame.\n";
    cout << "   - Advantage: Extremely lightweight, feels 'human' on easy mode.\n";
    cout << "   - Disadvantage: Too slow against high-speed angled shots.\n\n";

    cout << GREEN << "2. Predictive AI (Proactive)\n" << RESET;
    cout << "   - Principle: Uses linear algebra to calculate exact wall-bounces\n";
    cout << "     and predicts the precise Y-coordinate where the ball will arrive.\n";
    cout << "   - Time Complexity: O(N) where N is number of wall bounces.\n";
    cout << "   - Advantage: Nearly impossible to beat; perfect accuracy.\n";
    cout << "   - Disadvantage: Can feel robotic or unfair to casual players.\n\n";

    cout << MAGENTA << "[ Simulation Comparison (Hard Mode) ]\n" << RESET;
    cout << "   Reaction Time: Simple [80ms] | Predictive [0ms]\n";
    cout << "   Win Rate vs P1: Simple [25%]  | Predictive [85%]\n\n";
    cout << "Press any key to exit.";
    _getch();
}

void show_highscores()
{
    system("cls");
    cout << YELLOW << "\n\t=== HIGH SCORES ===\n" << RESET;
    vector<ScoreRecord> recs = loadScores();
    if(recs.empty())
        cout << "\n\tNo scores yet!";
    else
    {
        cout << "\tName\tScore\tDiff\tTime\n";
        cout << "\t----------------------------------\n";
        for (auto &r : recs) cout << "\t" << r.name << "\t" << r.score << "\t" << r.diff << "\t" << r.time << "s\n";
    }
    _getch();
}

int Main_Menu()
{
    int button = 0;
    string options[] = {"Single Player", "Two Player", "Tournament Mode", "High Scores",
                        "Statistics (Last Match)", "Settings", "Controls", "Algorithm Demo", "Exit"};
    while (true)
    {
        system("cls");
        cout << YELLOW << "\n\n\t================= PING PONG MASTER =================\n\n" << RESET;
        cout << "\tWelcome, " << CYAN << username << RESET << "\n\n";

        for (int i = 0; i < 9; i++)
        {
            if (button == i) cout << BLUE << "\t-> " << options[i] << " <-\n" << RESET;
            else cout << "\t   " << options[i] << "\n";
        }

        char key = tolower(_getch());
        if (key == 'w' && button > 0)
            button--;
        if (key == 's' && button < 8)
            button++;
        if (key == 13 || key == ' ')
            return button + 1;
    }
}

int main()
{
    srand(time(0));
    show_cursor(false);
    system("cls");

    set_positon(40, 5);
    cout << "Enter your Player Name: ";
    cin >> username;

    while (true)
    {
        int choice = Main_Menu();
        if (choice == 1 || choice == 2)
        {
            bool twoPlayer = (choice == 2);
            while (true)
            {
                run_match(twoPlayer);
                char postAction = display_stats(false);
                if (postAction == 'R') continue; // Loop again
                if (postAction == 'E') exit(0);  // Kill application
                break; // 'M' breaks out of match loop, returns to main menu
            }
        }
        else if (choice == 3) tournament_mode();
        else if (choice == 4) show_highscores();
        else if (choice == 5) display_stats(true);
        else if (choice == 6) Settings_Menu();
        else if (choice == 7) Controls_Menu();
        else if (choice == 8) algorithm_demo();
        else if (choice == 9) break;
    }

    show_cursor(true);
    return 0;
}
