/**************************************************
* Filename:       checkers.cpp
* Author:         Austin Anderson
* Class:          CSI 3336
* Date Modified:  2016-02-03
*   -File Created
* Description:   
* 
***************************************************/

#include <ncurses.h>

#include <iostream>
#include <vector>
#include <cstdlib>
#include <sys/file.h>
#include <fstream>

#include <stdio.h>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;
enum COLOR{
C_START,
C_HIGHLIGHT,
C_HOVER,
C_OFF_SQUARE,
C_EMPTY_SQUARE,
C_RED,
C_WHITE,
COLORS_SIZE,
};

const int CLICK_WHILE_DRAG=4097;
const int SQUARE_X=6;
const int SQUARE_Y=3;


enum GAME_MODE{PASS_N_PLAY, NETWORKED, AI};

class Piece{
    public:
        Piece(bool isRed){
            red=isRed;
            king=false;
        }
        void kingMe(){
            if(!king){
                king=true;
            }
        }
        bool isKing(){
            return king;
        }
        bool isRed(){
            return red;
        }

        friend ostream& operator<<(ostream& os, Piece& p){
            os<<p.king<<" "<<p.red<<" ";
            return os;
        }
        friend istream& operator>>(istream& is, Piece& p){
            is>>p.king>>p.red;
            return is;
        }

    private:
        bool king;
        bool red;

};

