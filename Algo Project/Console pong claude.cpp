#include <bits/stdc++.h>
#include <conio.h>
#include <windows.h>

using namespace std;

//======================== Core Game Settings ========================
int width=80,height=20,paddle_height=4,max_score=5;

int Ballx,Bally;
int Ball_velocity_x,Ball_velocity_y;

//speedLevel replaces the old "initial_Ball_Speed" mechanism.
//(In the original code, initial_Ball_Speed++ on a paddle hit had no visible
// effect, because ball_spawn() always reset it back to 1 before it could be
// used again. speedLevel is applied directly to the velocity, so a hit
// actually speeds the ball up now.)
int speedLevel = 1;
int max_speed_cap = 3;

int score1=0,score2=0;
int paddle1,paddle2;
int prev_Ball_X,prev_Ball_y,prev_paddle1,prev_paddle2;
bool game_runner= true;

//======================== Colors ========================
const char *RESET = "\033[0m";
const char *CYAN = "\033[1;36m";
const char *YELLOW = "\033[1;33m";
const char *GREEN = "\033[1;32m";
const char *RED = "\033[1;31m";
const char *BLUE = "\033[1;34m";
const char *WHITE = "\033[1;37m";
const char *MAGENTA ="\033[0;35m";

struct ColorOption { const char* name; const char* code; };
ColorOption colorPalette[] = {
    {"Red", RED}, {"Green", GREEN}, {"Yellow", YELLOW},
    {"Blue", BLUE}, {"Cyan", CYAN}, {"White", WHITE}, {"Magenta", MAGENTA}
};
int numColors = 7;

//Customizable colors (Add-on 3)
const char* paddleColor1 = CYAN;
const char* paddleColor2 = GREEN;
const char* ballColor    = RED;
const char* borderColor  = WHITE;

//======================== New Settings (Add-ons) ========================
bool aiMode = false;               //1. AI Mode
int  difficulty = 1;               //2. Difficulty: 1=Easy,2=Medium,3=Hard
bool speedIncreaseEnabled = false;  //6. Ball speed increase toggle
int  paddleSpeed = 1;              //7. Paddle speed control

string username = "Guest";         //8. Login

//======================== Cursor Positioning ========================
void set_positon(int x,int y)
{
    COORD axis;
    axis.X= x;
    axis.Y= y;
    SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE),axis);
}

