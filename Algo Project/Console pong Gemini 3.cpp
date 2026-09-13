#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <stack>
#include <queue>

using namespace std;

// --- ANSI Colors ---
const char* RESET   = "\033[0m";
const char* CYAN    = "\033[1;36m";
const char* YELLOW  = "\033[1;33m";
const char* GREEN   = "\033[1;32m";
const char* RED     = "\033[1;31m";
const char* BLUE    = "\033[1;34m";
const char* WHITE   = "\033[1;37m";
const char* MAGENTA = "\033[0;35m";

// --- Enums & Structs ---
enum GameMode { PVP, PVAI, AIVAI };
enum MenuState { MAIN, PLAY_SELECT, TOURNAMENT, SETTINGS, CONTROLS, STATS, HIGH_SCORES, EXIT_MENU };
enum PowerUpType { NONE, PADDLE_EXPAND, PADDLE_SHRINK, SPEED_BOOST, SLOW_BALL };

struct GameSettings {
    int difficulty   = 2; // 1: Easy, 2: Medium, 3: Hard
    int baseSpeed    = 1;
    int paddleSpeed  = 2;
    int winningScore = 5;
    bool soundON     = true;
    bool powerUpsON  = true;

    char p1Up = 'w', p1Down = 's';
    char p2Up = 'i', p2Down = 'k';
    char pauseKey = 'p', exitKey = 'x';
};

struct MatchStats {
    string winner;
    int p1Score = 0, p2Score = 0;
    int totalHits = 0, longestRally = 0, currentRally = 0;
    int timePlayed = 0;
    time_t startTime;
};

// --- Core Data Structures Implemented ---

// 1. Linked List: To keep a running log of match history during the session
struct HistoryNode {
    string matchDetails;
    HistoryNode* next;
    HistoryNode(string details) : matchDetails(details), next(nullptr) {}
};

class SessionHistory {
private:
    HistoryNode* head;
public:
    SessionHistory() : head(nullptr) {}
    void addMatch(string details) {
        HistoryNode* newNode = new HistoryNode(details);
        newNode->next = head;
        head = newNode;
    }
    void displayHistory() {
        HistoryNode* temp = head;
        if (!temp) cout << "\tNo matches played in this session yet.\n";
        while (temp) {
            cout << "\t- " << temp->matchDetails << "\n";
            temp = temp->next;
        }
    }
    ~SessionHistory() {
        while (head) {
            HistoryNode* temp = head;
            head = head->next;
            delete temp;
        }
    }
};

// 2. Binary Search Tree (BST): To manage and sort high scores efficiently
struct ScoreNode {
    string name;
    int score;
    ScoreNode* left;
    ScoreNode* right;
    ScoreNode(string n, int s) : name(n), score(s), left(nullptr), right(nullptr) {}
};

class ScoreBST {
private:
    ScoreNode* root;
    void insertNode(ScoreNode*& node, string name, int score) {
        if (!node) { node = new ScoreNode(name, score); return; }
        if (score > node->score) insertNode(node->left, name, score); // Higher scores go left
        else insertNode(node->right, name, score);
    }
    void inorder(ScoreNode* node, vector<pair<string, int>>& sortedScores, int& count) {
        if (!node || count >= 10) return;
        inorder(node->left, sortedScores, count);
        if (count < 10) { sortedScores.push_back({node->name, node->score}); count++; }
        inorder(node->right, sortedScores, count);
    }
    void saveToFile(ScoreNode* node, ofstream& out) {
        if (!node) return;
        saveToFile(node->left, out);
        out << node->name << " " << node->score << "\n";
        saveToFile(node->right, out);
    }
    void destroyTree(ScoreNode* node) {
        if (node) { destroyTree(node->left); destroyTree(node->right); delete node; }
    }
public:
    ScoreBST() : root(nullptr) {}
    ~ScoreBST() { destroyTree(root); }
    void insert(string name, int score) { insertNode(root, name, score); }
    vector<pair<string, int>> getTop10() {
        vector<pair<string, int>> topScores;
        int count = 0;
        inorder(root, topScores, count);
        return topScores;
    }
    void saveAll(const string& filename) {
        ofstream out(filename);
        saveToFile(root, out);
    }
    void loadAll(const string& filename) {
        ifstream in(filename);
        string name; int score;
        while (in >> name >> score) insert(name, score);
    }
};

// --- Game Engine ---
class PingPongGame {
private:
    int width = 80, height = 20;
    GameSettings settings;
    MatchStats stats;
    SessionHistory history;
    ScoreBST highScores;
    stack<MenuState> menuNav; // 3. Stack: Manage UI navigation

