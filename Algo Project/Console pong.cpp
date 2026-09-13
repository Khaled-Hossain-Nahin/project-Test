
#include <bits/stdc++.h>
#include <conio.h>
//input Functions

//to avoid writing std multiple times
//SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE),axis);
#include <windows.h>

using namespace std;

//Setting all values....
    //for values: border,paddle,max level
    int width=80,height=20,paddle_height=4,max_score=5;

    //x=horizontal position
    //y=vertical position
    int Ballx,Bally;

    //vx=horizontal velocity
    //vy=vertical velocity
    int Ball_velocity_x,Ball_velocity_y;

    int initial_Ball_Speed=1;
    //both players score counter
    int score1=0,score2=0;

    //paddles player control
    int paddle1,paddle2;

    //to store the previous position of the ball
    int prev_Ball_X,prev_Ball_y,prev_paddle1,prev_paddle2;

    //true, false,int
    bool game_runner= true;

//Coloring Text:- ANSCII Esacpe colors
    const char *RESET = "\033[0m";
    const char *CYAN = "\033[1;36m";
    const char *YELLOW = "\033[1;33m";
    const char *GREEN = "\033[1;32m";
    const char *RED = "\033[1;31m";
    const char *BLUE = "\033[1;34m";
    const char *WHITE = "\033[1;37m";
    const char *MAGENTA ="\033[0;35m";

//---------------------------------Creating the position of the cursor----------------------------------------------
void set_positon(int x,int y)
{
    COORD axis;
    axis.X= x;
    axis.Y= y;

    //to handle the cursor position and get its info and the output
    //Refreshes the screen to avoid any flicker.
    //Here x=coloumn and y=row
    SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE),axis);
}

//---------Gives the ball an initial direction, initial position and speed------------------------------------------
void ball_spawn(int direction)
{
    //Resets speed of the ball when it spawns again
    initial_Ball_Speed=1;

    //set position where ball spawns
    Ballx= width /2;
    Bally=height/2;

    //set direction = 1 *1= Right side
    // set direction = -1 * 1= Left side
    Ball_velocity_x = direction* initial_Ball_Speed;
    Ball_velocity_y  = 1;
}

//-------------------Funtion to prepare the game before it starts--------------------------------------------------
void start_game()
{
    //Resets the score for every time the game begins
    score1=0,score2=0;

    //Initial paddle position, to put the paddles in the center
    paddle1 = height/2 - paddle_height/2;
    paddle2 = height/2 - paddle_height/2;

    //Initial position of ball for drawing it.
    //prev_Ball_X,prev_Ball_y,prev_paddle1,prev_paddle2
    prev_Ball_X = Ballx;
    prev_Ball_y = Bally;

    //stores the preivous position of the paddle to find the new position
    prev_paddle1 = paddle1,prev_paddle2= paddle2;

    //Calling the function to set its spawn value and direction
    ball_spawn(-1);

    //clears the screen
    system("cls");

    set_positon(0,0);
    //Drawing the top wall
    for (int i=0; i<width+2; i++)
    {
        //printf("#");
        cout << "#";
    }

    //Drawing the side walls and center Line
    for (int j=0; j<height; j++)
    {
        //Left side wall, axix(x,y)
        set_positon(0,j+1);
        cout << "#";

        //Right side Wall
        set_positon(width +1,j+1);
        cout << "#";

        //Center Line wall
        set_positon(width/2 +1,j+1);
        cout << "|";
    }

    //Draw bottom wall at psoition(height+1)
    set_positon(0,height+1);
    for (int i=0;i<width+2;i++)
    {
        cout << "#";
    }

}

