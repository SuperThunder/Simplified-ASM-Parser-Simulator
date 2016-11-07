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
	//char opc[5]; //longest opcodes are 4 chars, + NUL terminator
	//char opd1[11]; //Gives room for 10 digit number + NUL
	//char opd2[11];
	//char opd3[11];
	int opc; //these are mapped to specific opcodes/operands
	int opd1; //See included text file
	int opd2;
	int opd3;
	int optype; //the type (1 to 7) of operands the opcode has
	unsigned int lnum; //line number of command
};

struct labeldata{
	char label[NUM_COLS];
	int line;
};

int fillfilelines(char filelines[][255], ifstream& asmin);
int removecruft(char filelines[][NUM_COLS], char dest[][NUM_COLS], int numlines);
int findlabel(char line[], char label[], labeldata* labelstruct);
int giveopval(char opcode[], int oplen);
int giveoptype(char operands[], int opval, opline &opdata);
int giveoperand(char segm[]);
bool checkopcode(char line[], opline &opdata);
bool toupper(char line[]);
bool nonvischar(char in);
bool alphachar(char in);
bool cmpop(char op1[], char op2[]);
//bool alphanumpunc(char in);
bool numchar(char in);
int wordcmp(char str1[], char str2[]);

int main(int argc, char* argv[]){
	char filelines[1000][255];
	char codelines[1000][255];
	char labeltemp[NUM_COLS];
	opline allopcodes[1000];
	labeldata labels[250];
	int numlines1, numlines2; //Number of lines at each stage
	int labelline, prevlabelline;
	int labellineoffset = 0, curopnum = 0, labelind = 0;
	
	//First argument that is filename, next is actual input
	if(argc != 2){
		cerr << "Invalid input" << endl;
		return -1;
	}

	ifstream asmin;
	asmin.open(argv[1]);
	
	numlines1 = fillfilelines(filelines, asmin);
	numlines2 = removecruft(filelines, codelines, numlines1);
	
	//Once this simulator is FULLY OPERATIONAL
	//We'll probably want to move all the file/parse stuff to a new function
	cout << "With cruft removed: " << numlines2 << " lines" << endl;
	
	for(int i = 0; i < numlines2; i++){
		cout << i << ':' << '\t' << codelines[i];
	}
	cout << endl;
	
	cout << "With unnecessary cases changed: " << endl;
	for(int i = 0; i < numlines2; i++){
		toupper(codelines[i]);
		cout << codelines[i];
	}
	
	
	//Get labels, opcodes, and operands
	for(int i = 0; i < numlines2; i++){
		//First, check for a label
		//labelline finds the line of the label, and stores them in the labels struct array

		//cout << "Label: " << labelline << "Prevlabel: " << prevlabelline << endl;
		labelline = findlabel(codelines[i], labeltemp, &labels[labelind]);
		if(labelline >= 0){
			cout << "Now in label " << labeltemp << " at " << labelline << endl;
			//cout << "Now in label " << labels[labelind].label << " at " << labels[labelind].line << endl;
			labellineoffset = 0; //reset the offset if we're in a new label
			labelind += 1;
		}
		else if(labelline == -1){
			//Just means there wasn't a label, check for opcode
			if(checkopcode(codelines[i], allopcodes[curopnum])){
				cout << "Found new opcode " << allopcodes[curopnum].opc;
				cout << " on line: " << prevlabelline+1 << endl;
				allopcodes[curopnum].lnum = prevlabelline + 1;
				curopnum += 1;
				labelline = prevlabelline + 1;
				labellineoffset += 1;
			}else{
				//this is out here since after a label line with no value
				//checkopcode isn't going to be true
				labelline = prevlabelline; //no +1 here is a very subtle difference
				labellineoffset += 1;
			}
			
		}
		else if(labelline == -2){
			//We found a label, but it didn't have a specific line
			//Work off the distance from the known label line
			//This does rely on people AT LEAST giving the code label a line
			labellineoffset += 1;
			labelline = prevlabelline + 1;
			labels[labelind].line = prevlabelline + 1;
			//cout << "Now in label " << labeltemp << " at " << labelline << endl;
			cout << "Now in label " << labels[labelind].label << " at " << labels[labelind].line << endl;
			labelind += 1;
			
		}
		else{
			//Should be only triggered by error
			cerr << "Invalid code in file, program exiting" << endl;
			return -1;
		}
			//cout << prevlabelline << " = " << labelline;
			prevlabelline = labelline;
		
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
		char curline[NUM_COLS];
		asmin.getline(curline, NUM_COLS-2, '\n');
		copy(curline, curline+NUM_COLS-2, filelines[line]);
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
//Also stores them in a given struct that will be part of array
//Returns address of label
//Returns -1 if no label found
//Returns -2 if a label is present, but has no specific number
//Returns -100 on error
//Valid label chars: alpha for first, pretty much anything except colon/ws after
int findlabel(char line[], char label[], labeldata* labelstruct){
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
			label[ind] = '\0'; //once colon is found, label name is over
		}
		if(!foundcol){
			label[ind] = line[ind]; //fill in the label name
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
	
	//No number after colon
	if(line[colloc+1] == '\0' || line[colloc+1] == '\n'){
		copy(label, label+colloc, labelstruct->label);
		cout << endl << "Found label with no new address " << endl;
		return -2;
	}
	
	//cout << "Found colon at " << colloc << endl;
	
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
	//Using atoi is sketchy, but we already checked the chars
	//And we're not going to have an address bigger than 2.4 billion
	address = atoi(foundnum);
	
	//Copy the found label and address into struct
	//Since labelstruct is a pointer to a struct, have to use special member operator
	copy(label, label+colloc, labelstruct->label);
	labelstruct->line = address;
		
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

/*In a given line, we expect an opcode to be there
* Might be 4, 3, or 2 chars
* After that, depending on the opcode, we expect certain 
* combinations of register/number
* Find the opcode, find if its operands are valid
* Opcode/operands then mapped to number values (giveopval function)
*/
bool checkopcode(char* line, opline &opdata){
	//cout << "one day" << endl;
	//All the opcodes stored by their character length
	int opcval = 0, oplen, oprtype;
	bool opmatch = true;
	char opcodes4[6][5]{"ADDI", "SUBI", "MULI", "DIVI", "JGEZ", "JLEZ"};
	char opcodes3[10][4]{"LDI", "SDI", "ADD", "SUB", "MUL", "DIV", "JMP", "JNZ", "JGZ", "JLZ"};
	char opcodes2[3][3]{"LD", "SD", "JZ"};
	
	//cout << "Line + 2: " << (line+2) << endl; //this SHOULD be perfectly valid
	//The line given should have no leading whitespace or extraneous characters
	//Check the 4 char opcodes first
	for(int op = 0; op < 6; op++){
		opmatch = true;
		//pretty much a bad word cmp, should write one
		for(int ind = 0; ind < 5; ind++){
			if(line[ind] == '\0' || opcodes4[op][ind] == '\0'){
				return false;
				break;
			}
			//if we get a non matching char
			if(!(line[ind] == opcodes4[op][ind])){
				opmatch = false;
				break;
			}
		}
		//if the opcode is still a match
		if(opmatch){
			oplen = 4;
			//get and set the opcode value
			opcval = giveopval(opcodes4[op], oplen);
			opdata.opc = opcval;
			//get and set the operand type values
			oprtype = giveoptype(line+oplen, opcval, opdata);
			opdata.optype = oprtype;
			return true; //at this point we should be all good for opcode value
		}
	}
	//Check the 3 char opcodes
	for(int op = 0; op < 10; op++){
		opmatch = true;
		for(int ind = 0; ind < 4; ind++){
			if(line[ind] == '\0' || opcodes3[op][ind] == '\0'){
				break;
			}
			//if we get a non matching char
			if(!(line[ind] == opcodes3[op][ind])){
				opmatch = false;
				break;
			}
		}
		//if the opcode is still a match
		if(opmatch){
			oplen = 3;
			opcval = giveopval(opcodes3[op], oplen);
			opdata.opc = opcval;
			//get and set the operand type values
			oprtype = giveoptype(line+oplen, opcval, opdata);
			opdata.optype = oprtype;
			return true;
		}
	}	
	
	//Check the 2 char opcodes
	for(int op = 0; op < 4; op++){
		opmatch = true;
		for(int ind = 0; ind < 3; ind++){
			if(line[ind] == '\0' || opcodes2[op][ind] == '\0'){
				break;
			}
			//if we get a non matching char
			if(!(line[ind] == opcodes2[op][ind])){
				opmatch = false;
				break;
			}
		}
		//if the opcode is still a match
		if(opmatch){
			oplen = 2;
			opcval = giveopval(opcodes2[op], oplen);
			opdata.opc = opcval;
			//get and set the operand type values
			oprtype = giveoptype(line+oplen, opcval, opdata);
			opdata.optype = oprtype;
			return true;
		}
	}	
	
	cerr << "Error parsing opcodes and operands" << endl;
	return false;
}

//given a set of characters by reference, converts any abcz to ABCZ
bool toupper (char line[]){
	int ind = 0;
	
	while(line[ind] != '\0'){
		if(line[ind] >= 97 && line[ind] <= 122){
			line[ind] -= 32; //flip the 6th bit off to get UCASE
		}
		ind++;
	}
	
	return true;
}

//Return the numerical value of an opcode
int giveopval(char opcode[], int oplen){
	switch(oplen){
		case 4:
			if(cmpop(opcode, "ADDI")){
				return 300;
			}
			else if(cmpop(opcode, "SUBI")){
				return 400;
			}
			else if(cmpop(opcode, "MULI")){
				return 500;
			}
			else if(cmpop(opcode, "DIVI")){
				return 600;
			}
			else if(cmpop(opcode, "JGEZ")){
				return 11;
			}
			else if(cmpop(opcode, "JLEZ")){
				return 13;
			}
			else{
				cerr << "Error: Reached end of opcode values for valid opcode" << endl;
				return -1;
			}
			break;
			
		case 3:
			if(cmpop(opcode, "LDI")){
				return 100;
			}
			else if(cmpop(opcode, "SDI")){
				return 200;
			}
			else if(cmpop(opcode, "ADD")){
				return 3;
			}
			else if(cmpop(opcode, "SUB")){
				return 4;
			}
			else if(cmpop(opcode, "MUL")){
				return 5;
			}
			else if(cmpop(opcode, "DIV")){
				return 6;
			}
			else if(cmpop(opcode, "JMP")){
				return 7;
			}
			else if(cmpop(opcode, "JNZ")){
				return 8;
			}
			else if(cmpop(opcode, "JGZ")){
				return 10;
			}
			else if(cmpop(opcode, "JLZ")){
				return 12;
			}
			else{
				cerr << "Error: Reached end of opcode values for valid opcode" << endl;
				return -1;
			}
			break;
			
		case 2:
			if(cmpop(opcode, "LD")){
				return 1;
			}
			else if(cmpop(opcode, "SD")){
				return 2;
			}
			else if(cmpop(opcode, "JZ")){
				return 7;
			}
			else{
				cerr << "Error: Reached end of opcode values for valid opcode" << endl;
			}
			break;
			
		default:
			cerr << "Invalid call to operator value assignment" << endl;
			return -1;
			break;
	}
}

//Give the numerical value of an opcode's operand types
//Note that they might not be valid for the opcode on the line 
//				- that needs to be checked
//Opval -> possible operands -> error or return Optype
int giveoptype(char operands[], int opval, opline &opdata){
	int comloc, retcode;
	int ind = 0, curopind = 0, oprind = 0;
	int opr[3];
	char curop[11];
	//cout << operands;
	
	//get the optype by checking each operand
	while(operands[ind] != 0 && operands[ind] != '\n'){
		//cout << operands[ind] << " ";

		if(operands[ind] == ','){
			curop[curopind] = '\0';
			//cout << "Sending: " << curop << endl;
			retcode = giveoperand(curop);
			if(retcode == -1000){
				cerr << "Error: Invalid operands" << endl;
				return -1;
			}
			opr[oprind] = retcode; //set the operand type that was found
			oprind++;
			curopind = 0;
			ind++;
		}
		else{
			//cout << "Putting " << operands[ind] << " in" << endl;
			curop[curopind] = operands[ind];
			curopind++;
			ind++;
		}	
	}
	
	//GIANT SWITCHES HERE TO DETERMINE OPTYPE BY OPERANDS
	
	return 1;
	
	
	cerr << "Invalid operands given" << endl;
	return -1;
}

//Finds a single operand (Register or number)
//The segm passed to this should be the chars between commas, including comma
//In the case of the last operand, the NUL replaces comma
int giveoperand(char segm[]){
	//1 digit register
	int regval;
	//cout << "Checking: " << segm << endl;
	//Need to check for two digits first, since a valid two digit is a valid one digit
	if(segm[0] == 'R' && numchar(segm[1]) && numchar(segm[2])){
		regval = 10*(segm[1]-'0')+(segm[2]-'0');
		cout << "Found register__: " << regval << endl; 
		return regval*-1;
	}
	//2 digit register
	else if(segm[0] == 'R' && numchar(segm[1])){
		regval = int(segm[1]-'0');
		//cout << "Found register_: " << regval << endl; 
		return regval*-1;
	}
	
	//Number operand
	//If we get a none numerical character here
	//There is no valid operand
	int ind = 0;
	char num[20];
	while(segm[ind] != '\0' && segm[ind] != ','){
		if(numchar(segm[ind])){
			//cout << "Adding: " << segm[ind];
			num[ind] = segm[ind];
			ind++;
		}
		else{
			cerr << "Error: Invalid character for numerical operand" << endl;
			return -1000;
		}
	}
	num[ind] = '\0';
	//cout << "Found number: " << atoi(num) << endl;
	return atoi(num);
	
	cerr << "How did this even happen?" << endl;
	return -1000;

}

bool cmpop(char op1[], char op2[]){
	//if the opcodes are the same character array
	if(wordcmp(op1, op2) == 0){
		return true;
	}
	else{
		return false;
	}
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


int wordcmp(char str1[], char str2[]){
	//cout << "SEGSEGSEG" << endl;	
	//cout << str1[0] << "\t" << str2[0] << endl;
	if(str1[0]==0 && str2[0]==0){
		return 0;
	}
	if(str1[0] == str2[0]){
		//keep calling strCmp until one is different than the cover
		return wordcmp(str1+1, str2+1); //this is trippy
	}
	//str1 comes before alphabet than string2
	//Technically this covers \0 cases too
	else if(str1[0] < str2[0]){
		return 1;
	}
	//Covers the case of char1 > char2: first word is later in the alphabet
	else{
		return -1;
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
 * weird requirement: case insensitive
 * opcodes:
 * 		√need to figure out what the opcode is and store that
 * 		need to figure out operands and store them
 * 			check validity too
 * 			first find out what OpType is happening and store that
 * 			then find the actual number / register values and store those
 * 		√need to get current line number and store that with opdata
 * 
 * rest:
 * 		get the statistics calculated and printed out
 */