    // Entities
    int ballX, ballY, ballVX, ballVY;
    int prevBX, prevBY;
    int speedModifier = 0;

    int p1Y, p2Y;
    int prevP1Y, prevP2Y;
    int p1Height = 5, p2Height = 5;
    string p1Name, p2Name;

    // PowerUp
    struct PowerUp {
        int x, y;
        bool active = false;
        PowerUpType type = NONE;
        int despawnTimer = 0;
    } activePower;

    // Console Helpers
    void setPos(int x, int y) {
        COORD axis = { (SHORT)x, (SHORT)y };
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), axis);
    }
    void showCursor(bool visible) {
        CONSOLE_CURSOR_INFO ci;
        GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
        ci.bVisible = visible;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    }
    void playBeep(int freq, int dur) { if (settings.soundON) Beep(freq, dur); }

    int getMenuKey() {
        int key = _getch();
        if (key == 224) {
            key = _getch();
            if (key == 72) return 1; // Up
            if (key == 80) return 2; // Down
        }
        if (key == 13) return 5; // Enter
        if (key == 27) return 6; // Esc
        return 0;
    }

    void spawnBall() {
        speedModifier = 0;
        ballX = width / 2; ballY = height / 2;
        ballVX = (rand() % 2 == 0 ? -1 : 1) * settings.baseSpeed;
        ballVY = (rand() % 2 == 0 ? -1 : 1);
        p1Height = 5; p2Height = 5; // Reset paddle sizes
        activePower.active = false;
    }

    void spawnPowerUp() {
        if (!settings.powerUpsON || activePower.active || rand() % 100 > 5) return;
        activePower.x = width / 4 + rand() % (width / 2);
        activePower.y = 1 + rand() % (height - 2);
        activePower.type = static_cast<PowerUpType>(1 + rand() % 4);
        activePower.active = true;
        activePower.despawnTimer = 100; // Lasts ~5 seconds
    }

    void applyPowerUp(int playerHit) {
        playBeep(1000, 100);
        activePower.active = false;

        switch (activePower.type) {
            case PADDLE_EXPAND:
                if (playerHit == 1) p1Height = min(9, p1Height + 2);
                else p2Height = min(9, p2Height + 2);
                break;
            case PADDLE_SHRINK:
                if (playerHit == 1) p2Height = max(3, p2Height - 2);
                else p1Height = max(3, p1Height - 2);
                break;
            case SPEED_BOOST:
                speedModifier += 1;
                break;
            case SLOW_BALL:
                speedModifier = max(0, speedModifier - 1);
                break;
            default: break;
        }
    }

    int predictBallY() {
        int x = ballX, y = ballY, vy = ballVY;
        while (x < width - 3) {
            x += abs(ballVX) + speedModifier;
            y += vy;
            if (y <= 0 || y >= height - 1) vy = -vy;
        }
        return y;
    }

    void aiLogic(int& pY, int pHeight, int difficulty, bool isLeft) {
        int targetY = height / 2;

        if ((isLeft && ballVX < 0) || (!isLeft && ballVX > 0)) {
            if (difficulty == 1) targetY = ballY - pHeight / 2; // Simple follow
            else targetY = predictBallY() - pHeight / 2; // Predictive

            // Add human-like error
            if (difficulty == 1 && rand() % 100 < 40) targetY += (rand() % 5 - 2);
            else if (difficulty == 2 && rand() % 100 < 15) targetY += (rand() % 3 - 1);
        }

        targetY = max(0, min(height - pHeight, targetY));

        int moveStep = settings.paddleSpeed + (difficulty == 3 ? 1 : 0);
        if (pY < targetY) pY = min(pY + moveStep, targetY);
        else if (pY > targetY) pY = max(pY - moveStep, targetY);
    }

    void drawArena() {
        system("cls");
        cout << WHITE;
        for (int i = 0; i <= width + 1; i++) cout << "═"; cout << "\n";
        for (int j = 0; j < height; j++) {
            cout << "║";
            setPos(width + 1, j + 1); cout << "║";
            setPos(width / 2, j + 1); cout << YELLOW << "¦" << WHITE;
        }
        setPos(0, height + 1);
        for (int i = 0; i <= width + 1; i++) cout << "═"; cout << RESET;
        drawScoreboard();
    }

    void drawScoreboard() {
        setPos(2, height + 3);
        cout << CYAN << p1Name << ": " << stats.p1Score << "   "
             << YELLOW << "TARGET: " << settings.winningScore << "   "
             << GREEN << p2Name << ": " << stats.p2Score << "      " << RESET;
    }

    void drawEntities() {
        // Erase old positions
        setPos(prevBX + 1, prevBY + 1);
        if (prevBX == width / 2 - 1) cout << YELLOW << "¦" << RESET; else cout << " ";

        for (int i = 0; i < 9; i++) { // Max paddle height is 9
            setPos(1, prevP1Y + i + 1); cout << " ";
            setPos(width - 1, prevP2Y + i + 1); cout << " ";
        }

        // Draw PowerUp
        if (activePower.active) {
            setPos(activePower.x + 1, activePower.y + 1);
            char icon = (activePower.type == SPEED_BOOST) ? '»' :
                        (activePower.type == SLOW_BALL) ? '~' :
                        (activePower.type == PADDLE_EXPAND) ? '+' : '-';
            cout << MAGENTA << icon << RESET;
        } else if (activePower.despawnTimer == 0) {
            setPos(activePower.x + 1, activePower.y + 1); cout << " "; // Erase
        }

        // Draw new positions
        for (int i = 0; i < p1Height; i++) { setPos(1, p1Y + i + 1); cout << CYAN << "█" << RESET; }
        for (int i = 0; i < p2Height; i++) { setPos(width - 1, p2Y + i + 1); cout << GREEN << "█" << RESET; }

        setPos(ballX + 1, ballY + 1);
        cout << RED << "O" << RESET;

        prevBX = ballX; prevBY = ballY;
        prevP1Y = p1Y; prevP2Y = p2Y;
    }

    void countdown() {
        drawArena();
        for(int i = 3; i > 0; i--) {
            setPos(width / 2 - 5, height / 2);
            cout << YELLOW << "STARTS IN: " << i << RESET;
            playBeep(600, 200);
            Sleep(800);
        }
        setPos(width / 2 - 5, height / 2); cout << GREEN << "   GO!     " << RESET;
        playBeep(1000, 400);
        Sleep(500);
        setPos(width / 2 - 5, height / 2); cout << "           "; // Clear
    }

    void handleCollisions() {
        // Wall collisions
        if (ballY <= 0 || ballY >= height - 1) {
            ballVY = -ballVY;
            ballY = (ballY <= 0) ? 1 : height - 2;
            playBeep(400, 50);
        }

        // PowerUp collision
        if (activePower.active && ballX == activePower.x && ballY == activePower.y) {
            int lastHit = (ballVX > 0) ? 1 : 2;
            applyPowerUp(lastHit);
        }

        // Paddle 1
        if (ballVX < 0 && ballX <= 2 && ballY >= p1Y && ballY < p1Y + p1Height) {
            ballX = 3;
            ballVX = -ballVX;
            int hitPos = ballY - p1Y;
            if (hitPos <= p1Height / 3) ballVY = -1;
            else if (hitPos >= (p1Height*2)/3) ballVY = 1;
            else ballVY = 0;

            stats.totalHits++; stats.currentRally++;
            if (stats.currentRally > stats.longestRally) stats.longestRally = stats.currentRally;
            if (stats.currentRally % 4 == 0) speedModifier++;
            playBeep(600, 50);
        }

        // Paddle 2
        if (ballVX > 0 && ballX >= width - 3 && ballY >= p2Y && ballY < p2Y + p2Height) {
            ballX = width - 4;
            ballVX = -ballVX;
            int hitPos = ballY - p2Y;
            if (hitPos <= p2Height / 3) ballVY = -1;
            else if (hitPos >= (p2Height*2)/3) ballVY = 1;
            else ballVY = 0;

            stats.totalHits++; stats.currentRally++;
            if (stats.currentRally > stats.longestRally) stats.longestRally = stats.currentRally;
            if (stats.currentRally % 4 == 0) speedModifier++;
            playBeep(600, 50);
        }
    }

    void runMatch(GameMode mode) {
        stats = MatchStats();
        stats.startTime = time(0);
        p1Y = height / 2 - p1Height / 2;
        p2Y = p1Y;
        prevP1Y = p1Y; prevP2Y = p2Y;

        spawnBall();
        countdown();

        bool running = true;
        bool paused = false;

        while (running) {
            if (!paused) {
                // Movement
                ballX += (ballVX < 0) ? (ballVX - speedModifier) : (ballVX + speedModifier);
                ballY += ballVY;

                if (activePower.active) activePower.despawnTimer--;

                handleCollisions();

                // Scoring
                if (ballX < 0 || ballX > width) {
                    if (ballX < 0) { stats.p2Score++; p1Height = 5; p2Height = 5; } // Reset sizes on score
                    else { stats.p1Score++; p1Height = 5; p2Height = 5; }

                    stats.currentRally = 0;
                    playBeep(800, 300);
                    drawScoreboard();

                    if (stats.p1Score >= settings.winningScore || stats.p2Score >= settings.winningScore) {
                        running = false;
                        continue;
                    }
                    spawnBall();
                    Sleep(500);
                }

                // AI
                if (mode == PVAI || mode == AIVAI) aiLogic(p2Y, p2Height, settings.difficulty, false);
                if (mode == AIVAI) aiLogic(p1Y, p1Height, settings.difficulty, true);

                spawnPowerUp();
                drawEntities();
            }

            // Input
            if (_kbhit()) {
                char key = tolower(_getch());
                if (key == settings.pauseKey) {
                    paused = !paused;
                    setPos(width/2 - 3, height/2);
                    if(paused) cout<<"PAUSED"<<RESET;
                    else cout<<"       "<<RESET;
                }
                if (key == settings.exitKey) running = false;

                if (!paused && mode != AIVAI) {
                    if (key == settings.p1Up) p1Y = max(0, p1Y - settings.paddleSpeed);
                    if (key == settings.p1Down) p1Y = min(height - p1Height, p1Y + settings.paddleSpeed);

                    if (mode == PVP) {
                        if (key == settings.p2Up) p2Y = max(0, p2Y - settings.paddleSpeed);
                        if (key == settings.p2Down) p2Y = min(height - p2Height, p2Y + settings.paddleSpeed);
                    }
                }
            }
            Sleep(50);
        }

        stats.timePlayed = difftime(time(0), stats.startTime);
        stats.winner = (stats.p1Score > stats.p2Score) ? p1Name : p2Name;

        if (stats.p1Score >= settings.winningScore || stats.p2Score >= settings.winningScore) {
            highScores.insert(stats.winner, max(stats.p1Score, stats.p2Score) * 100 + stats.longestRally * 10);
            highScores.saveAll("highscores.txt");
            string matchLog = stats.winner + " beat " + (stats.winner == p1Name ? p2Name : p1Name) +
                              " (" + to_string(stats.p1Score) + "-" + to_string(stats.p2Score) + ")";
            history.addMatch(matchLog);
            showGameOver();
        }
    }

    void showGameOver() {
        system("cls");
        cout << YELLOW << "\n\n\t================= MATCH OVER =================\n\n" << RESET;
        cout << "\t" << GREEN  << "Winner: " << stats.winner << RESET << "\n";
        cout << "\t" << CYAN   << "Final Score: " << p1Name << " " << stats.p1Score
             << " - " << stats.p2Score << " " << p2Name << RESET << "\n\n";
        cout << "\tTotal Hits   : " << stats.totalHits    << "\n";
        cout << "\tLongest Rally: " << stats.longestRally << "\n";
        cout << "\tTime Played  : " << stats.timePlayed   << "s\n\n";
        cout << "\tPress any key to return to menu...\n";
        _getch();
    }

    void runTournament() {
        system("cls");
        cout << YELLOW << "\n\t============ TOURNAMENT MODE ============\n\n" << RESET;

        // 4. Queue: Used for Tournament logic
        queue<string> tQueue;
        for (int i = 1; i <= 4; i++) {
            string name;
            cout << "\tEnter name for Player " << i << ": ";
            cin >> name;
            tQueue.push(name);
        }

        int matchNum = 1;
        while (tQueue.size() > 1) {
            p1Name = tQueue.front(); tQueue.pop();
            p2Name = tQueue.front(); tQueue.pop();

            system("cls");
            cout << YELLOW << "\n\tMatch " << matchNum << ": " << p1Name << " VS " << p2Name << "\n" << RESET;
            cout << "\tPress any key to start..."; _getch();

            runMatch(PVP);
            tQueue.push(stats.winner);
            matchNum++;
        }

        system("cls");
        cout << YELLOW << "\n\t======================================\n";
        cout << "\t     TOURNAMENT CHAMPION: " << GREEN << tQueue.front() << YELLOW << "\n";
        cout << "\t======================================\n\n" << RESET;
        cout << "\tPress any key to return..."; _getch();
    }

