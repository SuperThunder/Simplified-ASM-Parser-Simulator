/*potential states:
 * skipping whitespace
 * skipping comment
 * looking for code label
 * looking for label/opcode
 * 		looking for operands if opcode
 * 		storing label info if label
*/
#include <iostream>
#include <fstream>
#include <cstdlib>

#define NUM_COLS 255

using namespace std;

struct opline{
	char opc[5]; //longest opcodes are 4 chars, + NUL terminator
	char opd1[11]; //Gives room for 10 digit number + NUL
	char opd2[11];
	char opd3[11];
	unsigned int lnum; //line number of command
};

struct labeldata{
	char label[NUM_COLS];
	int line;
};

int fillfilelines(char filelines[][255], ifstream& asmin);
int removecruft(char filelines[][NUM_COLS], char dest[][NUM_COLS], int numlines);
int findlabel(char line[], char label[]);
bool checkopcode(char line[], opline opdata);
bool nonvischar(char in);
bool alphachar(char in);
//bool alphanumpunc(char in);
bool numchar(char in);

int main(int argc, char* argv[]){
	char filelines[1000][255];
	char codelines[1000][255];
	char labeltemp[NUM_COLS];
	opline allopcodes[1000];
	//Number of lines at each stage
	int numlines1, numlines2, labelline, labellineoffset = 0, curopnum = 0;
	
	//One argument that is filename, next is actual input
	if(argc != 2){
		cerr << "Invalid input" << endl;
		return -1;
	}

	ifstream asmin;
	asmin.open(argv[1]);
	
	numlines1 = fillfilelines(filelines, asmin);
	numlines2 = removecruft(filelines, codelines, numlines1);
	
	cout << "With cruft removed: " << numlines2 << " lines" << endl;
	
	for(int i = 0; i < numlines2; i++){
		cout << i << ':' << '\t' << codelines[i];
	}
	cout << endl;
	
	for(int i = 0; i < numlines2; i++){
		//First, check for a label
		//TODO: Implement label struct array and copy labeltemp and line into an element
		labelline = findlabel(codelines[0], labeltemp);
		if(labelline >= 0){
			cout << "Now in label " << labeltemp << " at " << labelline << endl;
			labellineoffset = 0; //reset the offset if we're in a new label
		}
		else if(labelline == -1){
			//Just means there wasn't a label, check for opcode
			if(checkopcode(codelines[i], allopcodes[curopnum])){
				cout << "Found new opcode" << endl;
				curopnum += 1;
			}
		}
		else{
			//Should be only triggerred by error
			cerr << "Invalid code, program exiting" << endl;
			return -1;
		}
	}
	
	
	return 0;
}


//This _WORKS_!!!
//Now we don't need to worry about going through the file live
//Returns the number of lines in the original file
int fillfilelines(char filelines[][255], ifstream& asmin){
	int line = 0;// int col = 0;
	//char curchar;
	
	//We only get the line up to the newline here
	//The newlines get added back by the remove cruft function
	while(asmin.good() && !asmin.eof()){	
		char curline[255];
		asmin.getline(curline, 253, '\n');
		copy(curline, curline+253, filelines[line]);
		line++;
	}
	
	for(int i = 0; i < line; i++){
		cout << filelines[i] << endl;
	}
	
	
	return line;
}

//Remove comments and whitespace
//Returns number of lines in cleaned up array
int removecruft(char filelines[][NUM_COLS], char dest[][NUM_COLS], int numlines){
	int line = 0;
	
	//For each line
	for(int i = 0; i < numlines; i++){
		bool charadded = false; //keeps track of whether we used a line or not
		bool commented = false; //ignore everything after a #
		bool endofline = false; //if \n has been encountered
		int col = 0;
		//For each character
		for(int k = 0; k < NUM_COLS-1; k++){
			if(!endofline){
				//Check for controlish characters first
				//cout << filelines[i][k];
				switch(filelines[i][k]){
					case '\n':
						dest[line][col] = '\n'; //add the newline to end the char array
						dest[line][col+1] = '\0'; //add the needed NUl
						k = NUM_COLS-1; //this should work by itself, can maybe get rid of endofline var
						endofline = true;
						break;
					case '#':
						commented = true;
						break;
					//indicates line is done, should have gotten newline first probably
					case '\0':
						dest[line][col] = '\n'; //add the newline to end the char array
						dest[line][col+1] = '\0'; //add the needed NUl
						k = NUM_COLS-1; //this should work by itself, can maybe get rid of endofline var
						endofline = true;
						//cerr << endl << "Encountered unexpected EOS" << endl;
						//return false;
						break;
					default:
						break;
				}
				//If char isn't WS and we aren't after a comment
				if(!nonvischar(filelines[i][k]) && !commented && !endofline){
					dest[line][col] = filelines[i][k];
					charadded = true;
					col++;
				}
			}
		}
		if(charadded){
			line++;
		}
		
	}
	
	return line;
	
}