class Square{
    public:
        Square(int color){
            bgColor=color;
            curColor=C_EMPTY_SQUARE;
            piece=NULL;
            init_pair(C_HIGHLIGHT    ,COLOR_BLACK,COLOR_GREEN);
            init_pair(C_HOVER        ,COLOR_BLACK,COLOR_CYAN);
            init_pair(C_OFF_SQUARE   ,COLOR_BLACK,COLOR_BLUE);
            init_pair(C_EMPTY_SQUARE ,COLOR_WHITE,COLOR_BLACK);
            init_pair(C_RED          ,COLOR_BLACK,COLOR_RED);
            init_pair(C_WHITE        ,COLOR_BLACK,COLOR_WHITE);
        }
        void setPiece(Piece* newp){
            piece=newp;
        }
        Piece* getPiece(){
            return piece;
        }
        void curSelect(){
            curColor=C_HIGHLIGHT;
        }
        void curHover(){
            curColor=C_HOVER;
        }
        void curClear(){
            curColor=C_EMPTY_SQUARE;
        }
        void print(int x,int y){
            x=x*SQUARE_X;
            y=y*SQUARE_Y;
            string bg(SQUARE_X,' ');
            string pc(SQUARE_X/3,' ');
            string pking(SQUARE_X/3,'_');
            COLOR color=(COLOR)bgColor;
            if(curColor!=C_EMPTY_SQUARE){
                color=(COLOR)curColor;
            }
            attron(COLOR_PAIR(color));
            mvprintw(y  ,x,"%s",bg.c_str());
            mvprintw(y+1,x,"%s",bg.c_str());
            mvprintw(y+2,x,"%s",bg.c_str());
            attroff(COLOR_PAIR(color));
            if(hasPiece()){
                color=C_WHITE;
                if(piece->isRed()){
                    color=C_RED;
                }
                attron(COLOR_PAIR(color));
                if(isKing()){
                    mvprintw(y,x+2,"%s",pking.c_str());
                }
                mvprintw(y+1,x+2,"%s",pc.c_str());
                attroff(COLOR_PAIR(color));
            }
        }
        bool isBlack(){
            return bgColor==C_EMPTY_SQUARE;
        }
        bool hasPiece(){
            return piece!=NULL;
        }
        bool isKing(){
            if(hasPiece()){
                return piece->isKing();
            }
            return false;
        }
        void kingMe(){
            if(hasPiece()){
                piece->kingMe();
            }
        }
        void newPiece(bool red){
            piece=new Piece(red);
        }
        void deletePiece(){
            if(hasPiece()){
                delete piece;
            }
        }
        friend ostream& operator<<(ostream& os, Square& s){
            os<<s.bgColor<<" "<<" "<<s.hasPiece()<<" ";
            if(s.hasPiece()){
                os<<*(s.piece)<<" ";
            }
            return os;
        }
        friend istream& operator>>(istream& is, Square& s){
            bool shouldHavePiece;
            is>>s.bgColor>>shouldHavePiece;
            if(shouldHavePiece){
                if(!s.hasPiece()){
                    s.newPiece(true);
                }
                is>>*(s.piece);
            }else{
                if(s.hasPiece()){
                    s.setPiece(NULL);
                }
            }
            return is;
        }
    private:
        int bgColor;//type saftey vs lazyness,
        int curColor;//change this type to COLOR and overload operators
        Piece* piece;

};
class Board{
    public:
        Board(){
            vector<Square> row;
            for(int i=0;i<8;i++){
                for(int j=0;j<8;j++){
                    if((j+i)%2==0){
                        row.push_back(Square(C_OFF_SQUARE));
                    }
                    else{
                        row.push_back(Square(C_EMPTY_SQUARE));
                        if(i<3){
                            row[j].newPiece(false);
                        }
                        if(i>4){
                            row[j].newPiece(true);
                        }
                    }
                }
                mat.push_back(row);
                row.clear();
            }
            xNdx=5;
            yNdx=4;
            xNdxFrom=-1;
            yNdxFrom=-1;
            selecting=false;
            mat[yNdx][xNdx].curHover();
            redTurn=true;
            mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        }
        ~Board(){
            for(int i=0;i<8;i++){
                for(int j=0;j<8;j++){
                    mat[i][j].deletePiece();
                }
            }
        }
        void print(){
            for(int i=0;i<mat.size();i++){
                for(int j=0;j<mat[0].size();j++){
                    mat[j][i].print(i,j);
                }
            }
        }
        bool updateMouse(int charX,int charY){
            translateToBoardCoords(charX,charY);
            bool flag=false;
            if(charY>=0&&charY<mat.size()){
                if(charX>=0&&charX<mat[0].size()){
                    if(mat[charY][charX].isBlack()){

                        mat[yNdx][xNdx].curClear();
                        yNdx=charY;
                        xNdx=charX;
                        setCur();
                        flag=true;
                    }
                }
            }
            return flag;
        }
        void translateToBoardCoords(int &x,int &y){
            x=x/SQUARE_X;
            y=y/SQUARE_Y;
        }
        void setCur(){
            mat[yNdx][xNdx].curHover();
            if(xNdxFrom>0&&yNdxFrom>0){
                mat[yNdxFrom][xNdxFrom].curSelect();
            }
        }
        void select(){
            selecting=true;
            xNdxFrom=xNdx;
            yNdxFrom=yNdx;
            setCur();
        }
        void deselect(){
            selecting=false;
            if(yNdxFrom>0&&yNdxFrom<mat.size()){
                if(xNdxFrom>0&&xNdxFrom<mat.size()){
                    mat[yNdxFrom][xNdxFrom].curClear();
                }
            }
            xNdxFrom=-1;
            yNdxFrom=-1;
            setCur();
        }
        bool makeMove(bool redPlayer){
            bool flag=redTurn;
            if(redTurn==redPlayer){
                Square* from=&mat[yNdxFrom][xNdxFrom];
                Square* to=&mat[yNdx][xNdx];
                int dy=yNdx-yNdxFrom;
                int dx=xNdx-xNdxFrom;
                if(from->hasPiece()&&(from->getPiece()->isRed()==redTurn)){
                    if(!to->hasPiece()){
                        //if up one, or king and two in any direction
                        if((dy==-1||(from->isKing()&&abs(dy==1)))&&abs(dx)==1){
                            to->setPiece(from->getPiece());
                            from->setPiece(NULL);
                            changeTurn();
                        }
                        else{
                            //if up two, or king and two in any direction
                            if((dy==-2||(from->isKing()&&abs(dy==2)))&&abs(dx)==2){
                                Square* mid=&mat[yNdxFrom+sub1(dy)][xNdxFrom+sub1(dx)];
                                if(mid->hasPiece()&&mid->getPiece()->isRed()!=from->getPiece()->isRed()){
                                    mid->setPiece(NULL);
                                    to->setPiece(from->getPiece());
                                    from->setPiece(NULL);
                                    changeTurn();
                                }

                            }
                            // find jumps, if possible double jump don't change turn
                             
                        }
                    }

                }
                if(yNdx==0&&flag!=redTurn){
                    to->kingMe();
                }
                deselect();
            }
            return flag!=redTurn;
        }
        int sub1(int two){
            if(two<0){
                return -1;
            }
            return 1;
        }

        void changeTurn(){
            redTurn=!redTurn;
        }

