/*
Open and read data from files, using C++, with appropriate error checking
Parse simplified assembly-language instructions
Compute basic statistical data about a file of simplified assembly
Develop basic test cases for your code
*/
//GAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHH

#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

//ifstream object needs to be passed by reference
bool parsef(ifstream& asmin); //pointer to filestream object given
bool validopc(char opc[]);
bool validreg(char reg[]);
bool validint(char chnum);
bool isws(char in);
bool numchar(char in);
bool signchar(char in);
bool letterchar(char in);
bool vischar(char in);
bool goodeol(char segm[]);
bool isvalidline(char line[]);
bool wordcmp(char word1[], char word2[]);
int parselabel(char line[], char label[]);
int findnextws(char segm[]);
int findnextchar(char segm[]);
int findnextcomma(char segm[]);
int findnextcolon(char segm[]);
int findeol(char line[]);
int findlastnum(char segm[]);
char* slice(char line[], int left, int right);
void fubar();

struct opline{
	//char opc[5]; //longest opcodes are 4 chars, + NUL terminator
	int opnum; //the integer value of the opcode
	//char opd1[11]; //Gives room for 10 digit number + NUL
	//char opd2[11];
	//char opd3[11];
	int opd1; //operands are a positive number if nnnn, or negative if register
	int opd2;
	int opd3;
	unsigned int lnum; //line number of command
};

int main(int argc, char* argv[]){
	//One argument that is filename, next is actual input
	if(argc != 2){
		cerr << "Invalid input" << endl;
		return -1;
	}

	
	ifstream asmin;
	
	//argv[1] should be the filename to open
	//todo: need to check if file exists
	asmin.open(argv[1]);
	if(!parsef(asmin)){
		return -1;
	}
	asmin.close();
}

bool parsef(ifstream& asmin){
	int numins, numldst, numalu, numcmpjmp; //numins = sum of each
	int linenum = 0, codestart, datastart, lind = 0, rind = 1;
	int curdynsize = 20; //initial number of lines to allocate memory for is 20
	bool validline = true, foundcodelbl = false;
	char curline[255], curseg[255];
	struct opline *oplines = NULL; //init pointer to dyn struct array to null
	oplines = new opline[curdynsize]; //start off with a default of 20 lines, can increase
	
	while(asmin.good()){
		//skipws(); //skips any leading wspace, might as well strip it out
		//get a max of 255 chars upto the next newline
		lind = 0;
		rind = 1;
		asmin.get(curline, 254, '\n');
		cout << linenum << ": " << curline << endl;
		copy(curline+lind, curline+254, curseg);
			
		validline = isvalidline(curline);	
		cout << "line is " << validline << endl;
		
		if(foundcodelbl == false && validline){
			cout << "Finding code label" << endl;
			codestart = parselabel(curline, "code");
			if(codestart < 0){
				cerr << "Invalid code label";
			}
			else{
				cout << "Found code label" << endl;
				//cout << curline << endl;
				foundcodelbl = true;
			}
		}
		
		//parse line if it's not commented out
		if(foundcodelbl && validline){
			//curseg = slice(curline, lind, rind);
			//find where chars actually begin
			
			lind = findnextchar(curseg);
			cout << "Finding left index: " << lind << endl;
			//Move curseg up the start
			copy(curline+lind, curline+rind, curseg);
			cout << curseg;
			rind = findnextws(curseg);
			cout << "Finding right index: " << rind << endl;
			
			//if lind or rind is 0, then there are no more commas or chars
			if(!(lind>0) || !(rind>0)){
				//do something, whether or not l/rind returning 0 means an error
				//depends on if we're reaching EOL with a valid line or not
				if(validline == false){
					cerr << "Invalid line" << endl;
					return false;
				}
			}
		
			copy(curline+lind, curline+rind, curseg);
			//even if an opcode is valid, it still needs to have valid operands
			if(validopc(curseg)){
				//oplines[linenum].opc = curseg;
				//rind-lind is a length of the current 'word'
				copy(curseg, curseg+(rind-lind), oplines[linenum].opc);	
			}
		}
		linenum += 1;
	}	
	cout << "fstream: " << (asmin.good()) << endl;
	//check if a file error happened
	if(asmin.bad()){
		cerr << "File error" << endl;
		return false;
	}
	
	cout << "Leaving parser" << endl;
	//at some point, memory given to oplines needs to be freed
	delete[] oplines;
	return true;
}

