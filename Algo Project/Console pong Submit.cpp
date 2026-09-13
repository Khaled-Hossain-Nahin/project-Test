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

const char *RESET   = "\033[0m";
const char *CYAN    = "\033[1;36m";
const char *YELLOW  = "\033[1;33m";
const char *GREEN   = "\033[1;32m";
const char *RED     = "\033[1;31m";
const char *BLUE    = "\033[1;34m";
const char *WHITE   = "\033[1;37m";
const char *MAGENTA = "\033[0;35m";

struct GameSettings
{
    int  difficulty    = 2;
    int  ballSpeed     = 1;
    int  paddleSpeed   = 2;
    int  winningScore  = 5;
    bool soundON       = true;
    int  aiType        = 2;//1 -> simple, 2-> pred

    const char* paddle1Col = CYAN;
    const char* paddle2Col = GREEN;
    const char* ballCol    = RED;
    const char* borderCol  = WHITE;

    char p1Up   = 'w';
    char p1Down = 's';
    char p2Up   = 'i';
    char p2Down = 'k';
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

string p1Name = "Player 1";
string p2Name = "Player 2";


int getMenuKey()
{
    int key = _getch();

    if (key == 224)
    {
        key = _getch();

        if (key == 72) return 1;
        if (key == 80) return 2;
        if (key == 75) return 3;
        if (key == 77) return 4;
    }

    if (key == 13) return 5;
    if (key == 27) return 6;

    return 0;
}

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

bool isValidKey(char c)
{
    return c >= 33 && c <= 126; // any printable non-space
}

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

void ball_spawn()
{
    speedLevel = settings.ballSpeed;
    Ballx = width / 2;
    Bally = height / 2;

    int dirX = (rand() % 2 == 0) ? -1 : 1;
    int dirY = (rand() % 2 == 0) ? -1 : 1;

    Ball_velocity_x = dirX * speedLevel;
    Ball_velocity_y = dirY;

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

    //1 -> simple
    if (settings.aiType == 1)
        targetY = Bally - paddle_height / 2;
    else
        targetY = predictBallY(Ballx, Bally, Ball_velocity_x, Ball_velocity_y, width - 3)
                  - paddle_height / 2;

    if (settings.difficulty == 1 && (rand() % 100 < 30))
        targetY += (rand() % 5 - 2);
    targetY = max(0, min(height - paddle_height, targetY));

    int moveStep = settings.paddleSpeed;
    if (settings.difficulty == 1)
        moveStep = max(1, moveStep - 1);
    if (settings.difficulty == 3)
        moveStep += 1;

    if (paddle2 < targetY)
        paddle2 = min(paddle2 + moveStep, targetY);
    else if (paddle2 > targetY)
        paddle2 = max(paddle2 - moveStep, targetY);
}

void draw_interface()
{
    system("cls");
    cout << settings.borderCol;
    //top part
    for (int i = 0; i < width + 2; i++) cout << "#";
    for (int j = 0; j < height; j++)
    {
        set_position(0, j + 1);
        cout << settings.borderCol << "#";
        set_position(width + 1, j + 1);
        cout << settings.borderCol << "#";
        set_position(width / 2 + 1, j + 1);
        cout << YELLOW << "|";
    }
    set_position(0, height + 1);
    //bottom
    for (int i = 0; i < width + 2; i++) cout <<RESET<< "#";

    set_position(2, height + 3);
    cout << CYAN << p1Name << ": " << score1
         << " | " << p2Name << ": " << score2;
    cout<< YELLOW << " Target: " << settings.winningScore;

    set_position(2, height + 4);
    cout << MAGENTA << "Speed: " << speedLevel << "x | Rally: "
         << stats.currentRally << RESET;
}

void in_game()
{
    if (prev_Ball_y > 0 && prev_Ball_y < height + 1)
    {
        set_position(prev_Ball_X + 1, prev_Ball_y + 1);
        if (prev_Ball_X + 1 == 0 || prev_Ball_X + 1 == width + 1)
            cout << settings.borderCol << "#" << RESET;
        else if (prev_Ball_X == width / 2)
            cout << RESET << "|";
        else
            cout << " ";
    }

    for (int row = 1; row <= height; row++)
    {
        set_position(0, row);
        cout << settings.borderCol << "#";
        set_position(width + 1, row);
        cout << settings.borderCol << "#" << RESET;
    }

    for (int i = 0; i < paddle_height; i++)
    {
        set_position(1, prev_paddle1 + i + 1);
        cout << " ";
        set_position(width - 1, prev_paddle2 + i + 1);
        cout << " ";
    }

    if (Ballx >= 0 && Ballx <= width && Bally >= 0 && Bally < height)
    {
        set_position(Ballx + 1, Bally + 1);
        cout << settings.ballCol << "O" << RESET;
    }

    for (int i = 0; i < paddle_height; i++)
    {
        set_position(1, paddle1 + i + 1);
        cout << settings.paddle1Col << "|";
        set_position(width - 1, paddle2 + i + 1);
        cout << settings.paddle2Col << "|";
    }

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

    if (Ball_velocity_x < 0 &&
            Ballx <= 2 &&
            Bally >= paddle1 &&
            Bally < paddle1 + paddle_height)
    {
        Ballx = 3;
        Ball_velocity_x = abs(Ball_velocity_x);
        int hitPoint = Bally - paddle1;

        if (hitPoint == 0)
            Ball_velocity_y = -1;
        else if (hitPoint == paddle_height - 1)
            Ball_velocity_y = 1;
        if (speedLevel < 5) speedLevel++;
        stats.totalHits++;
        stats.currentRally++;
        stats.maxSpeedReached = max(stats.maxSpeedReached, speedLevel);
        stats.longestRally    = max(stats.longestRally,    stats.currentRally);
        playBeep();
    }

    if (Ball_velocity_x > 0 &&
            Ballx >= width - 3 &&
            Bally >= paddle2 &&
            Bally < paddle2 + paddle_height)
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
    if (Ballx < 0)
    {
        score2++;
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
            cout << RED << "[ Saba balo ]" << RESET;
            controller(isTwoPlayer);
            if (!game_paused) draw_interface();
        }
        Sleep(50);
    }

