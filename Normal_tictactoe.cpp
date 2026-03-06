//Variations: if you win a block, you get another turn vs you dont

#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <vector>
#include <bitset>
#include <queue>
#include <map>
#include <cstdlib> 
#include <ctime>
#include <cmath>
using namespace std;
struct ttt{
    bool board_filled [3][3];
    bool board [3][3];
    ttt() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                board_filled[i][j] = false;
                board[i][j] = false;
            }
        }
    }
    bool move(int row, int col, bool player) {
        if (row < 0 || row > 2 || col < 0 || col > 2) {
            cout << "Invalid move. Row and column must be between 0 and 2." << endl;
            return false;
        }
        if (board_filled[row][col]) {
            cout << "Invalid move. Cell is already filled." << endl;
            return false;
        }
        board_filled[row][col] = true;
        board[row][col] = player;
        return true;
    }
    void print_row(int row){
        for (int j = 0; j < 3; j++) {
            if (board_filled[row][j]) {
                cout << (board[row][j] ? "X" : "O");
            } else {
                cout << ".";
            }
            if(j<2) cout << "|";
        }
    }
    void print_board() {
        for (int i = 0; i < 3; i++) {
            print_row(i);
            if(i<2) cout << "\n-+-+-\n";
        }
        cout<<'\n'<<endl;
    }
    void play(){
        cout<<"Welcome to Tic Tac Toe!"<<endl;
        bool curr_player = 1;
        int total_moves = 0;
        bool draw =1;
        while(total_moves<9){
            int row, col;
            cout<<(curr_player?'X':'O')<<"'s turn| Enter row and column (0-2): "<<endl;
            cin>>row>>col;
            cout<<endl;
            if(move(row, col, curr_player)){
                print_board();
                curr_player = !curr_player;
                if(game_over(row, col) != 0) {
                    draw=0;
                    break;
                }
                total_moves++;
            }
        }
        cout<<"Game over!\n";
        if(draw){
            cout<<"It's a draw!"<<endl;
        }
        else{
            cout<<(curr_player?'O':'X')<<" wins!"<<endl;
        }
    }
    bool game_over(int row, int col) {
        // Check row
        if (board_filled[row][0] && board_filled[row][1] && board_filled[row][2] &&
            board[row][0] == board[row][1] && board[row][1] == board[row][2]) {
            return true;
        }
        // Check column
        if (board_filled[0][col] && board_filled[1][col] && board_filled[2][col] &&
            board[0][col] == board[1][col] && board[1][col] == board[2][col]) {
            return true;
        }
        // Check diagonals
        if (row == col) { // Top-left to bottom-right diagonal
            if (board_filled[0][0] && board_filled[1][1] && board_filled[2][2] &&
                board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
                return true;
            }
        }
        if (row + col == 2) { // Top-right to bottom-left diagonal
            if (board_filled[0][2] && board_filled[1][1] && board_filled[2][0] &&
                board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
                return true;
            }
        }
        return false;
    }
};
void startgame() {
    char choice;
    cout<<"Choose X or O: ";
    cin>>choice;
    if(choice == 'X' || choice == 'x') {
        cout<<"You chose X. You will go first."<<endl;
    } else if(choice == 'O' || choice == 'o') {
        cout<<"You chose O. You will go second."<<endl;
    } else {
        cout<<"Invalid choice. Please choose X or O."<<endl;
    }
}

struct ult_ttt{

    ttt big_board;
    ttt small_boards[3][3];
    void play(){
        cout<<"Welcome to Ultimate Tic Tac Toe!"<<endl;
        bool curr_player = 1;
        bool big_block_fixed = 0;
        int big_row, big_col;
        int total_moves = 0;
        bool draw =1;
        while(total_moves<81){
            int row, col;
            cout<<(curr_player?'X':'O')<<"'s turn\n";
            if(big_block_fixed){
                cout<<"You must play in the big block ("<<big_row<<","<<big_col<<")"<<endl;
            }
            else{
                cout<<"You can play in any big block."<<endl;
                cout<<"Enter row and column of big block: "<<endl;
                cin>>big_row>>big_col;
                cout<<"You chose to play in the big block ("<<big_row<<","<<big_col<<")"<<endl;
            }
            cout<<"Enter row and column of small block: "<<endl;
            cin>>row>>col;
            cout<<endl;
            if(small_boards[big_row][big_col].move(row, col, curr_player)){
                if(small_boards[big_row][big_col].game_over(row, col)){
                    big_board.move(big_row, big_col, curr_player);
                    if(big_board.game_over(big_row, big_col)){
                        draw=0;
                        break;
                    }
                }
                curr_player = !curr_player;
                big_row = row;
                big_col = col;
                big_block_fixed = 1;
                total_moves++;
            }
        }
    }
    void print_board(){
        for (int br = 0; br < 3; br++) {
            for (int r = 0; r < 3; r++) {
                for (int bc = 0; bc < 3; bc++){
                    if(big_board.board_filled[br][bc]){
                        if(big_board.board[br][bc]){
                            //X
                            if(r==0) cout<<"\\   /";
                            if(r==1) cout<<"  X  ";
                            if(r==2) cout<<"/   \\";
                        }
                        else{
                            //O
                            if(r==0) cout<<"  _  ";
                            // " / \\ "
                            if(r==1) cout<<"|   |";
                            if(r==2) cout<<" \\_/ ";
                        }
                    }
                    else small_boards[br][bc].print_row(r);
                    if(bc<2) cout << " || ";
                    
                }
                //if(r<2) cout<<"\n-+-+- || -+-+- || -+-+-\n";
                

                if(r<2){
                    cout<<'\n';
                    for (int bc = 0; bc < 3; bc++){
                        if(big_board.board_filled[br][bc]){
                            if(big_board.board[br][bc]){
                                //X
                                if(r==0) cout<<" \\ / ";
                                if(r==1) cout<<" / \\ ";
                            }
                            else{
                                //O
                                if(r==0) cout<<" / \\ ";
                                if(r==1) cout<<"\\   /";
                            }
                        }
                        else cout<<"-+-+-";
                        if(bc<2) cout << " || ";
                        
                    }
                    cout<<'\n';
                }
                
            }
            if(br<2) cout << "\n      ||       ||\n======++=======++=======\n      ||       ||\n";
        }
        cout<<'\n'<<endl;
    }
};

int main(){
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);
    ult_ttt game;
    game.big_board.move(0,0,1);
    game.big_board.move(1,1,0);
    game.print_board();
    cout<<""<<endl;
}