//------------------Drawing in Game structures-Ball, Walls---------------------------------------------------------
void in_game()
{
    //replaces the ball with a space
    set_positon(prev_Ball_X+1,prev_Ball_y+1);
    if (prev_Ball_X==width/2)
        cout <<RESET<<"|"; //so middle bar is not replaced by space
    else
        cout << " ";
    // Re-draws the | , after the ball passes through

    //----------replaceing the paddle with a space------------------

    //For player 1:
    for (int i=0;i<paddle_height;i++)
    {
        set_positon(1,prev_paddle1+i+1);
            cout <<" ";
    }

    //For Player 2:
    for (int i=0;i<paddle_height;i++)
    {
        set_positon(width-1,prev_paddle2+i+1);
            cout <<" ";
    }
    //----------replaceing the paddle with a space------------------


    //----------Drawing new the ball with---------------------------
    set_positon(Ballx+1,Bally+1);
        cout << RED<<"0"<<RESET;
    //----------Drawing new the ball with---------------------------


    //----------replaceing one paddle with a another paddle------------------
    for (int i=0;i<paddle_height;i++)
    {

        //Drawing the paddle for player 1 in blue color
        set_positon(1,paddle1+i+1);
            cout<<CYAN << "|";

        //Drawing the paddle for player 2 in blue color
        set_positon(width-1,paddle2+i+1);
            cout <<GREEN<< "|";

    }
    //----------replaceing one paddle with a another paddle------------------


   //---------------------Drawing Score-------------------------
    set_positon(44,height+3);
        cout <<CYAN<< "Player-1 Score: " << score1;

    set_positon(44,height+4);
        cout <<GREEN<< "Player-2 Score: " << score2;

    set_positon(44,height+5);
        cout<<YELLOW << "Target   Score: " << max_score<<RESET;
   //---------------------Drawing Score-------------------------



    //Setting values to their previous values,
    prev_Ball_X = Ballx;
    prev_Ball_y = Bally;
    prev_paddle1 = paddle1;
    prev_paddle2 = paddle2;

    set_positon(40,height+6);
        cout<<RED<<"Press x to exist the game";

}

//-------------Controlling the game with keyborad input------------------------------------------------------------
void controller()
{
    //Is there an Input?
    if(_kbhit())
    {
        //Variable 'key' stores the user input button
        //getch -> get character
        char key = _getch();

        //to control Paddle 1 with keyboard
        if(key == 'w' && paddle1>0)
        {
            paddle1--;
        }
        if(key == 's' && paddle_height+paddle1<height)
        {
            paddle1++;
        }

        //to control Paddle 2 with keyboard
        if(key == 'i' && paddle2>0)
        {
            paddle2--;
        }
        if(key == 'k' && paddle_height+paddle2<height)
        {
            paddle2++;
        }

        //To exit the game with key
        if (key == 'x')
        {
            //bool is false,so game doesnt run
            game_runner= false;
        }
    }
}

//-------Main Logic and Physics of the game------------------------------------------------------------------------
void main_logic()
{

    //Changes its positon based on the velocity of the ball which is in the ball_spawn()
    Ballx = Ballx + Ball_velocity_x;
    Bally = Bally + Ball_velocity_y;

    //Logic so that the ball bounces with the top wall and bottom wall for its Y-axis position
    if (Bally<=0 ||  Bally>=height-1)
    {
        Ball_velocity_y=-Ball_velocity_y;
    }
    //For Ball's  X-axis position

    // for Collision with left paddle when ball is at collum =2
    if (Ballx==2  && Bally>=paddle1 && Bally<paddle1 + paddle_height )
    {
        //when the ball hits the paddle 1 it gets negative velocity
        Ball_velocity_x=-Ball_velocity_x;

        // Calculating if the Ball hits upper part or lower part of the paddle
        int paddle_half = paddle1 + paddle_height/2;

        //gets negative velocity if hits lower part of the ball
        if (Bally < paddle_half)
        {
            Ball_velocity_y=-abs(Ball_velocity_y);
        }

        //gets positive velocity if hits uppper part of the ball
        else
        {
            Ball_velocity_y=abs(Ball_velocity_y);
        }

        //increases the ball speed everytime a player hits the ball
        initial_Ball_Speed++;

    }

    //Logic is the ball collides with the right side of the ball when it is at Width-3
    if (Ballx==width-3 && Bally >= paddle2 && Bally<paddle2 + paddle_height)
    {
        Ball_velocity_x=-Ball_velocity_x;
        int paddle_half = paddle2 + paddle_height/2;

        if (Bally< paddle_half)
        {
            Ball_velocity_y=-abs(Ball_velocity_y);
        }
        else
        {
            Ball_velocity_y=abs(Ball_velocity_y);
        }
        initial_Ball_Speed++;

    }

    //-----------Counting the Score of the ball------------
    if (Ballx<2)
    {
        score2++;
        //calling the ball func. where
        ball_spawn(1);

        //pauses the game for one second
        //Sleep(00);
    }

    if (Ballx>width-3)
    {
        score1++;
        ball_spawn(-1);
        //pauses the game for one second
        //Sleep(1000);
    }
}