void show_cursor(bool visible)
{
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    ci.bVisible = visible;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

//======================== Ball Spawn ========================
void ball_spawn(int direction)
{
    speedLevel = 1;

    Ballx= width /2;
    Bally=height/2;

    Ball_velocity_x = direction* speedLevel;
    Ball_velocity_y  = 1;
}

//======================== Score / History persistence (Add-on 4 & 8) ========================
struct ScoreRecord { string name; int score; };

vector<ScoreRecord> loadHighScores()
{
    vector<ScoreRecord> records;
    ifstream fin("highscores.txt");
    string name; int score;
    while (fin >> name >> score)
        records.push_back({name, score});
    return records;
}

void saveHighScores(vector<ScoreRecord> &records)
{
    ofstream fout("highscores.txt", ios::trunc);
    for (auto &r : records)
        fout << r.name << " " << r.score << "\n";
}

//Edge case: empty file / brand-new player -> handled naturally since
//loadHighScores() just returns an empty vector, and "found" stays false.
void updateHighScore(const string &name, int score)
{
    vector<ScoreRecord> records = loadHighScores();
    bool found = false;
    for (auto &r : records)
    {
        if (r.name == name)
        {
            found = true;
            if (score > r.score) r.score = score;
            break;
        }
    }
    if (!found) records.push_back({name, score});

    //Edge case: duplicate scores between two different players are kept as
    //separate entries and simply sorted together (stable_sort keeps whoever
    //already had the score first among equals).
    stable_sort(records.begin(), records.end(), [](const ScoreRecord &a, const ScoreRecord &b){
        return a.score > b.score;
    });
    saveHighScores(records);
}

void logHistory(const string &name, int s1, int s2)
{
    ofstream fout("history.txt", ios::app);
    time_t now = time(0);
    string timeStr = ctime(&now);
    if (!timeStr.empty() && timeStr.back()=='\n') timeStr.pop_back();
    string result = (s1>s2)? "Player 1 Won" : (s2>s1)? "Player 2 Won" : "Draw";
    fout << "[" << timeStr << "] " << name << " | Score: " << s1 << "-" << s2 << " | " << result << "\n";
}

string getUsername()
{
    system("cls");
    show_cursor(true);

    set_positon(10,5);
    cout << YELLOW << "====================Ping Pong Ding Dong Ching Chong====================" << RESET << "\n\n";
    set_positon(10,8);
    cout << "Enter your player name: ";

    string name;
    cin >> name;
    //Edge case: empty/whitespace-only input falls back to a default name
    //instead of leaving the login state undefined.
    if (name.empty()) name = "Guest";

    show_cursor(false);
    return name;
}

//======================== AI Mode (Add-on 1) - trajectory prediction ========================
//Predicts the ball's Y position when it reaches targetX, unfolding wall
//bounces along the way. Returns -1 if the ball isn't currently headed
//toward targetX (an "unreachable target" from this paddle's perspective).
int predictBallY(int startX,int startY,int vx,int vy,int targetX)
{
    if (vx == 0) return -1;
    if ((targetX > startX && vx < 0) || (targetX < startX && vx > 0)) return -1;

    int x = startX, y = startY;
    int guard = 0; //safety guard so a bad state can never infinite-loop
    while (x != targetX && guard < 2000)
    {
        x += vx;
        y += vy;
        if (y <= 0)       { y = -y;                 vy = -vy; }
        else if (y >= height-1){ y = 2*(height-1)-y; vy = -vy; }
        guard++;
    }
    return y;
}

void aiController()
{
    if (!aiMode) return;

    int predictedY = predictBallY(Ballx, Bally, Ball_velocity_x, Ball_velocity_y, width-3);

    if (predictedY == -1)
    {
        //Ball moving away: drift back toward center instead of freezing
        int center = height/2 - paddle_height/2;
        if (paddle2 < center) paddle2++;
        else if (paddle2 > center) paddle2--;
        return;
    }

    //Difficulty controls how noisy/imprecise the AI's aim is
    int error = 0;
    if (difficulty == 1)      error = (rand() % 7) - 3;   //Easy: sloppy aim
    else if (difficulty == 2) error = (rand() % 3) - 1;   //Medium: slight noise
    //Hard: error stays 0, perfect aim

    int targetY = predictedY - paddle_height/2 + error;
    //Edge case: predicted/aim target falls outside the board -> clamp
    //instead of letting the paddle try to move off-screen.
    targetY = max(0, min(height - paddle_height, targetY));

    //Difficulty also controls reaction speed
    int moveStep = paddleSpeed;
    if (difficulty == 1) moveStep = max(1, paddleSpeed - 1); //Easy: slower reflexes

    if (paddle2 < targetY)      paddle2 = min(paddle2 + moveStep, targetY);
    else if (paddle2 > targetY) paddle2 = max(paddle2 - moveStep, targetY);
}

//======================== Color Picker (Add-on 3) ========================
void pickColor(const char* &target, const string &label)
{
    int idx = 0;
    for (int i=0;i<numColors;i++) if (colorPalette[i].code == target) idx = i;

    while (true)
    {
        system("cls");
        set_positon(10,2);
        cout << YELLOW << "Choose a color for " << label << RESET;
        cout << "\n\n";
        for (int i = 0; i < numColors; i++)
        {
            set_positon(10, 4+i);
            if (i == idx) cout << colorPalette[i].code << "-> " << colorPalette[i].name << RESET;
            else          cout << "   " << colorPalette[i].code << colorPalette[i].name << RESET;
        }
        set_positon(10, 6+numColors);
        cout << "w/s to move, Enter to select, b to cancel";

        char key = _getch();
        if (key=='w'||key=='W') idx = (idx==0)? numColors-1 : idx-1;
        else if (key=='s'||key=='S') idx = (idx+1)%numColors;
        else if (key==13) { target = colorPalette[idx].code; return; }
        else if (key=='b'||key=='B') return;
    }
}

//======================== Settings Menu (Add-ons 1,2,3,6,7) ========================
void Settings_Menu()
{
    int idx = 0;
    int totalItems = 8;

    while (true)
    {
        system("cls");
        set_positon(15,2);
        cout << YELLOW << "====================Settings====================" << RESET;

        auto printItem = [&](int i, const string &text){
            set_positon(10, 5+i);
            if (i==idx) cout << BLUE << "-> " << RESET << text << "        ";
            else        cout << "   " << text << "        ";
        };

        printItem(0, string("AI Mode (Player 2): ") + (aiMode? "ON":"OFF"));
        printItem(1, string("Difficulty: ") + (difficulty==1?"Easy":difficulty==2?"Medium":"Hard"));
        printItem(2, string("Paddle Speed: ") + to_string(paddleSpeed));
        printItem(3, string("Ball Speeds Up On Hit: ") + (speedIncreaseEnabled?"ON":"OFF"));
        printItem(4, "Paddle 1 Color");
        printItem(5, "Paddle 2 Color");
        printItem(6, "Ball Color");
        printItem(7, "Border Color");

        set_positon(10, 16);
        cout << RED << "Press b to go back to Main Menu" << RESET;
        set_positon(10,17);
        cout << "w/s: move   a/d: change value   Enter: open color picker";

        char key = _getch();
        if (key=='w'||key=='W') idx = (idx==0)? totalItems-1 : idx-1;
        else if (key=='s'||key=='S') idx = (idx+1)%totalItems;
        else if (key=='b'||key=='B') return;
        else if (key=='a'||key=='A'||key=='d'||key=='D'||key==13)
        {
            if (idx==0) aiMode = !aiMode;
            else if (idx==1)
            {
                if (key=='a'||key=='A') difficulty = (difficulty==1)?3:difficulty-1;
                else                    difficulty = (difficulty==3)?1:difficulty+1;
            }
            else if (idx==2)
            {
                if (key=='a'||key=='A') paddleSpeed = max(1, paddleSpeed-1);
                else                    paddleSpeed = min(4, paddleSpeed+1);
            }
            else if (idx==3) speedIncreaseEnabled = !speedIncreaseEnabled;
            else if (idx==4) pickColor(paddleColor1, "Paddle 1");
            else if (idx==5) pickColor(paddleColor2, "Paddle 2");
            else if (idx==6) pickColor(ballColor, "Ball");
            else if (idx==7) pickColor(borderColor, "Border");
        }
    }
}

//======================== Setup / Drawing ========================
void start_game()
{
    score1=0,score2=0;

    paddle1 = height/2 - paddle_height/2;
    paddle2 = height/2 - paddle_height/2;

    prev_Ball_X = Ballx;
    prev_Ball_y = Bally;
    prev_paddle1 = paddle1,prev_paddle2= paddle2;

    ball_spawn(-1);

    system("cls");
    set_positon(0,0);

    cout << borderColor;
    for (int i=0; i<width+2; i++) cout << "#";

    for (int j=0; j<height; j++)
    {
        set_positon(0,j+1);
        cout << borderColor << "#";
        set_positon(width +1,j+1);
        cout << borderColor << "#";
        set_positon(width/2 +1,j+1);
        cout << RESET << "|";
    }

    set_positon(0,height+1);
    cout << borderColor;
    for (int i=0;i<width+2;i++) cout << "#";
    cout << RESET;
}

void in_game()
{
    set_positon(prev_Ball_X+1,prev_Ball_y+1);
    if (prev_Ball_X==width/2)
        cout <<RESET<<"|";
    else
        cout << " ";

    for (int i=0;i<paddle_height;i++)
    {
        set_positon(1,prev_paddle1+i+1);
        cout <<" ";
    }
    for (int i=0;i<paddle_height;i++)
    {
        set_positon(width-1,prev_paddle2+i+1);
        cout <<" ";
    }

    set_positon(Ballx+1,Bally+1);
    cout << ballColor <<"0"<<RESET;

    for (int i=0;i<paddle_height;i++)
    {
        set_positon(1,paddle1+i+1);
        cout<<paddleColor1 << "|";
        set_positon(width-1,paddle2+i+1);
        cout <<paddleColor2<< "|";
    }

    set_positon(44,height+3);
    cout <<CYAN<< "Player-1 Score: " << score1;
    set_positon(44,height+4);
    cout <<GREEN<< "Player-2 Score: " << score2;
    set_positon(44,height+5);
    cout<<YELLOW << "Target   Score: " << max_score<<RESET;

    set_positon(44,height+6);
    cout << (aiMode? "AI Mode: ON " : "2-Player Mode") << RESET;

    prev_Ball_X = Ballx;
    prev_Ball_y = Bally;
    prev_paddle1 = paddle1;
    prev_paddle2 = paddle2;

    set_positon(40,height+8);
    cout<<RED<<"Press x to exit the game"<<RESET;
}

//======================== Controls ========================
void controller()
{
    if(_kbhit())
    {
        char key = _getch();

        if(key == 'w') paddle1 = max(0, paddle1-paddleSpeed);
        if(key == 's') paddle1 = min(height-paddle_height, paddle1+paddleSpeed);

        //Player 2's manual keys only apply when a human is controlling paddle2
        if (!aiMode)
        {
            if(key == 'i') paddle2 = max(0, paddle2-paddleSpeed);
            if(key == 'k') paddle2 = min(height-paddle_height, paddle2+paddleSpeed);
        }

        if (key == 'x') game_runner= false;
    }
}

//======================== Physics ========================
void main_logic()
{
    Ballx = Ballx + Ball_velocity_x;
    Bally = Bally + Ball_velocity_y;

    if (Bally<=0 ||  Bally>=height-1)
        Ball_velocity_y=-Ball_velocity_y;

    //Left paddle collision
    if (Ballx==2  && Bally>=paddle1 && Bally<paddle1 + paddle_height )
    {
        Ball_velocity_x=-Ball_velocity_x;
        int paddle_half = paddle1 + paddle_height/2;

        if (Bally < paddle_half) Ball_velocity_y=-abs(Ball_velocity_y);
        else                     Ball_velocity_y=abs(Ball_velocity_y);

        if (speedIncreaseEnabled) speedLevel = min(speedLevel+1, max_speed_cap);
        int sx = (Ball_velocity_x>0)?1:-1;
        int sy = (Ball_velocity_y>0)?1:-1;
        Ball_velocity_x = sx*speedLevel;
        Ball_velocity_y = sy*speedLevel;
    }

    //Right paddle collision
    if (Ballx==width-3 && Bally >= paddle2 && Bally<paddle2 + paddle_height)
    {
        Ball_velocity_x=-Ball_velocity_x;
        int paddle_half = paddle2 + paddle_height/2;

        if (Bally< paddle_half) Ball_velocity_y=-abs(Ball_velocity_y);
        else                    Ball_velocity_y=abs(Ball_velocity_y);

        if (speedIncreaseEnabled) speedLevel = min(speedLevel+1, max_speed_cap);
        int sx = (Ball_velocity_x>0)?1:-1;
        int sy = (Ball_velocity_y>0)?1:-1;
        Ball_velocity_x = sx*speedLevel;
        Ball_velocity_y = sy*speedLevel;
    }

    if (Ballx<2)
    {
        score2++;
        ball_spawn(1);
    }
    if (Ballx>width-3)
    {
        score1++;
        ball_spawn(-1);
    }
}

//======================== End Screen ========================
void Game_End()
{
    system("cls");

    //Persist this match (Add-ons 4 & 8)
    updateHighScore(username, max(score1,score2));
    logHistory(username, score1, score2);

    cout<<endl<<endl<<endl<<RESET<<YELLOW;
    cout <<"                                   #-----------------------Game Over-------------------------#"<< "\n";
    cout <<"                                   #                                                         #"<< "\n";
    if (score1>score2)
        cout <<"                                   #                Player 1 Wins the Game!                  #"<< "\n";
    else if (score1<score2)
        cout <<"                                   #                Player 2 Wins the Game!                  #"<< "\n";
    else
        cout <<"                                   #                   Noone Won the Game!                   #"<< "\n";
    cout <<"                                   #                                                         #"<< "\n";
    cout <<"                                   #-----------------------Game Over-------------------------#"<< "\n";

    cout <<endl;
    cout<<CYAN;
    cout <<"                                                        Player 1 Score: " << score1<<endl;
    cout  <<GREEN;
    cout <<"                                                        Player 2 Score: " << score2<<endl<<RESET;

    //Show the leaderboard (Edge case: file may be empty on a fresh install)
    cout << endl << YELLOW << "                                                   -- Top Scores --" << RESET << endl;
    vector<ScoreRecord> records = loadHighScores();
    if (records.empty())
    {
        cout << "                                                   (no scores recorded yet)" << endl;
    }
    else
    {
        int shown = 0;
        for (auto &r : records)
        {
            if (shown>=5) break;
            cout << "                                                   " << (shown+1) << ". " << r.name << " - " << r.score << endl;
            shown++;
        }
    }

    cout << endl << RED;
    cout<<"                                                   Press any button to continue"<<RESET;
    _getch();
}

//======================== Starting Menu ========================
int Starting_Menu()
{
    system ("cls");
    system("Color 07");
    int button=1;

    while(true)
    {
        system("cls");
        printf("%s", YELLOW);

        set_positon(16,3);
        cout <<"=================Ping Pong Ding Dong Ching Chong================="<<RESET;
        cout <<endl;

        set_positon(10,4);
        cout << "Logged in as: " << CYAN << username << RESET;

        set_positon(38,6);
        if (button==1) cout <<BLUE<<"1."<<RESET<<"Begin the Game"<<BLUE<<"<--"<<RESET;
        else           cout <<BLUE<<"1."<<RESET<<"Begin the Game";

        set_positon(38,7);
        if (button==2) cout <<BLUE<<"2."<<RESET<<"Settings"<<BLUE<<"<--"<<RESET;
        else           cout <<BLUE<<"2."<<RESET<<"Settings";

        set_positon(38,8);
        if (button==3) cout <<BLUE<<"3."<<RESET<<"How to Play"<<BLUE<<"<--"<<RESET;
        else           cout <<BLUE<<"3."<<RESET<<"How to Play";

        set_positon(38,9);
        if (button==4) cout <<BLUE<<"4."<<RESET<<"Exit!"<<BLUE<<"<--"<<RESET;
        else           cout <<BLUE<<"4."<<RESET<<"Exit!";

        char key = _getch();

        if(key=='1') return 1;
        if(key=='2') return 2;
        if(key=='3') return 3;
        if(key=='4') return 4;

        if (key == 'w' || key == 'W')
        {
            if (button ==1) button = 4;
            else button--;
        }
        else if (key == 's' || key == 'S')
        {
            if (button ==4) button = 1;
            else button++;
        }
        else if (key==13 || key == ' ')
        {
            return button;
        }
    }
}

//======================== Players Guide ========================
void players_Guide()
{
    system ("cls");

    set_positon(30,3);
    cout << "====================Players Guide====================";

    set_positon(47,5);
    cout <<CYAN << "For Player 1:\n";
    set_positon(47,6);
    cout <<RESET<< "Press w to Move up\n";
    set_positon(47,7);
    cout <<"Press s to Move Down\n";

    set_positon(47,9);
    cout<<GREEN << "For Player 2 (disabled if AI Mode is ON):";
    set_positon(47,10);
    cout <<RESET << "Press i to Move up";
    set_positon(47,11);
    cout<< "Press k to Move Down";

    set_positon(47,13);
    cout<<RED<<"Press x to exit the game";
    set_positon(47,14);
    cout<<RESET<< "First player to reach "<<max_score<<" scores wins!!";
    set_positon(47,15);
    cout << "Visit Settings from the main menu to enable AI Mode,";
    set_positon(47,16);
    cout << "change difficulty, colors, and speeds.";

    set_positon(30,19);
    cout<<YELLOW<<"Press anything on keyboard to return to main menu........"<<RESET;

    _getch();
}

//======================== Main ========================
int main()
{
    srand((unsigned)time(0));
    show_cursor(false);

    username = getUsername();

    int choice= 0;

    while (true)
    {
        choice = Starting_Menu();
        if (choice==1)
        {
            game_runner= true;
            start_game();
            while (game_runner)
            {
                in_game();
                controller();
                if (aiMode) aiController();
                main_logic();

                if(score1>=max_score || score2>=max_score)
                    game_runner=false;

                Sleep(50);
            }
            Game_End();
        }
        else if (choice==2)
        {
            Settings_Menu();
        }
        else if (choice==3)
        {
            players_Guide();
        }
        else if (choice==4)
            break;
    }

    show_cursor(true);
    return 0;
}