public:
    PingPongGame() {
        highScores.loadAll("highscores.txt");
        showCursor(false);
    }

    void start() {
        p1Name = "Player 1";
        menuNav.push(MAIN);

        int selection = 0;
        string mainOpts[] = {"Play Game", "Tournament", "Settings", "Controls", "Match Stats & History", "High Scores", "Exit"};
        string playOpts[] = {"Player vs Player", "Player vs AI", "AI vs AI", "Back"};

        while (!menuNav.empty()) {
            system("cls");
            MenuState current = menuNav.top();

            if (current == MAIN) {
                cout << YELLOW << "\n\n\t================= ARCADE PING PONG =================\n\n" << RESET;
                for (int i = 0; i < 7; i++) {
                    if (selection == i) cout << BLUE << "\t-> " << mainOpts[i] << " <-\n" << RESET;
                    else cout << "\t   " << mainOpts[i] << "\n";
                }
            } else if (current == PLAY_SELECT) {
                cout << YELLOW << "\n\n\t================= SELECT MODE =================\n\n" << RESET;
                for (int i = 0; i < 4; i++) {
                    if (selection == i) cout << BLUE << "\t-> " << playOpts[i] << " <-\n" << RESET;
                    else cout << "\t   " << playOpts[i] << "\n";
                }
            } else if (current == STATS) {
                cout << YELLOW << "\n\t========= SESSION HISTORY =========\n\n" << RESET;
                history.displayHistory();
                cout << "\n\tPress any key to return..."; _getch();
                menuNav.pop(); continue;
            } else if (current == HIGH_SCORES) {
                cout << YELLOW << "\n\t========= HIGH SCORES (BST) =========\n\n" << RESET;
                vector<pair<string, int>> tops = highScores.getTop10();
                if (tops.empty()) cout << "\tNo scores yet!\n";
                for (size_t i = 0; i < tops.size(); i++) cout << "\t" << i+1 << ". " << tops[i].first << " - " << tops[i].second << "\n";
                cout << "\n\tPress any key to return..."; _getch();
                menuNav.pop(); continue;
            } else if (current == SETTINGS) {
                cout << YELLOW << "\n\t========= SETTINGS =========\n\n" << RESET;
                cout << "\t1. Difficulty : " << (settings.difficulty == 1 ? "Easy" : settings.difficulty == 2 ? "Medium" : "Hard") << "\n";
                cout << "\t2. Power-Ups  : " << (settings.powerUpsON ? "ON" : "OFF") << "\n";
                cout << "\t3. Win Score  : " << settings.winningScore << "\n";
                cout << "\n\tPress 1-3 to toggle, ESC to return...";
                char k = _getch();
                if (k == '1') settings.difficulty = (settings.difficulty % 3) + 1;
                if (k == '2') settings.powerUpsON = !settings.powerUpsON;
                if (k == '3') settings.winningScore = (settings.winningScore % 15) + 1;
                if (k == 27) menuNav.pop();
                continue;
            } else if (current == CONTROLS) {
                 cout << YELLOW << "\n\t========= CONTROLS =========\n\n" << RESET;
                 cout << "\tPlayer 1: 'W' (Up) / 'S' (Down)\n";
                 cout << "\tPlayer 2: 'I' (Up) / 'K' (Down)\n";
                 cout << "\tGeneral : 'P' (Pause) / 'X' (Exit Game)\n\n";
                 cout << "\tPress any key to return..."; _getch();
                 menuNav.pop(); continue;
            }

            int key = getMenuKey();
            int maxOpts = (current == MAIN) ? 7 : 4;

            if (key == 1) { selection--; if (selection < 0) selection = maxOpts - 1; }
            else if (key == 2) { selection++; if (selection >= maxOpts) selection = 0; }
            else if (key == 6 && current != MAIN) { menuNav.pop(); selection = 0; } // Pop Stack
            else if (key == 5) { // Enter
                if (current == MAIN) {
                    if (selection == 0) { menuNav.push(PLAY_SELECT); selection = 0; }
                    if (selection == 1) runTournament();
                    if (selection == 2) menuNav.push(SETTINGS);
                    if (selection == 3) menuNav.push(CONTROLS);
                    if (selection == 4) menuNav.push(STATS);
                    if (selection == 5) menuNav.push(HIGH_SCORES);
                    if (selection == 6) break;
                } else if (current == PLAY_SELECT) {
                    if (selection == 0) { p2Name = "Player 2"; runMatch(PVP); }
                    if (selection == 1) { p2Name = "AI"; runMatch(PVAI); }
                    if (selection == 2) { p1Name = "AI-1"; p2Name = "AI-2"; runMatch(AIVAI); }
                    if (selection == 3) { menuNav.pop(); selection = 0; }
                }
            }
        }
    }
};

int main() {
    srand((unsigned)time(0));
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    PingPongGame game;
    game.start();

    system("cls");
    cout << "\n\tThanks for playing Arcade Ping Pong!\n\n";
    return 0;
}
