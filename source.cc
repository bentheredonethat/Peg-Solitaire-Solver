
/*
Author: Benjamin Levinsky
Program Name: Peg_solitaire_solver.cpp
Description: Program asks user for a board in the form of a text file in the same folder.
If the board is possible, then report sequence of moves to solve game.
Else the board is impossible to solve, then report lowest number of pegs left on board at end of game.

For a description of Peg Solitaire and an image of the board, see other attachment
*/


#include <iostream>
#include <vector>
#include <time.h> // includes time statistics
#include <stdio.h> // strtok() fn to parse a string
#include <string>
using namespace std;


// needed to avoid warning on strtok() function
#pragma warning(disable: 4996) 

// minimum number of pegs left on a board that is impossible to solve
int MinPegsonFailedBoard = 49;

// because a board only has 41 pegs, it is compactly stored as a 64 bit number.
// the first 49 bits represent the 7x7 layout of the pegs
// the corners are left on the board to simplify calculating row and column on a board
typedef unsigned long long board;

// macro that returns specific bit mask for given peg on board
#define PEG(number) ((long long) 1 << number) 

// move is represented by a start peg, the peg it will jump over and the final destination peg where it will land
// 2 bitmasks are used to expedite calculations
struct move{
	unsigned char start;
	unsigned char jump;
	unsigned char destination;

	// only start, jump and destination pegs are turned on
	// used to test if move is legal by first isolating 3 pegs by using bitwise & with board and then comparing with pattern
	// finally if found to be a legal then make resulting board after move by XOR'ing maskwith board
	board mask;

	// only start and jump pegs are on
	// if resulting board & mask match pattern then move is legal
	board pattern;
};

// hold all (76) possible moves on board
vector <struct move> LegalMoves;


// return true if only 1 peg left on board
bool  IsThisBoardSolved(board n){
	return !(n & (n - 1));
}

// return which row peg is in
int row(int position){
	return position / 7;
}

// return which column peg is in
int column(int position){
	// return what column peg is in
	return position % 7;
}

// return true if peg is outside bounds of board
bool iscorner(int position){
	return (((row(position) < 2) || row(position) > 4) && (column(position) < 2 || column(position) > 4));
}

// generate all possibles moves that are legal on a board
void GeneratePossibleMoves(vector <struct move> & PossibleMoves){

	// for every peg on the board, try each direction(up down left right) to destination peg
	// if jumping from arbitrary direction to destination is legal, then add to list of LegalMoves
	for (char destination = 0; destination < 49; destination++){

		// if destination is not in a corner, then further evaluate destination peg
		if (!(iscorner(destination))){

			// possible move
			struct move candidate = {};

			//see if we can jump to destination from left
			//start is destination - 2 and jump is dest. - 1
			candidate.destination = destination;
			candidate.start = destination - 2;
			candidate.jump = destination - 1;

			// if start is on same row as destination and start is not in corner then append to list of possible moves
			if (row(candidate.start) == row(candidate.destination) && !(iscorner(candidate.start))){
				candidate.mask = PEG(candidate.destination) | PEG(candidate.start) | PEG(candidate.jump);
				candidate.pattern = PEG(candidate.start) | PEG(candidate.jump);
				PossibleMoves.push_back(candidate);
			}
			//for dest. from right
			//start is dest. + 2 and jump is dest. + 1
			candidate.destination = destination;
			candidate.start = destination + 2;
			candidate.jump = destination + 1;

			// if start is on same row as destination and start is not in corner then append to list of possible moves
			if (row(candidate.start) == row(candidate.destination) && (!iscorner(candidate.start))){
				candidate.mask = PEG(candidate.destination) | PEG(candidate.start) | PEG(candidate.jump);
				candidate.pattern = PEG(candidate.start) | PEG(candidate.jump);
				PossibleMoves.push_back(candidate);
			}
			//for dest.up
			//start is dest. - 14 and jump is dest. - 7
			//if row(start) < 0 or corner(start) then not legal
			candidate.destination = destination;
			candidate.start = destination - 14;
			candidate.jump = destination - 7;
			if (column(candidate.start) > -1 && !iscorner(candidate.start)){
				candidate.mask = PEG(candidate.destination) | PEG(candidate.start) | PEG(candidate.jump);
				candidate.pattern = PEG(candidate.start) | PEG(candidate.jump);
				PossibleMoves.push_back(candidate);
			}
			//for dest.down
			//start is dest. + 14 and jump is dest. + 7
			//if row(start) > 6 or corner(start) then not legal
			candidate.destination = destination;
			candidate.start = destination + 14;
			candidate.jump = destination + 7;
			if (candidate.start < 49 && column(candidate.start) < 7 && !iscorner(candidate.start)) {
				candidate.mask = PEG(candidate.destination) | PEG(candidate.start) | PEG(candidate.jump);
				candidate.pattern = PEG(candidate.start) | PEG(candidate.jump);
				PossibleMoves.push_back(candidate);
			}
		}
	}
}

// collection of boards; this is used to track which boards have failed
class Open_Hash{

private:
#define BIGPRIME  123787     // size of hash table
	vector <board> OpenHashingTable[BIGPRIME]; // open hash table to keep track of boards that are impossible to solve
	int OH_EntriesAdded = 0;// global to keep track of entries in hash table
public:

	// return whether board key has already been added
	bool Search(board key){

		// determine which bucket to search for this board
		int bucket = key % BIGPRIME;

		//	loop through bucket for board to see if it has board
		for (unsigned int CurrentBoard = 0; CurrentBoard < OpenHashingTable[bucket].size(); CurrentBoard++){

			// if board is found then return true
			if (OpenHashingTable[bucket][CurrentBoard] == key) return true;
		}

		// board not found so return false
		return false;
	}

	// add board key to existing collection of boards
	void Add(board key){

		// determine which bucket to search for this board
		int bucket = key % BIGPRIME;

		// loop through bucket, if found then we are done
		for (unsigned int CurrentBoard = 0; CurrentBoard < OpenHashingTable[bucket].size(); CurrentBoard++)
			if (OpenHashingTable[bucket][CurrentBoard] == key) return;

		// because board was not found, append board to bucket
		OpenHashingTable[bucket].emplace_back(key);

		// update number of entries in table
		OH_EntriesAdded++;
	}
};

// recursively attempt to solve a board x. if solved, then return true and report list of moves that solve game
// if board is impossible to solve, return false and report lowest possible number of pegs left on board at end of game
bool OpenHashingSolve(board x, vector <struct move> & successfulmoves, Open_Hash & ImpossibleBoards) {

	// if the board is impossible to solve, report failure
	if (ImpossibleBoards.Search(x)) return false;

	// if board is solved (i.e. only one peg left on board), then report success
	if (IsThisBoardSolved(x)) return true;

	// loop through all possible moves 
	for (unsigned int Current_Possible_Move = 0; Current_Possible_Move < LegalMoves.size(); Current_Possible_Move++) {

		// if the move is legal and the board resulting from the current move is part of the solution,
		// then append to sequence of successful moves
		if ((x &  LegalMoves[Current_Possible_Move].mask) == LegalMoves[Current_Possible_Move].pattern
			&& OpenHashingSolve(x ^ LegalMoves[Current_Possible_Move].mask, successfulmoves, ImpossibleBoards)) {

			// reaching this point means solution was found using this move so append to list and report success
			successfulmoves.push_back(LegalMoves[Current_Possible_Move]);
			return true;
		}
	}

	// if board does not have any legal sequence of moves resulting in a solved board, add to table of impossible-to-solve boards
	ImpossibleBoards.Add(x);

	// lowest possible number of pegs left on current board
	unsigned int PegsOnBoard = 0;

	// loop to count number of pegs on board ( bits turned on)
	while (x){
		PegsOnBoard += x & 1;
		x >>= 1;
	}

	// if current board has lowest number of pegs currently found on board, update counter
	if (PegsOnBoard < MinPegsonFailedBoard) MinPegsonFailedBoard = PegsOnBoard;

	// current board is impossible
	return false;
}

// report to user time taken to solve board and sequence of moves to win game
// if board is impossible to solve, report lowest possible number of pegs left on board
void UI(board BoardtoAttempt){

	// record time to solve board
	double cpu_time_used;

	// list of moves from initial board configuration towards solution
	vector <struct move> WinningMoves;

	// collection of tables that do not have a solution
	Open_Hash ImpossibleBoardsTable;

	// time when method to solve board begins
	clock_t timeStart = clock();

	// if board can be solved, then report sequence of moves that will win the game
	if (OpenHashingSolve(BoardtoAttempt, WinningMoves, ImpossibleBoardsTable)){

		// time at end of attempt to solve board
		clock_t timeEnd = clock();

		// calculate time taken to solve board
		cpu_time_used = ((double)(timeEnd - timeStart)) / CLOCKS_PER_SEC;

		// loop to report sequence of moves
		for (unsigned int i = 0; i < WinningMoves.size(); i++)
			cout << "Move Number " << i << "  " << static_cast<int>(WinningMoves[i].start) << " " << static_cast<int>(WinningMoves[i].jump) << " " << static_cast<int>(WinningMoves[i].destination) << endl;
	}

	// no solution: report the least number of pegs left on board
	else{

		// report failure to user
		cout << "no solution\n minimum number of pegs left on board is " << MinPegsonFailedBoard << endl;

		// time at end of attempt to solve board
		clock_t timeEnd = clock();

		// calculate time taken to solve board
		cpu_time_used = ((float)(timeEnd - timeStart)) / CLOCKS_PER_SEC;
	}

	// report time taken to find solution to user
	cout << endl << cpu_time_used << " seconds" << endl << endl;

	return;
}

// solve board by parsing user-specified text file to board and then try to solve. report information about board
int main(int argc, char * argv[]){

	// read in string of text file holding peg locations to occupy on board
	string Text;
	cout << "Enter filename with .txt in name  ";
	getline(cin, Text);

	// will hold all pegs on board as specified by user
	board BoardtoAttempt = 0;

	// read from text file the list of pegs occupied and store in vector of boards
	FILE * f = fopen(Text.c_str(), "r");

	// current line of text file
	char line[256];

	// loop through lines of file
	while (fgets(line, 256, f)){

		// set a pointer to the beginning of the line
		char *wordsbeingparsed = line;

		// loop to read each number per line
		for (char *currentnumber = NULL; currentnumber = strtok(wordsbeingparsed, "  \n \t \r  "); wordsbeingparsed = NULL){

			// use bitwise OR to turn on bits of occupied pegs in board
			BoardtoAttempt = BoardtoAttempt | PEG(stoi(currentnumber));
		}
	}

	// calculate all possible moves 
	GeneratePossibleMoves(LegalMoves);

	// report information about board to user
	UI(BoardtoAttempt);

	// wait for user to exit
	cout << "Press any key to exit. \n";
	cin >> Text;

	return 0;
}