//---------------THe screen after the game ends--------------------------------------------------------------------
void Game_End()
{
    //clears the screen and then sets it to color (X=balck,y=Light aqua)

    system("cls");
    //Beep(800, 30);
    //End Game Screen

    cout<<endl<<endl<<endl<<endl<<RESET<<YELLOW;
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
    //End Game Score Count
    cout<<CYAN;
    cout <<"                                                        Player 1 Score: " << score1<<endl;
    cout  <<GREEN;
    cout <<"                                                        Player 2 Score: " << score2<<endl<<RED;

    cout << endl;
    cout<<"                                                   Press any button to continue"<<RESET;
    //Reads any character from the keyboard and takes back to main menu
    _getch();
}

//-------------The Screen for the starting menu--------------------------------------------------------------------
int Starting_Menu()
{
    system ("cls");
    system("Color 07");
    int button=1;

    while(true)
    {
        //clears the game after ending a game, if user returns to the main menu
        system("cls");
        printf("%s", YELLOW);

        set_positon(16,3);
            cout <<"=================Ping Pong Ding Dong Ching Chong================="<<RESET;
            cout <<endl;




        set_positon(38,5);
            if (button==1)
                cout <<BLUE<<"1."<<RESET<<"Begin the Game"<<BLUE<<"<--"<<RESET;
            else
                cout <<BLUE<<"1."<<RESET<<"Begin the Game";

        set_positon(38,6);
            if (button==2)
                cout <<BLUE<<"2."<<RESET<<"How to Play"<<BLUE<<"<--"<<RESET;
            else
                cout <<BLUE<<"2."<<RESET<<"How to Play";

        set_positon(38,7);
            if (button==3)
                cout <<BLUE<<"3."<<RESET<<"Exit!"<<BLUE<<"<--"<<RESET;
            else
                cout <<BLUE<<"3."<<RESET<<"Exit!";


        //Reads a Key from the keyboard
        char key = _getch();

        //Number keys makes an option
        if(key=='1') return 1;
        if(key=='2') return 2;
        if(key=='3') return 3;

        //Moves "-->" to up
        if (key == 'w' || key == 'W')
        {
            if (button ==1) button = 3;
            else button--;
        }

        else if (key == 's' || key == 'S')
        {
            if (button ==3) button = 1;
            else button++;
        }

        //key 13 is equal to pressing Enter on keyboard and  other character is space
        else if (key==13 || key == ' ')
        {
            return button;
        }

    }
}

//-------------Guide texts Keymaps to control the game-------------------------------------------------------------------------
void players_Guide()
{
    system ("cls");

    set_positon(30,3);
        cout << "====================Players Guide====================";
    //control show

    set_positon(47,5);
        cout <<CYAN << "For Player 1:\n";
    set_positon(47,6);
        cout <<RESET<< "Press w to Move up\n";
    set_positon(47,7);
        cout <<"Press s to Move Down\n";

    set_positon(47,9);
        cout<<GREEN << "For Player 2:";
    set_positon(47,10);
        cout <<RESET << "Press i to Move up";
    set_positon(47,11);
        cout<< "Press k to Move Down";

    set_positon(47,13);
        cout<<RED<<"Press x to exist the game";
    set_positon(47,14);
        cout<<RESET<< "First player to reach "<<max_score<<" scores wins!!";
    set_positon(30,17);
        cout<<YELLOW<<"Press anything on keyboard to return to main menu........"<<RESET;

    _getch();
}

//-------------------------The main Game Function------------------------------------------------------------------
int main()
{

    //Cursor Hiding: It retrieves the current console cursor settings,
    //sets the bVisible flag within the cursorInfo structure to false,
    //and then applies the change. This hides the annoying blinking cursor from the user,
    //which would otherwise flicker intensely during the game's continuous redrawing.
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
   SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    int choice= 0;

    while (true)
    {
        choice = Starting_Menu();
        if (choice==1)
        {
            game_runner= true;
            start_game(); //Calls this function and spawns the ball
            while (game_runner)
            {

                //Calls in game logical drawing function which draws the struct
                in_game();
                //calls this function so we can control through keyboard
                controller();
                //the logical function that controls the physics of the game
                main_logic();

                if(score1>=max_score || score2>=max_score)
                {
                    game_runner=false;
                    //ends the game and after reaching max_score
                }

                //Controls the speed of the Game
                Sleep(50);
            }
            Game_End();//Puts the game over screeen after the game
        }
        else if (choice==2)
        {
            players_Guide();
        }
        else if (choice==3)
            break;
    }


    //hides the annoying cursor
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    return 0;
}