bool validopc(char opc[]){
	const int NUM_OPC = 19;
	int ind = 0;
	bool match = true;
	char opcodes[NUM_OPC][5]{"LD", "LDi", "SD", "SDi", "ADD", "ADDi",\
		"SUB", "SUBi", "MUL", "MULi", "DIV", "DIVi", "JMP", "JZ",\
		"JNZ", "JGZ", "JGEZ", "JLZ", "JLEZ"};
	
	for(int i =0; i < NUM_OPC; i++){
		while(opc[ind] != '\0'){
			if(opc[ind] != opcodes[NUM_OPC][ind]){
				match = false;
			}
			else{
				ind++;
			}
		}
		if(match == true){
			return true;
		}
	}
	
	return false;
}

bool validreg(char reg[]){
	int lind, rind;
	//char segm[20];
	
	lind = findnextchar(reg);
	
	//A valid register will be R and then some num 0-9
	if((reg[lind] == 'R') && numchar(reg[lind+1])){
		return true;
	}else{
		return false;
	}
		
	
}

//checks if a given int is valid
bool validint(char chnum[]){
	int ind = 0;
	
	//until we get to the terminator, check if any chars aren't numeric
	while(chnum[ind] != '\0'){
		if(!numchar(chnum[ind])){
			return false;
		}else{
			ind++;
		}
	}
	
	return true;
	
}

//borrowed from robustAdd
bool numchar(char in){
	const int NUM_CHAR_NUM = 10;
	const char validchars[NUM_CHAR_NUM] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	//bool valid = false;
	
	for(int i = 0; i < NUM_CHAR_NUM; i++){
		if(in == validchars[i]){
			return true;
		}
	}
	//If not a numeric character WILL return false
	//cerr << "Non numeric character" << endl;
	return false;
}

bool signchar(char in){
	const int SIGN_CHAR_NUM = 2;
	const char validchars[SIGN_CHAR_NUM] = {'+', '-'};
	
	for(int i = 0; i < SIGN_CHAR_NUM; i++){
		if(in == validchars[i]){
			return true;
		}
	}
	
	return false;
}

bool isws(char in){
	if(in == '\t' || in == ' '){
		return true;
	}else{
		return false;
	}
}

bool letterchar(char in){
	int numval = int(in);
	
	//Upper case letter
	if(numval >= 65 && numval <= 90){
		return true;
	}
	//Lower case letter
	else if(numval >= 97 && numval <= 122){
		return true;
	}
	else{
		return false;
	}
}

//Any kind of visible non-ws/non# char; letters, numbers, punctuation
bool vischar(char in){
	int numval = int(in);
	
	//35 is the # char for comments
	if(numval == 35){
		cerr << "Error: Comment encountered in code" << endl;
		return false;
	}
	else if(numval >= 33 && numval <= 126){
		return true;
	}
	else{
		return false;
	}
}

//Checks that line is not just comment or whitespace
bool isvalidline(char line[]){
	int ind = 0;
	bool allws = true;
	if(line[0] == '\n' || line[0] == '#'){
		return false;
	}
	
	while(line[ind] != '\0'){
		if(allws && (line[ind] == '#')){
			return false;
		}
		else if(!isws(line[ind])){
			return true;
		}
		else if(line[ind] == '\n'){
			return true;
		}
		else{
			ind++;
		}
	}
	
	cout << "Full whitespace line" << endl;
	return false;
	
}