    if (!showGameOver) return '\0';
    return game_over_screen(stats.winner);
}

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

    p1Name = players[0];
    p2Name = players[1];
    run_match(true, false);
    string winner1 = (score1 > score2) ? players[0] : players[1];

    system("cls");
    cout << GREEN << "\n\tMatch 1 Winner: " << winner1 << RESET << "\n\n";
    cout << "\tPress any key to start Match 2...";
    _getch();

    p1Name = players[2];
    p2Name = players[3];
    run_match(true, false);
    string winner2 = (score1 > score2) ? players[2] : players[3];

    system("cls");
    cout << GREEN << "\n\tMatch 2 Winner: " << winner2 << RESET << "\n\n";

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

    settings.winningScore = savedWinningScore;
    p1Name = "Player 1";
    p2Name = "Player 2";
}
void controls_menu()
{
    int selected = 0;
    const int N = 8;

    while (true)
    {
        system("cls");

        cout << YELLOW
             << "\n\t========== CONTROLS ==========\n\n"
             << RESET;

        cout << CYAN << "\tPlayer 1\n" << RESET;

        if (selected == 0)
            cout << BLUE << "\t-> Move Up   : "
                 << (char)toupper(settings.p1Up) << " <-\n" << RESET;
        else
            cout << "\t   Move Up   : "
                 << (char)toupper(settings.p1Up) << "\n";

        if (selected == 1)
            cout << BLUE << "\t-> Move Down : "
                 << (char)toupper(settings.p1Down) << " <-\n" << RESET;
        else
            cout << "\t   Move Down : "
                 << (char)toupper(settings.p1Down) << "\n";


        cout << "\n" << GREEN << "\tPlayer 2\n" << RESET;

        if (selected == 2)
            cout << BLUE << "\t-> Move Up   : "
                 << (char)toupper(settings.p2Up) << " <-\n" << RESET;
        else
            cout << "\t   Move Up   : "
                 << (char)toupper(settings.p2Up) << "\n";

        if (selected == 3)
            cout << BLUE << "\t-> Move Down : "
                 << (char)toupper(settings.p2Down) << " <-\n" << RESET;
        else
            cout << "\t   Move Down : "
                 << (char)toupper(settings.p2Down) << "\n";


        cout << "\n" << MAGENTA << "\tGeneral\n" << RESET;

        if (selected == 4)
            cout << BLUE << "\t-> Pause     : "
                 << (char)toupper(settings.pauseKey) << " <-\n" << RESET;
        else
            cout << "\t   Pause     : "
                 << (char)toupper(settings.pauseKey) << "\n";

        if (selected == 5)
            cout << BLUE << "\t-> Restart   : "
                 << (char)toupper(settings.restartKey) << " <-\n" << RESET;
        else
            cout << "\t   Restart   : "
                 << (char)toupper(settings.restartKey) << "\n";

        if (selected == 6)
            cout << BLUE << "\t-> Exit Game : "
                 << (char)toupper(settings.exitKey) << " <-\n" << RESET;
        else
            cout << "\t   Exit Game : "
                 << (char)toupper(settings.exitKey) << "\n";


        if (selected == 7)
            cout << BLUE << "\n\t-> Back <-\n" << RESET;
        else
            cout << "\n\t   Back\n";


        cout << "\n\t";
        cout << YELLOW << "↑ / ↓" << RESET << " Navigate    ";
        cout << YELLOW << "ENTER" << RESET << " Change\n";

        cout << "\t";
        cout << YELLOW << "ESC" << RESET << " Back\n";


        int key = getMenuKey();

        if (key == 1)
        {
            selected--;

            if (selected < 0)
                selected = N - 1;
        }

        else if (key == 2)
        {
            selected++;

            if (selected >= N)
                selected = 0;
        }

        else if (key == 5)
        {
            if (selected == 7)
                return;

            char oldKey;

            if (selected == 0)
                oldKey = settings.p1Up;
            else if (selected == 1)
                oldKey = settings.p1Down;
            else if (selected == 2)
                oldKey = settings.p2Up;
            else if (selected == 3)
                oldKey = settings.p2Down;
            else if (selected == 4)
                oldKey = settings.pauseKey;
            else if (selected == 5)
                oldKey = settings.restartKey;
            else
                oldKey = settings.exitKey;


            system("cls");

            cout << YELLOW
                 << "\n\t========== CHANGE CONTROL ==========\n\n"
                 << RESET;

            cout << "\tSelected: ";

            if (selected == 0)
                cout << "Player 1 Move Up\n";
            else if (selected == 1)
                cout << "Player 1 Move Down\n";
            else if (selected == 2)
                cout << "Player 2 Move Up\n";
            else if (selected == 3)
                cout << "Player 2 Move Down\n";
            else if (selected == 4)
                cout << "Pause\n";
            else if (selected == 5)
                cout << "Restart\n";
            else
                cout << "Exit Game\n";

            cout << "\n\tCurrent Key: "
                 << (char)toupper(oldKey);

            cout << "\n\n\tPress a new key.";
            cout << "\n\tPress ESC to cancel.\n";


            while (true)
            {
                int k = _getch();

                if (k == 27)
                    break;

                if (k == 224)
                {
                    _getch();
                    continue;
                }

                k = tolower(k);

                if (!isValidKey((char)k))
                    continue;


                char newKey = (char)k;

                bool duplicate = false;

                if (selected != 0 && newKey == settings.p1Up)
                    duplicate = true;

                if (selected != 1 && newKey == settings.p1Down)
                    duplicate = true;

                if (selected != 2 && newKey == settings.p2Up)
                    duplicate = true;

                if (selected != 3 && newKey == settings.p2Down)
                    duplicate = true;

                if (selected != 4 && newKey == settings.pauseKey)
                    duplicate = true;

                if (selected != 5 && newKey == settings.restartKey)
                    duplicate = true;

                if (selected != 6 && newKey == settings.exitKey)
                    duplicate = true;


                if (duplicate)
                {
                    cout << RED
                         << "\n\tThat key is already being used!"
                         << RESET;

                    Sleep(1000);
                    break;
                }


                if (selected == 0)
                    settings.p1Up = newKey;
                else if (selected == 1)
                    settings.p1Down = newKey;
                else if (selected == 2)
                    settings.p2Up = newKey;
                else if (selected == 3)
                    settings.p2Down = newKey;
                else if (selected == 4)
                    settings.pauseKey = newKey;
                else if (selected == 5)
                    settings.restartKey = newKey;
                else if (selected == 6)
                    settings.exitKey = newKey;


                cout << GREEN
                     << "\n\tControl changed successfully!"
                     << RESET;

                Sleep(800);

                break;
            }
        }

        else if (key == 6)
        {
            return;
        }
    }
}
void settings_menu()
{
    int selected = 0;


    const int N = 7;

    while (true)
    {
        system("cls");

        cout << YELLOW
             << "\n\t========== SETTINGS ==========\n\n"
             << RESET;

        if (selected == 0)
            cout << BLUE << "\t-> Difficulty     : "
                 << (settings.difficulty == 1 ? "Easy" :
                     settings.difficulty == 2 ? "Medium" : "Hard")
                 << " <-\n" << RESET;
        else
            cout << "\t   Difficulty     : "
                 << (settings.difficulty == 1 ? "Easy" :
                     settings.difficulty == 2 ? "Medium" : "Hard")
                 << "\n";

        if (selected == 1)
            cout << BLUE << "\t-> Ball Speed     : "
                 << settings.ballSpeed << " <-\n" << RESET;
        else
            cout << "\t   Ball Speed     : "
                 << settings.ballSpeed << "\n";

        if (selected == 2)
            cout << BLUE << "\t-> Paddle Speed   : "
                 << settings.paddleSpeed << " <-\n" << RESET;
        else
            cout << "\t   Paddle Speed   : "
                 << settings.paddleSpeed << "\n";

        if (selected == 3)
            cout << BLUE << "\t-> Winning Score  : "
                 << settings.winningScore << " <-\n" << RESET;
        else
            cout << "\t   Winning Score  : "
                 << settings.winningScore << "\n";

        if (selected == 4)
            cout << BLUE << "\t-> Sound          : "
                 << (settings.soundON ? "ON" : "OFF")
                 << " <-\n" << RESET;
        else
            cout << "\t   Sound          : "
                 << (settings.soundON ? "ON" : "OFF")
                 << "\n";

        if (selected == 5)
            cout << BLUE << "\t-> AI Type        : "
                 << (settings.aiType == 1 ? "Simple" : "Predictive")
                 << " <-\n" << RESET;
        else
            cout << "\t   AI Type        : "
                 << (settings.aiType == 1 ? "Simple" : "Predictive")
                 << "\n";

        if (selected == 6)
            cout << BLUE << "\n\t-> Back <-\n" << RESET;
        else
            cout << "\n\t   Back\n";

        cout << "\n\t";
        cout << YELLOW << "↑ / ↓" << RESET << " Navigate    ";
        cout << YELLOW << "← / →" << RESET << " Change\n";

        cout << "\t";
        cout << YELLOW << "ENTER" << RESET << " Select    ";
        cout << YELLOW << "ESC" << RESET << " Back\n";

        int key = getMenuKey();

        if (key == 1)
        {
            selected--;

            if (selected < 0)
                selected = N - 1;
        }

        else if (key == 2)
        {
            selected++;

            if (selected >= N)
                selected = 0;
        }

        else if (key == 3)
        {
            if (selected == 0)
            {
                settings.difficulty--;

                if (settings.difficulty < 1)
                    settings.difficulty = 3;
            }

            else if (selected == 1)
            {
                settings.ballSpeed--;

                if (settings.ballSpeed < 1)
                    settings.ballSpeed = 3;
            }

            else if (selected == 2)
            {
                settings.paddleSpeed--;

                if (settings.paddleSpeed < 1)
                    settings.paddleSpeed = 4;
            }

            else if (selected == 3)
            {
                settings.winningScore--;

                if (settings.winningScore < 1)
                    settings.winningScore = 20;
            }

            else if (selected == 4)
            {
                settings.soundON = !settings.soundON;
            }

            else if (selected == 5)
            {
                settings.aiType--;

                if (settings.aiType < 1)
                    settings.aiType = 2;
            }
        }

        else if (key == 4)
        {
            if (selected == 0)
            {
                settings.difficulty++;

                if (settings.difficulty > 3)
                    settings.difficulty = 1;
            }

            else if (selected == 1)
            {
                settings.ballSpeed++;

                if (settings.ballSpeed > 3)
                    settings.ballSpeed = 1;
            }

            else if (selected == 2)
            {
                settings.paddleSpeed++;

                if (settings.paddleSpeed > 4)
                    settings.paddleSpeed = 1;
            }

            else if (selected == 3)
            {
                settings.winningScore++;

                if (settings.winningScore > 20)
                    settings.winningScore = 1;
            }

            else if (selected == 4)
            {
                settings.soundON = !settings.soundON;
            }

            else if (selected == 5)
            {
                settings.aiType++;

                if (settings.aiType > 2)
                    settings.aiType = 1;
            }
        }

        else if (key == 5)
        {
            if (selected == 6)
                return;

            if (selected == 0)
            {
                settings.difficulty++;

                if (settings.difficulty > 3)
                    settings.difficulty = 1;
            }

            else if (selected == 1)
            {
                settings.ballSpeed++;

                if (settings.ballSpeed > 3)
                    settings.ballSpeed = 1;
            }

            else if (selected == 2)
            {
                settings.paddleSpeed++;

                if (settings.paddleSpeed > 4)
                    settings.paddleSpeed = 1;
            }

            else if (selected == 3)
            {
                settings.winningScore++;

                if (settings.winningScore > 20)
                    settings.winningScore = 1;
            }

            else if (selected == 4)
            {
                settings.soundON = !settings.soundON;
            }

            else if (selected == 5)
            {
                settings.aiType++;

                if (settings.aiType > 2)
                    settings.aiType = 1;
            }
        }

        else if (key == 6)
        {
            return;
        }
    }
}
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

        cout << YELLOW
             << "\n\n\t================= PING PONG MASTER =================\n\n"
             << RESET;

        cout << "\tWelcome, " << CYAN << username << RESET << "\n\n";

        for (int i = 0; i < N; i++)
        {
            if (button == i)
            {
                cout << BLUE << "\t-> " << options[i] << " <-\n" << RESET;
            }
            else
            {
                cout << "\t   " << options[i] << "\n";
            }
        }

        cout << "\n\tUse ";
        cout << YELLOW << "↑ / ↓" << RESET;
        cout << " to navigate, ";
        cout << YELLOW << "ENTER" << RESET;
        cout << " to select.\n";

        cout << "\tPress ";
        cout << YELLOW << "ESC" << RESET;
        cout << " to exit.\n";

        int key = getMenuKey();

        if (key == 1)
        {
            button--;

            if (button < 0)
                button = N - 1;
        }

        else if (key == 2)
        {
            button++;

            if (button >= N)
                button = 0;
        }

        else if (key == 5)
        {
            return button + 1;
        }

        else if (key == 6)
        {
            return N; // Exit
        }
    }
}
int main()
{
    srand((unsigned)time(0));

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    show_cursor(false);
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

        if (choice == 1)
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
        else if (choice == 2)
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
        else if (choice == 5) settings_menu();
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