        //changes view for changing players
        void invert(){
            vector<Square> temp;
            Square tempInner(C_EMPTY_SQUARE);
            mat[yNdx][xNdx].curClear();
            for(int i=0;i<mat.size()/2;i++){
                for(int j=0;j<mat[0].size()/2;j++){
                    tempInner=mat[i][j];//flip row horizontal
                    mat[i][j]=mat[i][mat[0].size()-(j+1)];
                    mat[i][mat[0].size()-(j+1)]=tempInner;
                }
                temp=mat[i];//swap rows
                mat[i]=mat[mat.size()-(i+1)];
                mat[mat.size()-(i+1)]=temp;
                for(int j=0;j<mat[0].size()/2;j++){
                    tempInner=mat[i][j];//flip row horizontal
                    mat[i][j]=mat[i][mat[0].size()-(j+1)];
                    mat[i][mat[0].size()-(j+1)]=tempInner;
                }
            }
            mat[yNdx][xNdx].curHover();
        }
        bool isSelecting(){
            return selecting;
        }
        bool getTurn(){
            return redTurn;
        }
        friend ostream& operator<<(ostream& os, Board& b){
            for(int i=0;i<8;i++){
                for(int j=0;j<8;j++){
                    os<<b.mat[i][j]<<" ";
                }
            }
            os<<b.redTurn<<" ";
            return os;
        }
        friend istream& operator>>(istream& is, Board& b){
            for(int i=0;i<8;i++){
                for(int j=0;j<8;j++){
                    is>>b.mat[i][j];
                }
            }
            is>>b.redTurn;
            b.deselect();
            return is;
        }


    private:
        vector<vector<Square> > mat;
        int xNdx;
        int yNdx;
        int xNdxFrom;
        int yNdxFrom;
        bool selecting;
        bool redTurn;

};

class Interface{
    public:
        Interface(bool isRedPlayer, GAME_MODE mode){
            this->mode=mode;
            redPlayer=isRedPlayer;
            if(!redPlayer){
                board.invert();
            }
            turn=0;
            fileName="./SaveGame";

            print();
        }

        void print(){
            board.print();
            refresh();
        }
        void play(){
            bool done=false;
            while(!done){
                int pressed=getch();
                if (pressed == KEY_MOUSE){
                    if(getmouse(&event) == OK){
                        board.updateMouse(event.x,event.y);
                        if(event.bstate==CLICK_WHILE_DRAG||event.bstate==BUTTON1_CLICKED){

                            if(board.isSelecting()){
                                if(board.makeMove(redPlayer)){
                                    print();
                                    usleep(500000);
                                    board.invert();
                                    if(mode==NETWORKED){
                                        encode();//encode
                                        board.invert();
                                    }else if(mode==PASS_N_PLAY){
                                        redPlayer=!redPlayer;
                                    }
                                }
                            }
                            else{
                                board.select();
                            }
                        }
                    }
                }
                else if(pressed=='q'){
                    done=true;
                }
                if(mode==NETWORKED){
                    decode();//decode
                }
                print();
            }
        }
        string getFileName(){
            return fileName;
        }

        


    private:

        void encode(){
            int file=open((fileName+".lock").c_str(),O_CREAT);
            //cout<<"encode getting lock"<<endl;
            flock(file, LOCK_EX);
            ofstream out;
            out.open(fileName.c_str());
            out<<++turn<<" ";
            out<<board;
            out.flush();
            out.close();
            flock(file, LOCK_UN);
            close(file);
        }
        void decode(){
            int file=open((fileName+".lock").c_str(),O_RDONLY);
            //cout<<"decode getting lock"<<endl;
            flock(file, LOCK_EX);
            ifstream in;
            in.open(fileName.c_str());
            long storedTurn;
            in>>storedTurn;
            if(storedTurn>turn){
                turn=storedTurn;
                in>>board;
            }
            in.close();
            flock(file, LOCK_UN);
            close(file);
        }
        GAME_MODE mode;
        bool redPlayer;
        Board board;
        string fileName;
        long turn;
        MEVENT event;


};
int main(int argc, char** argv){

    putenv("TERM=xterm-1002");//makes click and drag work
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    clear();
    cbreak();
    start_color();
    if(argc==3){
        GAME_MODE mode=PASS_N_PLAY;
        bool red=false;
        if('r'==argv[1][0]){
            red=true;
        }
        if('p'==argv[2][0]){
            mode=PASS_N_PLAY;
        }
        else if('n'==argv[2][0]){
            mode=NETWORKED;
        }
        Interface i(red,mode);
        i.play();
        system((("rm "+i.getFileName())+" 2>/dev/null").c_str());
        system((("rm -f "+i.getFileName())+".lock 2>/dev/null").c_str());
    }
    endwin();
    return 0;
}
