#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>
#define SIZE 8
#define MAX_MOVES 1000
/* COLORS */
#define RESET "\033[0m"
#define WHITE_P "\033[1;37m"
#define BLACK_P "\033[1;31m"
#define LIGHT "\033[48;5;250m"
#define DARK  "\033[48;5;240m"
#define HIGHLIGHT "\033[48;5;82m"
/* PIECE */
typedef struct {
    char type;
} Piece;
/* MOVE (UNDO) */
typedef struct {
    int r1,c1,r2,c2;
    Piece* captured;
} Move;
Piece* board[8][8];
int highlight[8][8];
Move history[MAX_MOVES];
int moveCount=0;
char turn='W';
/* ---------- SYMBOLS ---------- */
const char* symbol(char p) {
    switch(p) {
        case 'K': return "K";
        case 'Q': return "Q";
        case 'R': return "R";
        case 'B': return "B";
        case 'N': return "N";
        case 'P': return "P";
        case 'k': return "k";
        case 'q': return "q";
        case 'r': return "r";
        case 'b': return "b";
        case 'n': return "n";
        case 'p': return "p";
        default:  return " ";
    }
}
int isWhite(char p){ return isupper(p); }
/* ---------- UTILS ---------- */
int inside(int r,int c){ return r>=0&&r<8&&c>=0&&c<8; }
void chessToIndex(char *p,int *r,int *c){
    *c=p[0]-'a';
    *r=8-(p[1]-'0');
}
/* ---------- INIT ---------- */
void initBoard(){
    const char *s[8]={
        "rnbqkbnr","pppppppp","........","........",
        "........","........","PPPPPPPP","RNBQKBNR"
    };
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++)
            if(s[i][j]!='.'){
                board[i][j] = (Piece*)calloc(1, sizeof(Piece));
                board[i][j]->type=s[i][j];
            } else board[i][j]=NULL;
}
/* ---------- PATH CLEAR ---------- */
int pathClear(int r1,int c1,int r2,int c2){
    int dr=(r2>r1)-(r2<r1);
    int dc=(c2>c1)-(c2<c1);
    r1+=dr; c1+=dc;
    while(r1!=r2||c1!=c2){
        if(board[r1][c1]) return 0;
        r1+=dr; c1+=dc;
    }
    return 1;
}
/* ---------- MOVE VALIDATION (NO TURN CHECK HERE) ---------- */
int validMove(int r1,int c1,int r2,int c2){
    if(!inside(r1,c1)||!inside(r2,c2)||!board[r1][c1]) return 0;
    char p=board[r1][c1]->type;
    if(board[r2][c2] &&
       isWhite(board[r2][c2]->type)==isWhite(p))
        return 0;
    int dr=r2-r1, dc=c2-c1;
    switch(tolower(p)){
        case 'p':{
            int dir=isWhite(p)?-1:1;
            int start=isWhite(p)?6:1;
            if(dc==0 && dr==dir && !board[r2][c2])
                return 1;
            if(dc==0 && dr==2*dir && r1==start &&
               !board[r1+dir][c1] && !board[r2][c2])
                return 1;
            if(abs(dc)==1 && dr==dir && board[r2][c2] &&
               isWhite(board[r2][c2]->type)!=isWhite(p))
                return 1;
            return 0;
        }
        case 'r': return (r1==r2||c1==c2)&&pathClear(r1,c1,r2,c2);
        case 'b': return abs(dr)==abs(dc)&&pathClear(r1,c1,r2,c2);
        case 'q': return ((r1==r2||c1==c2)||abs(dr)==abs(dc))
                    &&pathClear(r1,c1,r2,c2);
        case 'n': return (abs(dr)==2&&abs(dc)==1)||(abs(dr)==1&&abs(dc)==2);
        case 'k': return abs(dr)<=1&&abs(dc)<=1;
    }
    return 0;
}
/* ---------- CHECK LOGIC ---------- */
void findKing(char k,int *r,int *c){
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++)
            if(board[i][j] && board[i][j]->type==k){
                *r=i; *c=j; return;
            }
}
int isInCheck(char k){
    int kr,kc; findKing(k,&kr,&kc);
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++)
            if(board[i][j] &&
               isWhite(board[i][j]->type)!=isWhite(k))
                if(validMove(i,j,kr,kc)) return 1;
    return 0;
}
int hasLegalMove(char side){
    for(int r1=0;r1<8;r1++)
        for(int c1=0;c1<8;c1++)
            if(board[r1][c1] &&
               ((side=='W'&&isWhite(board[r1][c1]->type))||
                (side=='B'&&!isWhite(board[r1][c1]->type))))
                for(int r2=0;r2<8;r2++)
                    for(int c2=0;c2<8;c2++)
                        if(validMove(r1,c1,r2,c2)) return 1;
    return 0;
}
int isStalemate(char side){
    char k=(side=='W')?'K':'k';
    return !isInCheck(k)&&!hasLegalMove(side);
}
/* ---------- HIGHLIGHT ---------- */
void clearHighlights(){
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++)
            highlight[i][j]=0;
}
void markLegalMoves(int r,int c){
    clearHighlights();
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++)
            if(validMove(r,c,i,j)) highlight[i][j]=1;
}
/* ---------- MOVE / UNDO ---------- */
void makeMove(int r1,int c1,int r2,int c2){
    history[moveCount]=(Move){r1,c1,r2,c2,board[r2][c2]};
    board[r2][c2]=board[r1][c1];
    board[r1][c1]=NULL;
    moveCount++; turn=(turn=='W')?'B':'W';
}
void undoMove(){
    if(!moveCount) return;
    Move m=history[--moveCount];
    board[m.r1][m.c1]=board[m.r2][m.c2];
    board[m.r2][m.c2]=m.captured;
    turn=(turn=='W')?'B':'W';
}
/* ---------- PRINT ---------- */
void printBoard(){
    printf("\n      a  b  c  d  e  f  g  h\n");
    printf("    +--------------------------------+\n");
    for(int i=0;i<8;i++){
        printf(" %d  ¦",8-i);
        for(int j=0;j<8;j++){
            if(highlight[i][j]) printf(HIGHLIGHT);
            else printf(((i+j)%2)?DARK:LIGHT);
            if(board[i][j]){
                char p=board[i][j]->type;
                printf(" %s%s%s ",
                    isWhite(p)?WHITE_P:BLACK_P,
                    symbol(p),RESET);
            } else printf("   ");
            printf(RESET);
        }
        printf("¦\n");
    }
    printf("    +--------------------------------+\n");
}
/* ---------- MAIN ---------- */
int main(){
    setlocale(LC_ALL,"");
    initBoard();
    printBoard();
    while(1){
        char a[10],b[10];
        printf("\n%s move (e2 e4 | undo): ",
               turn=='W'?"White":"Black");
        scanf("%s",a);
        if(!strcmp(a,"undo")){ undoMove(); printBoard(); continue; }
        scanf("%s",b);
        int r1,c1,r2,c2;
        chessToIndex(a,&r1,&c1);
        chessToIndex(b,&r2,&c2);
        if(!board[r1][c1]) continue;
        char p=board[r1][c1]->type;
        if(turn=='W' && !isWhite(p)) continue;
        if(turn=='B' && isWhite(p)) continue;
        if(validMove(r1,c1,r2,c2)){
            makeMove(r1,c1,r2,c2);
            printBoard();
            char enemy=(turn=='W')?'k':'K';
            if(isInCheck(enemy)){
                if(!hasLegalMove(turn)){
                    printf("\nCHECKMATE!\n"); break;
                } else printf("CHECK!\n");
            } else if(isStalemate(turn)){
                printf("\nSTALEMATE! DRAW.\n"); break;
            }
        }
    }
    return 0;
}
