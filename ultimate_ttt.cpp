#include "tictactoe.hpp"

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
                
                bool valid = 0;
                while(!valid){
                    cout<<"Enter row and column of big block: "<<endl;
                    cin>>big_row>>big_col;
                    if(big_row<0 || big_row>2 || big_col<0 || big_col>2){
                        cout<<"Invalid big block. Row and column must be between 0 and 2."<<endl;
                    }
                    else if(big_board.board_filled[big_row][big_col]){
                        cout<<"Invalid big block. This block is already won by "<<(big_board.board[big_row][big_col]?'X':'O')<<"."<<endl;
                    }
                    else if(!big_board.board[big_row][big_col] && !big_board.board_filled[big_row][big_col]){
                        cout<<"Invalid big block. This block is already drawn."<<endl;
                    }
                    else valid = 1;
                }
                cout<<"You chose to play in the big block ("<<big_row<<","<<big_col<<")"<<endl;
            }
            cout<<"Enter row and column of small block: "<<endl;
            cin>>row>>col;
            cout<<endl;
            if(small_boards[big_row][big_col].move(row, col, curr_player)){
                bool full_check [3][3] = {{1,1,1},{1,1,1},{1,1,1}};
                if(small_boards[big_row][big_col].game_over(row, col)){
                    big_board.board_filled[big_row][big_col] = 1;
                    big_board.move(big_row, big_col, curr_player);
                    if(big_board.game_over(big_row, big_col)){
                        cout<<(curr_player?'X':'O')<<" wins the game!"<<endl;
                        return;
                    }
                }
                else if(small_boards[big_row][big_col].board_filled ==  full_check){
                    //big_board.board_filled[big_row][big_col] = 0;
                    big_board.board[big_row][big_col] = 1;
                }
                curr_player = !curr_player;
                if(big_board.board_filled[row][col]== 0 && big_board.board[row][col]== 0 ){
                    big_block_fixed = 0;
                }
                else{
                big_row = row;
                big_col = col;
                big_block_fixed = 1;
                }
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