//Goes through the label until the colon, and tries to parse the address
//Returns address of label
//Returns -1 if no label found
//Returns -100 on error
int findlabel(char line[], char label[]){
	int ind = 0;
	int colloc, endloc, address;
	bool foundcol = false;
	//bool colfound = false;
	char foundnum[NUM_COLS];
	
	//Check that first char is OK
	if(!alphachar(line[0])){
		cout << "Invalid line starting character: " << line[0] << endl;
		return -100;
	}
	
	//Used to segfault here because of || rather than && for truth condition
	//Both of the delineating characters, to be safe
	while(line[ind] != '\n' && line[ind] != '\0'){
		//Find the index of the colon in the label char array
		if(line[ind] == ':'){
			colloc = ind;
			foundcol = true;
		}
		//if we encounter an invalid character
		/* every invalid character should already have been handled
		else if(!alphanumpunc(line[ind])){
			return -1;
		}*/
		ind++;
	}
	endloc = ind; //the location of last char will be last value of ind
	
	//If no colon is found, will happen for most (ie opcode) lines
	if(!foundcol){
		return -1;
	}
	
	
	cout << "Found colon at " << colloc << endl;
	
	//Get all the numeric characters into an array
	int numind = 0;
	for(int i = colloc+1; i < endloc; i++){
		if(numchar(line[i])){
			foundnum[numind] = line[i];
			//cout << "num character : " << foundnum[numind] << endl;
			numind++;
		}
		else{
			cerr << "Non numeric character found in label address: "\
			<< line[i] << endl;
			return -1;
			
		}
	}
	//Need to get the terminator in
	foundnum[numind] = '\0';
	
	//Convert the array into an int
	address = atoi(foundnum);
	
	cout << endl << "Found label with address " << address << endl;
	
	return address;
}

bool nonvischar(char in){
	if(in == '\t' || in == ' '){
		return true;
	}else{
		return false;
	}
}

bool checkopcode(char line[], opline opdata){
	cout << "one day" << endl;
}

//Returns if a character is valid for a label
bool labelchar(char in){
	int numchar = int(in);
	//The colon and hash and special characters for format
	if(numchar == ':' || numchar == '#'){
		return false;
	}
	//All the other alphanumberic and punctuation should be fine
	else if(numchar >= 33 && numchar <= 126){
		return true;
	}
	else{
		//This might happen if extended ascii or some weird char get put in
		cerr << "Invalid character with value " << numchar << endl;
		return false;
	}
}

//borrowed from robustAdd
bool numchar(char in){
	const int NUM_CHAR_NUM = 10;
	const char validchars[NUM_CHAR_NUM] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	
	for(int i = 0; i < NUM_CHAR_NUM; i++){
		if(in == validchars[i]){
			return true;
		}
	}

	return false;
}

bool alphachar(char in){
	//Capital letter
	if(in >= 65 || in <= 90){
		return true;
	}
	else if(in >= 97 || in <= 122){
		return true;
	}
	else{
		return false;
	}
}
/* √Can reprocess code into pure 2d array with multiple filters
 * √Commented line, whitespace line, etc
 * √Then for remaining lines strip out comments and leading/trailing whitespace
 * √Find code labels and get their addresses
 * Find opcodes and and their operands
 * Store it all in a nice clean struct array
 * Do the statistical analysis on number of each type
 * 
 * Keep track of:
 * Total number of asm instructions
 * Number of Load/Store
 * Number of ALU
 * Number of CMP/JMP
 */

/* Random planning:
 * Need to find labels first -> extract all labels out?
 * Code label has to be before code, but might have data labels before it
 * ̀Reading opcodes: Could read until R, number, square bracket
 * OR Check first 4 characters for code, then 3, then 2; error if not found
 * Data label last? hopefully won't show up in random bits of code, but that could be handled
 * 
 * for each clean line:
 * 		check if it's a label->need label name and associated number
 * 			set the line index to the label value, and index offset to 0
 * 		check what the opcode is
 * 			check the operands are valid
 * 			store the opcode, operands and line # in the struct array
 * 
 * opcodes:
 * 		need to figure out what the opcode is and store that
 * 		need to figure out operands and store them
 * 		need to get current line number and store that with opdata
 */