//The location of code is guaranteed to be the first thing in the asm
int parselabel(char line[], char label[]){
	int lind, rind, num, eol;
	char segm[30], foundlabel[30];
	
	lind = findnextchar(line);
	rind = findnextcolon(line);
	
	//Check that the first char is a letter
	if(!letterchar(line[lind])){
		return -1;
	}
	
	//Check that from the first char to the colon only valid characters are there
	for(int i = lind+1; i < rind; i++){
		if(vischar(line[i])){
			return -1;
		}
		//if the label we're looking at doesn't match the one we want
		else if(line[i] != label[i]){
			return -1;
		}
	}
	
	copy(line+lind, line+rind, foundlabel);
	cout << "found label " << foundlabel << endl;
	
	//Create a new segment after the colon
	lind = rind+1;
	eol = findeol(line);
	copy(line+rind, line+eol, segm);
	//New right index is index of last number
	rind = findlastnum(segm);
	
	//After the colon, can only be valid numbers
	for(int i = lind; i <= rind; i++){
		if(!numchar(line[i])){
			return -1;
		}
	}
	copy(line+lind, line+rind, segm);
	num = atoi(segm);
	
	//return the actual number where code starts
	return num;
}

int findnextws(char segm[]){
	int ind = 0;
	while(!(segm[ind] == '\0')){
		if(isws(segm[ind])){
			return ind;
		}else{
			ind++;
		}
	}
	
	cout << "findnextws" << endl;
	fubar();
	return -1;
}

int findnextchar(char segm[]){
	int ind = 0;
	while(!(segm[ind] == '\0' || segm[ind] == '\n' || segm[ind] == '#')){
		if(!isws(segm[ind])){
			return ind;
		}else{
			ind++;
		}
	}
	
	cout << "findnextchar" << endl;
	fubar();
	return -1;
}

int findnextcomma(char segm[]){
	int ind = 0;
	while(!(segm[ind] == '\0' || segm[ind] == '\n' || segm[ind] == '#')){
		if(segm[ind] == ','){
			return ind;
		}else{
			cout << "No comma at " << ind << endl;
			ind++;
		}
	}
	
	cout << "findnextcomma" << endl;
	fubar();
	return -1;
}

int findnextcolon(char segm[]){
	int ind = 0;
	while(!(segm[ind] == '\0')){
		if(segm[ind] == ':'){
			return ind;
		}else{
			ind++;
		}
	}
	
}

int findeol(char line[]){
	int ind = 0;
	
	//Go until we encounter either a next line or EOS
	while(line[ind] != '\n' || line[ind] != '#' || line[ind] != '\0'){
		//We should encounter newline or comment before EOL
		if(line[ind] == '\0'){
			return -1;
		}
		ind++;
	}
	
	//Once '\n' or '#' is found, return the index
	return ind;
}

//Returns index of last numeric character found
int findlastnum(char segm[]){
	int ind = 0, numind = -1;
	
	while(segm[ind] != '\n' || segm[ind] != '#' || segm[ind] != '\0'){
		if(numchar(segm[ind])){
			numind = ind;
		}
	}
	
	return numind;
}

int findlastchar(char segm[]){
	int ind = 0;
	
	while(segm[ind] != '\n' || segm[ind] != '#' || segm[ind] != '\0'){
		
	}
}
//returns a subset of a string
char* slice(char line[], int left, int right){
	char segment[255];
	int ind = left;
	while(ind < right){
		segment[ind-left] = line[left];
	}
	
	return segment;
}

//for life's little surprises
void fubar(){
	for(int i = 0; i < 1; i++){
		cout << "~!@#$%^&*() THIS SHOULD NEVER HAPPEN <>?:{}|-=_+" << endl;
	}
}

/*Keep track of:
 * Total number of asm instructions
 * Number of Load/Store
 * Number of ALU
 * Number of CMP/JMP
 */
