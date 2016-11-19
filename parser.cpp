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
	int oplen; //character length of the operand (used for sending operands)
	int optype; //the type (1 to 7) of operands the opcode has
	int lnum; //line number of command: this is address, not file line!
};

struct labeldata{
	char label[NUM_COLS];
	int line;
};

int fillfilelines(char filelines[][NUM_COLS], ifstream& asmin);
int removecruft(char filelines[][NUM_COLS], char dest[][NUM_COLS], int numlines);
int findlabel(char line[], char label[], labeldata* labelstruct);
int giveopval(char opcode[], int oplen);
int giveoptype(char operands[], int opval, opline* opdata);
int giveoperand(char segm[]);
bool checkopcode(char line[], opline* opdata);
bool validopt(int opval, int optype);
bool toupper(char srcline[], char destline[]);
bool nonvischar(char in);
bool alphachar(char in);
bool cmpop(char op1[], char op2[]);
//bool alphanumpunc(char in);
bool numchar(char in);
int wordcmp(char str1[], char str2[]);
int nocasewordcmp(char str1[], char str2[]);
int labelLookup(char label[], labeldata* labels[], int len);
int preparse(char filelines[][NUM_COLS], int linec);
bool isDupLabel(char label[], labeldata labels[], int len);
void dispStats(opline opdata[], int opcount);
void dispData(opline opcodes[], labeldata labels[], int opnum, int labnum);

//global variable for current file line; forgive me
int curfl = 0;

int main(int argc, char* argv[]){
	char filelines[1000][255];
	char codelines[1000][255];
	char ucodelines[1000][255];
	char labeltemp[NUM_COLS];
	opline allopcodes[1000];
	labeldata labels[250];
	int numlines1, numlines2, totalopcodes; //Number of lines at each stage
	int labelline, prevlabelline;
	int labellineoffset = 0, curopnum = 0, labelind = 0;
	bool foundcode = false, founddata = false, prevwaslabel;
	
	//First argument that is filename, next is actual input
	if(argc < 2){
		cerr << "Error: Not enough arguments" << endl;
		return -1;
	}
	else if(argc > 2){
		cerr << "Error: Too many arguments" << endl;
		return -1;
	}
		
	//Once this simulator is FULLY OPERATIONAL
	//We'll probably want to move all the file/parse stuff to a new function
	ifstream asmin;
	asmin.open(argv[1]);
	
	if(asmin.fail()){
		cerr << "Error: Invalid filename" << endl;
		return -1;
	}
	
	numlines1 = fillfilelines(filelines, asmin);
	numlines2 = removecruft(filelines, codelines, numlines1);


	//Super important: replaces [label] with the actual address
	preparse(codelines, numlines2);
	
	//cout << "With cruft removed: " << numlines2 << " lines" << endl;
	
	for(int i = 0; i < numlines2; i++){
		//cout << i << ':' << '\t' << codelines[i];
	}
	//cout << endl;
	
	//cout << "With unnecessary cases changed: " << endl;
	for(int i = 0; i < numlines2; i++){
		toupper(codelines[i], ucodelines[i]);
		//cerr << ucodelines[i];
	}
	
	
	//Get labels, opcodes, and operands
	for(int i = 0; i < numlines2; i++){
		curfl = i+1; //i is 0 indexed, the file line is 1 indexed
		//First, check for a label
		//labelline finds the line of the label, and stores them in the labels struct array

		//cout << "Label: " << labelline << "Prevlabel: " << prevlabelline << endl;
		//Have this to deal with unused lines
		if(codelines[i][0] == '#'){
			//cout << "Skipping non code line" << i << endl;
			continue;
		}
		
		cout << curfl << "\t";
		
		labelline = findlabel(codelines[i], labeltemp, &labels[labelind]);
		if(labelline >= 0){
			//check if we got the code label
			//cout << "Checking: " << labels[labelind].label << endl;
			if(isDupLabel(labels[labelind].label, labels, labelind-1)){
				cout << "Duplicate label was found" << endl;
				//return -1; //every duplicate needs to be caught
			}
			
			if(nocasewordcmp("CODE", labels[labelind].label) == 0){
			//handles special case of code label being found
				if(foundcode){
					cerr << "Error on line " << curfl <<  " duplicated Code directive label with value " << labelline << endl;
					 return -1;
				}
				//no value for code label
				else if(labelline == -2){	
				}
				else{
					foundcode = true;
					prevwaslabel = true;
					cout << "Code: " << labelline << endl;
					labellineoffset = 0; //reset the offset if we're in a new label
					labelind += 1;
				}
			}
			else if(nocasewordcmp("DATA", labels[labelind].label) == 0){
				//if it was already found
				if(founddata){
					cerr << "Error on line " << curfl << " duplicated Data directive label" << endl;
					return -1;
				}
				//no value for data label
				else if(labelline == - 2){	
				}
				else{
					founddata = true;
					prevwaslabel = true;
					//we don't reset the line offset for data, because it doesn't refer to code
					cout << "Data: " << labelline << endl;
					labelind += 1;
					labelline = prevlabelline; //do this as well since DATA label doesn't affect the code
				}
			}
			else{
				cout << labeltemp << ": " << labelline << i << endl;
				//cout << "Now in label " << labels[labelind].label << " at " << labels[labelind].line << endl;
				labellineoffset = 0; //reset the offset if we're in a new label
				labelind += 1;
			}
		}
		else if(labelline == -1){
			//Just means there wasn't a label, check for opcode
			if(checkopcode(ucodelines[i], &allopcodes[curopnum])){
				/*
				cout << "Found new opcode " << allopcodes[curopnum].opc;
				cout << " on line: " << prevlabelline+1 << endl;
				*/
				//get and set the operand type values
				//cout << "Calling giveoptype with: " << codelines[i]+allopcodes[curopnum].oplen << endl;
				//cout << "\t" << allopcodes[curopnum].opc;
				//cout << "\t" << allopcodes[curopnum].lnum << endl;
				//ensure that code directive has already appeared
				if(!foundcode){
					cerr << "Error on line " << curfl << ", code directive must come before any opcodes!" << endl;
					return -1;
				}
				prevwaslabel = false;
				allopcodes[curopnum].lnum = prevlabelline + 1;
				allopcodes[curopnum].optype = giveoptype(\
				(codelines[i]+allopcodes[curopnum].oplen), allopcodes[curopnum].opc,\
				&allopcodes[curopnum]);
				
				//cout << "line: " << curfl << " Found new opcode " << allopcodes[curopnum].opc;
				//cout << " at address: " << allopcodes[curopnum].lnum;
				//cout << " with type: " << allopcodes[curopnum].optype << endl;
				//cout << " with length: " << allopcodes[curopnum].oplen << endl;
				
				//iterate counters
				curopnum += 1;
				labelline = prevlabelline + 1;
				labellineoffset += 1;
			/*}else{
				//this is out here since after a label line with no value
				//checkopcode isn't going to be true
				labelline = prevlabelline; //no +1 here is a very subtle difference
				*///labellineoffset += 1; //removed this to get rid of extra line from label
			} //possibly deprecated by -2 return code
			
		}
		else if(labelline == -2){
			//We found a label, but it didn't have a specific line
			//Work off the distance from the known label line
			//This does rely on people AT LEAST giving the code label a line
			
			if(isDupLabel(labels[labelind].label, labels, labelind-1)){
				//return -1; //no fast fail here
			}
			labellineoffset += 0; //ASSUMING NEW LABEL DOESN'T CHANGE ADDRESS
			labelline = prevlabelline + 0;
			//if the previous was label, don't give this label new line
			labels[labelind].line = prevlabelline+0;
			//this may or may not apply, spec unclear
			if(!prevwaslabel){
				//labels[labelind].line++;
			}
			//cout << "Now in label " << labeltemp << " at " << labelline << endl;
			cout << labels[labelind].label << ": " << labels[labelind].line << endl;
			//<< " from line " << i << endl;
			labelind += 1;
			
		}
		else if(labelline == -3){
		}
		else if(labelline == -4){
		}
		else{
			//Should be only triggered by error
			cerr << "Error on line " << curfl << ", program exiting" << endl; 
			return -1;
		}
			//cout << prevlabelline << " = " << labelline;
			prevlabelline = labelline;
		
	}
	
	totalopcodes = curopnum; //need this to know how long opcode array is
	
	if(!foundcode){
		//the code label is meant to be before code
		cerr << "Error on line 1, no Code directive given" << endl;
		return -1;
	}
	
	//Calculate and print the stats about the program
	dispStats(allopcodes, totalopcodes);
	
	//Show data collected by program
	
	cout << endl << endl << "---------------------------------" << endl;
	dispData(allopcodes, labels, curopnum, labelind);
	
	
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

//Remove comments and whitespace, replaces them with sentinel
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
		else{
			//set sentinel for unused line, needed to count file lines
			dest[line][0] = '#';
			dest[line][1] = '\n';
			dest[line][2] = '\0';
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
	bool foundinvchar = false;
	//bool colfound = false;
	char foundnum[NUM_COLS];
	
	//Check that first char is OK
	if(!alphachar(line[0])){
		cerr << "Error on line " << curfl << " invalid label starting character: " << line[0] << endl;
		//return -100;
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
		//Apart for the colon, label characters can only be alphanumeric
		else if(!(alphachar(line[ind]) || numchar(line[ind]))){
			foundinvchar = true;
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
	if(foundinvchar){
		cerr << "Error on line " << curfl << " invalid character for label"\
			<< " '" << line[ind] << "'" << endl;
	}
	
	
	//No number after colon
	if(line[colloc+1] == '\0' || line[colloc+1] == '\n'){
		copy(label, label+colloc, labelstruct->label);
		//cout << endl << "Found label with no new address " << endl;
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
		
	//cout << endl << "Found label with address " << address << endl;
	
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
bool checkopcode(char* line, opline* opdata){
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
		//and a word tokenizer/splitter
		for(int ind = 0; ind < 5; ind++){
			if(line[ind] == '\0' || opcodes4[op][ind] == '\0'){
				//return false; //probably ended up here from early development, quite the bug
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
			//cout << "Matching 4 char opcode: " << endl;
			//get and set the opcode value
			opcval = giveopval(opcodes4[op], oplen);
			opdata->opc = opcval;
			opdata->oplen = oplen; //forgetting to add this caused quite the bugs
			/* this functionality moved to main
			//get and set the operand type values
			oprtype = giveoptype(line+oplen, opcval, opdata);
			opdata.optype = oprtype;
			*/
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
			opdata->opc = opcval;
			opdata->oplen = oplen; 
			/*
			//get and set the operand type values
			oprtype = giveoptype(line+oplen, opcval, opdata);
			opdata.optype = oprtype;
			*/
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
			opdata->opc = opcval;
			opdata->oplen = oplen;
			//get and set the operand type values
			/*
			oprtype = giveoptype(line+oplen, opcval, opdata);
			opdata.optype = oprtype;
			*/
			return true;
		}
	}	
	
	cerr << "Error on line " << curfl << " parsing opcode and operand" << endl;
	return false;
}

//given a set of characters, converts any abcz to ABCZ and puts it in array given by reference
bool toupper (char srcline[], char destline[]){
	int ind = 0;
	//cout << "src: " << srcline << endl;
	
	//cout << "noseg" << endl;
	while(srcline[ind] != '\0'){
		//cout << "seg" << srcline[ind] << endl;
		if(srcline[ind] >= 97 && srcline[ind] <= 122){
			destline[ind] = srcline[ind] - 32; //flip the 6th bit off to get UCASE
		}
		else{
			destline[ind] = srcline[ind];
		}
		ind++;
	}
	//Shouldn't be needed, but safety first
	srcline[ind] = '\0';
	//This is absolutely needed
	destline[ind] = '\0';
	
	return true;
}

//Return the numerical value of an opcode
int giveopval(char opcode[], int oplen){
	//cout << "giveopval sent oplen: " << oplen << endl;
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
				cerr << "Error on " << curfl << " Reached end of opcode values for valid opcode" << endl;
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
				cerr << "Error on " << curfl << " Reached end of opcode values for valid opcode" << endl;
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
				return 8;
			}
			else{
				cerr << "Error on " << curfl << " Reached end of opcode values for valid opcode" << endl;
				return -1;
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
int giveoptype(char operands[], int opval, opline* opdata){
	int retcode;
	int ind = 0, curopind = 0, oprind = 0;
	int opr[3];
	char curop[11];
	
	//cout << "Operands: " << operands << endl;
	
	//get the optype by checking each operand
	//while(operands[ind] != 0 && operands[ind] != '\n'){
	//cout << operands[ind] << " ";
	for(ind = 0; operands[ind] != '\0'; ind++){
		
		if(operands[ind] == ',' || operands[ind] == '\0' || operands[ind] == '\n'){
			//cout << "Encountered: " << int(operands[ind]) << endl;
			curop[curopind] = '\0';
			//cout << "Sending: " << curop << endl;
			
			//if(operands[ind] == ',' && !(numchar(operands[ind+1]) || alphachar(operands[ind+1]))){
			if(operands[ind] == ',' && operands[ind+1] == ','){
				cerr << "Error on line " << curfl << " expected operand between commas" << endl;
				return -1;
			}
			retcode = giveoperand(curop);
			//cout << "Was returned: " << retcode << endl;
			if(retcode == -1000){
				cerr << "Error on line " << curfl << " invalid operands for opcode;"\
				" perhaps extra, missing, duplicated, or invalid" << endl;
				return -1;
			}
			//cout << "Returned: " << retcode << endl;
			//cout << "Filling: " << oprind << endl;
			opr[oprind] = retcode; //set the operand type that was found
			//cout << "Increased oprind: " << oprind+1 << " " << oprind << endl;
			oprind++;
			curopind = 0;
		}
		else{
			//cout << "Putting " << operands[ind] << " in" << endl;
			curop[curopind] = operands[ind];
			curopind++;
		}	
		
		//Awful bug without below: doesn't 'cap' curop
		curop[curopind+1] = '\0';
		//cout << "Curop: " << curop << endl;
		
		//ind++;
	}
	//store the operand values in the struct array members
	//opdata->opd1 = opr[0]; opdata->opd2 = opr[1]; opdata->opd3 = opr[2];
	switch(oprind){
		case 3:
			opdata->opd3 = opr[2];
		case 2:
			opdata->opd2 = opr[1];
		case 1:
			opdata->opd1 = opr[0]; 
			break;
		case 0:
			cerr << "Error on line " << curfl << "invalid operand" << endl;
		default:
			//It can get here with too many inputs!
			cerr << "Error on line " << curfl << " extra operands"\
			<< " at address: " << opdata->lnum << ": " << (operands);
			return -1;
		
	}
	
	//GIANT SWITCHES/IFS HERE TO DETERMINE OPTYPE BY OPERANDS
	//Figure out what optype is present
	//AND if that optype is valid for the opcode
	
	//I get the distinct feeling this whole structure could be smaller
	//cout << opdata->lnum << " operands: " << opr[0] << " " << opr[1] << " " << opr[2] << endl;
	switch(oprind){
		//one argument: only JMP
		//type 7
		case 1:
			if(validopt(opval, 7)){
				return 7;
			}
			else{
				cerr << "Error on line " << curfl << " invalid operands for opcode"\
				<< " maybe missing, duplicated, or invalid" << endl;
				return -1;
			}
		//two arguments: LDx2, LDi, SDix2, SDx2, all the cond. jumps
		//type 1, 2, 3, or 4
		case 2:
			//type 1 or 3: operand 0 is numerical
			//Type 3: SDi: Operand 1 CAN be a negative number
			//Which really messes things up since we reserved those for ints
			//special check for SDi with negative
			if(opr[0] < 0 && opval == 200 && opr[1] > 0){
				return 3;
			}
			else if(opr[0] >= 0){
				//type 3: SDi: operand 1 numerical->ONLY POSITIVE HERE
				if(opr[1] >= 0){
					//cout << "Triggering for: " << opr[1] << endl;
					if(validopt(opval, 3)){
						return 3;
					}
					else{
						//It's kind of cludgy to state every type of operand error may have happened
						//But it gets the job done
						cerr << "Error on line " << curfl << " invalid operands for opcode"\
						<< " maybe missing, duplicated, or invalid" << endl;
						return -1;
					}
				}
				//type 1: LD, LDi, <\SDi>: operand 1 is register
				else{ //operand 1 is register
					//cout << "Register: " << opr[1] << endl;
					if(validopt(opval, 1)){
						return 1;
					}
					else{
						cerr << "Error on line " << curfl << " invalid operands for opcode"\
						<< " maybe missing, duplicated, or invalid" << endl;
						return -1;
					}
				}
			}
			//type 2 or 4: operand 0 is register
			else{
				//type 4: SD and cond. jumps (register, number)
				if(opr[1] >= 0){
					if(validopt(opval, 4)){
						return 4;
					}
					else{
						cerr << "Error on line " << curfl << " invalid operands for opcode"\
						<< " maybe missing, duplicated, or invalid" << endl;
						return -1;	
					}
				}
				//type 2: register, register: LD, SD
				else{
					//if(opr[1] < 0){
					
						if(validopt(opval, 2)){
							return 2;
						}
						else{
							cerr << "Error on line " << curfl << " invalid operands for opcode"\
							<< " maybe missing, duplicated, or invalid" << endl;
							return -1;
						}
					//}
				}
			}
		//three arguments: All ALU and ALUi
		//type 5 or 6: register, register/number, register
		case 3:
			//type 5: register, register, register
			if(opr[0] < 0 && opr[1] < 0 && opr[2] < 0){
				if(validopt(opval, 5)){
					return 5;
				}
				else{
					cerr << "Error on line " << curfl << " invalid operands for opcode"\
					<< " maybe missing, duplicated, or invalid" << endl;
					return -1;
				}
			}
			//type 6: reg, num, reg
			if(opr[0] < 0 && opr[1] >= 0 && opr[2] < 0){
				if(validopt(opval, 6)){
					return 6;
				}
				else{
					cerr << "Error on line " << curfl << " invalid operands for opcode"\
					<< " maybe missing, duplicated, or invalid" << endl;
					return -1;
				}
			}
			else{
				cerr << "Error on line " << curfl << " REALLY invalid operands for opcode\
						missing, extra, duplicated, or invalid?" << endl;
				return -1;
			}
			
		default:
			cerr << "Error: This should probably never occur" << endl;
			return -1;
	}
	
	/*
	//Register as operand 1
	if(opr[0] < 0){
		//Register as operand 1,2
		if(opr[1] < 0){
			//Register as operand 1,2,3
			if(opr[2] < 0 && oprind == 2 && validopt(opval, 5)){
				return 5;
			}
			//Register as operand 1,2
			else if(oprind == 1 && validopt(opval, 2)){
				return 2;
			}
			//Register as operands 1,2; number as 3
			else{
				cerr << "Expected register or EOL on line: " << opdata->lnum << endl;
				return -1;
			}
		}
		//Register as opr1, number as opr2
		else{
			//Register as 3
			if(opr[2] < 0 && oprind == 2 && validopt(opval, 6)){
				return 6;
			}
			//1,2
			else if(oprind == 1 && validopt(opval, 4)){
				return 4;
			}
			//register, number, number
			else{
				cerr << "Expected register or EOL on line: " << opdata->lnum << endl;
				return -1;
			}
		}
				
	}
	//number as opr1
	else{	
	}*/
	
	cerr << "Invalid operands given" << endl;
	return -1;
}

bool validopt(int opval, int optype){
	//precheck for too many operands
	if(opval < 0){
		cerr << "Error on line " << curfl << " invalid opcode value given" << endl;
		return false;
	}
	//all the opcodes except ALU and ALUi take max 2 arguments
	if((opval < 3 || opval > 7) && opval < 100){
		if(optype == 5 || optype == 6){
			cerr << "Error on line " << curfl << " extra operands" << endl;
		}
	}
	//unfortunately have to go the long way for proper errors
	//cout << "Called with opval: " << opval << " checking for optype: " << optype << endl; 
	switch(opval){
		case 1:
		
			if(optype == 1 || optype == 2){
				return true;
			}
			else{
				cerr << "Error: Expected <location, register> or <register, register> for LD" << endl;
				cerr << "Error on line " << curfl << " you are either missing or have extra operands" << endl;
				return false;
			}
		case 2:
			if(optype == 2 || optype == 4){
				return true;
			}
			else{
				cerr << "Error: Expected <register, register> or <register, location> for SD" << endl;
				return false;
			}
		//ADD, SUB, MUL, DIV all have same format of 3 reg
		case 3:
		case 4:
		case 5:
		case 6:
			if(optype == 5){
				//cout << "XCalled with " << opval << " " << optype << endl;
				return true;
			}
			else{
				//cout << "Called with " << opval << " " << optype << endl;
				cerr << "Error: Expected <register, register, register> for ALU opcode" << endl;
				return false;
			}
		case 7:
			if(optype == 7){
				return true;
			}
			else{
				cerr << "Error: Expected <location> for JMP" << endl;
				return false;
			}
		//JZ, JNZ, JGZ, JGEZ, JLZ, JLEZ all get covered here
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			if(optype == 4){
				return true;
			}
			else{
				cerr << "Error: Expected <register, location> for conditional jump opcode" << endl;
				return false;
			}
		case 100:
			if(optype == 1){
				return true;
			}
			else{
				cerr << "Error: Expected <location, register> for LDi" << endl;
				return false;
			}
		case 200:
			if(optype == 1 || optype == 3){
				return true;
			}
			else{
				cerr << "Error: Expected <number, register> or <number, location> for SDi" << endl;
				return false;
			}
		case 300:
		case 400:
		case 500:
		case 600:
			//cout << "ALUi called" << endl;
			if(optype == 6){
				return true;
			}
			else{
				cerr << "Error: Expected <register, number, register> for ALUi" << endl;
				return false;
			}
		default:
			cerr << "Invalid opcode value invoked" << endl;
			return false;
	}
}

//Finds a single operand (Register or number)
//The segm passed to this should be the chars between commas, including comma
//In the case of the last operand, the NUL replaces comma
int giveoperand(char segm[]){
	//1 digit register
	int regval;
	//cout << "Checking: " << segm << endl;
	//First check that register isn't too big, and that it has valid char after
	if(segm[0] == 'R' && numchar(segm[1]) && numchar(segm[3])){
		cerr << "Error on line " << curfl << " access to non-existent register" << endl;
	}
	//Need to check for two digits first, since a valid two digit is a valid one digit
	else if(segm[0] == 'R' && numchar(segm[1]) && numchar(segm[2])){
		if(segm[3] != '\n' && segm[3] != ',' && segm[3] != '\0'){
			cerr << "Error on line " << curfl << " bad register name" << endl;
		}
		regval = 10*(segm[1]-'0')+(segm[2]-'0');
		regval *= -1;
		//cout << "Found register__: " << regval << endl; 
		return regval;
	}
	//2 digit register
	else if(segm[0] == 'R' && numchar(segm[1])){
		if(segm[2] != '\n' && segm[2] != ',' && segm[2] != '\0'){
			cerr << "Error on line " << curfl << " bad register name" << endl;
		}
		regval = int(segm[1]-'0');
		regval *= -1;
		//cout << "Found register_: " << regval << endl; 
		return regval;
	}
	
	//Number operand
	//If we get a non numerical character here
	//There is no valid operand
	int ind = 0;
	char num[20];
	while(segm[ind] != '\0' && segm[ind] != ','){
		//cout << "char: " << segm[ind] << endl;
		if(numchar(segm[ind])){
			//cout << "Adding: " << segm[ind];
			num[ind] = segm[ind];
			ind++;
		}
		else if(segm[ind] == '-' && ind == 0){
			num[ind] = segm[ind];
			ind++;
		}
		else if (segm[ind] == '.'){
			cerr << "Error on line " << curfl << " extra precision numbers are not supported" << endl;
			return -1000;
		}
		else{
			cerr << "Error on line " << curfl <<  " invalid character for numerical operand (giveoperand)" << endl;
			return -1000;
		}
	}
	num[ind] = '\0';
	//cout << "Found number: " << atoi(num) << endl;
	//so since we used atoi, it seems like negative numbers have been handled
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
	if(in >= 65 && in <= 90){
		return true;
	}
	//Lowercase letter
	else if(in >= 97 && in <= 122){
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
	//str1 comes before alphabet than str2
	//Technically this covers \0 cases too
	else if(str1[0] < str2[0]){
		return 1;
	}
	//Covers the case of char1 > char2: first word is later in the alphabet
	else{
		return -1;
	}
}

//compares two words regardless of case
int nocasewordcmp(char str1[], char str2[]){
	char ustr1[NUM_COLS], ustr2[NUM_COLS];
	int ind1=0, ind2=0;
	
	//cout << "Given: " << str1 << "   " << str2 << endl;
	
	//segfaults for no reason
	//toupper(str1, ustr1);
	//toupper(str2, ustr2);
	while(str1[ind1] != '\0'){
		if(str1[ind1] >= 97 && str1[ind1] <= 122){
			ustr1[ind1] = str1[ind1] - 32; //flip the 6th bit off to get UCASE
		}
		else{
			ustr1[ind1] = str1[ind1];
		}
		ind1++;
	}
	ustr1[ind1] = '\0';
	
	while(str2[ind2] != '\0'){
		if(str2[ind2] >= 97 && str2[ind2] <= 122){
			ustr2[ind2] = str2[ind2] - 32; //flip the 6th bit off to get UCASE
		}
		else{
			ustr2[ind2] = str2[ind2];
		}
		ind2++;
	}
	ustr2[ind2] = '\0';
	
	//cout << "Comparing: " << ustr1 << " " << ustr2 << endl;
	
	if(str1[0]==0 && str2[0]==0){
		return 0;
	}
	if(str1[0] == str2[0]){
		//keep calling strCmp until one is different than the cover
		//cout << "cmpMatched: " << str1 << "   " << str2 << endl;
		return wordcmp(ustr1+1, ustr2+1); //this is trippy
	}
	//str1 comes before alphabet than str2
	//Technically this covers \0 cases too
	else if(str1[0] < str2[0]){
		return 1;
	}
	//Covers the case of char1 > char2: first word is later in the alphabet
	else{
		return -1;
	}
}

//Checks all the labels
//Error on ind repeats, invalid names, pointing to the same line
//Fill in line numbers of labels for the jmp commands
int preparse(char filelines[][NUM_COLS], int linec){
	//Can take skeleton structure of main to preparse label lines
	//Go through every line in advance and only use label functions from main
	//Once the address of every label has been found, they can be filled in
	//To there [...label...]s are
	//Technically label parsing runs twice, but my time is limited and CPU cycles are not
	
	//first task: find every instance of a label; figure out the line number
	//identifiers: first character of J; [ and then ] on same line
	
	//second task: check for duplicates, labels on adjacent lines
	//Could simply go through list and check each value for this
	
	//third task: replace every label that is given to a jump with its address
	
	//This is a stripped out copy of main, exists purely to preparse labels
	char labeltemp[NUM_COLS];
	//opline allopcodes[1000];
	labeldata labels[250];
	int numlines1, numlines2, totalopcodes; //Number of lines at each stage
	int labelline, prevlabelline;
	int labellineoffset = 0, curopnum = 0, labelind = 0;
	bool foundcode = false, founddata = false, prevwaslabel;
	for(int i = 0; i < numlines2; i++){
		curfl = i+1; //i is 0 indexed, the file line is 1 indexed
		//First, check for a label
		//labelline finds the line of the label, and stores them in the labels struct array

		//cout << "Label: " << labelline << "Prevlabel: " << prevlabelline << endl;
		//Have this to deal with unused lines
		if(filelines[i][0] == '#'){
			//cout << "Skipping non code line" << i << endl;
			continue;
		}
		
		cout << curfl << "\t";
		
		labelline = findlabel(filelines[i], labeltemp, &labels[labelind]);
		if(labelline >= 0){
			//check if we got the code label
			//cout << "Checking: " << labels[labelind].label << endl;
			if(isDupLabel(labels[labelind].label, labels, labelind-1)){
				cout << "Duplicate label was found" << endl;
				//return -1; //every duplicate needs to be caught
			}
			
			if(nocasewordcmp("CODE", labels[labelind].label) == 0){
			//handles special case of code label being found
				if(foundcode){
					cerr << "Error on line " << curfl <<  " duplicated Code directive label with value " << labelline << endl;
					 return -1;
				}
				//no value for code label
				else if(labelline == -2){	
				}
				else{
					foundcode = true;
					prevwaslabel = true;
					cout << "Code: " << labelline << endl;
					labellineoffset = 0; //reset the offset if we're in a new label
					labelind += 1;
				}
			}
			else if(nocasewordcmp("DATA", labels[labelind].label) == 0){
				//if it was already found
				if(founddata){
					cerr << "Error on line " << curfl << " duplicated Data directive label" << endl;
					return -1;
				}
				//no value for data label
				else if(labelline == - 2){	
				}
				else{
					founddata = true;
					prevwaslabel = true;
					//we don't reset the line offset for data, because it doesn't refer to code
					cout << "Data: " << labelline << endl;
					labelind += 1;
					labelline = prevlabelline; //do this as well since DATA label doesn't affect the code
				}
			}
			else{
				cout << labeltemp << ": " << labelline << i << endl;
				//cout << "Now in label " << labels[labelind].label << " at " << labels[labelind].line << endl;
				labellineoffset = 0; //reset the offset if we're in a new label
				labelind += 1;
			}
		}
		else if(labelline == -1){
			//Just means there wasn't a label, check for opcode
			if(checkopcode(ufilelines[i], &allopcodes[curopnum])){
				/*
				cout << "Found new opcode " << allopcodes[curopnum].opc;
				cout << " on line: " << prevlabelline+1 << endl;
				*/
				//get and set the operand type values
				//cout << "Calling giveoptype with: " << filelines[i]+allopcodes[curopnum].oplen << endl;
				//cout << "\t" << allopcodes[curopnum].opc;
				//cout << "\t" << allopcodes[curopnum].lnum << endl;
				//ensure that code directive has already appeared
				if(!foundcode){
					cerr << "Error on line " << curfl << ", code directive must come before any opcodes!" << endl;
					return -1;
				}
				prevwaslabel = false;
				allopcodes[curopnum].lnum = prevlabelline + 1;
				allopcodes[curopnum].optype = giveoptype(\
				(filelines[i]+allopcodes[curopnum].oplen), allopcodes[curopnum].opc,\
				&allopcodes[curopnum]);
				
				//cout << "line: " << curfl << " Found new opcode " << allopcodes[curopnum].opc;
				//cout << " at address: " << allopcodes[curopnum].lnum;
				//cout << " with type: " << allopcodes[curopnum].optype << endl;
				//cout << " with length: " << allopcodes[curopnum].oplen << endl;
				
				//iterate counters
				curopnum += 1;
				labelline = prevlabelline + 1;
				labellineoffset += 1;
			/*}else{
				//this is out here since after a label line with no value
				//checkopcode isn't going to be true
				labelline = prevlabelline; //no +1 here is a very subtle difference
				*///labellineoffset += 1; //removed this to get rid of extra line from label
			} //possibly deprecated by -2 return code
			
		}
		else if(labelline == -2){
			//We found a label, but it didn't have a specific line
			//Work off the distance from the known label line
			//This does rely on people AT LEAST giving the code label a line
			
			if(isDupLabel(labels[labelind].label, labels, labelind-1)){
				//return -1; //no fast fail here
			}
			labellineoffset += 0; //ASSUMING NEW LABEL DOESN'T CHANGE ADDRESS
			labelline = prevlabelline + 0;
			//if the previous was label, don't give this label new line
			labels[labelind].line = prevlabelline+0;
			//this may or may not apply, spec unclear
			if(!prevwaslabel){
				//labels[labelind].line++;
			}
			//cout << "Now in label " << labeltemp << " at " << labelline << endl;
			cout << labels[labelind].label << ": " << labels[labelind].line << endl;
			//<< " from line " << i << endl;
			labelind += 1;
			
		}
		else if(labelline == -3){
		}
		else if(labelline == -4){
		}
		else{
			//Should be only triggered by error
			cerr << "Error on line " << curfl << ", program exiting" << endl; 
			return -1;
		}
			//cout << prevlabelline << " = " << labelline;
			prevlabelline = labelline;
		
	}
	
	totalopcodes = curopnum; //need this to know how long opcode array is
	
	if(!foundcode){
		//the code label is meant to be before code
		cerr << "Error on line 1, no Code directive given" << endl;
		return -1;
	}
	
	//Calculate and print the stats about the program
	dispStats(allopcodes, totalopcodes);
	
	//Show data collected by program
	
	cout << endl << endl << "---------------------------------" << endl;
	dispData(allopcodes, labels, curopnum, labelind);
	
	
	curfl = 0; //quite important to reset file line counter!
	return 0;
	
}

//Returns the program line of a label
//Returns -1 if label DNE in array
//Returns -2 if label is duplicated
int labelLookup(char label[], labeldata* labels[], int numlabels){
	int line, cmp;
	enum lookupState{FOUND, DUP, NOTFOUND};
	lookupState state = NOTFOUND;
	
	for(int k = 0; k < numlabels; k++){
		cout << labels[k]->line << ": " << labels[k]->label << endl;
	}
	
	for(int i = 0; i < numlabels; i++){
		//cout << "s: " << state << endl;
		//cout << endl << "calling: " << i << ": " << label << "  " << labels[i]->label << endl;
		cmp = nocasewordcmp(label, labels[i]->label);
		//	cout << "cmp: " << cmp << endl;
		if (cmp == 0){
			if(state == FOUND){
				state = DUP;
				cerr << "Error on line " << curfl << "duplicate: " << label << "   " << labels[i]->label << endl;
				return -2;
			}
			//got very nasty bug from instinctively treating !FOUND like a boolean
			else if(state == NOTFOUND){
				state = FOUND;
				//cout << "Matched: " << label << "   " << labels[i]->label << endl;
				line = labels[i]->line;
			}
			else{
				cout << "How did we even end up here?" << endl;
			}
		}
		else{
			cout << "Did not match: " << label << "   " << labels[i]->label << endl;
		}
		cout << "s2: " << state << endl;
	}
	
	if(state == FOUND){
		return line;
	}
	else{
		return -1;
	}
}

//Called when a label is found to check for duplicates
bool isDupLabel(char label[], labeldata labels[], int numlabels){
	//cout << "Cur number of labels: " << numlabels << endl;
	int line, cmp;
	
	for(int k = 0; k < numlabels; k++){
		cout << labels[k].line << ": " << labels[k].label << endl;
	}
	
	for(int i = 0; i < numlabels; i++){
		//cout << "s: " << state << endl;
		//cout << endl << "calling: " << i << ": " << label << "  " << labels[i]->label << endl;
		cmp = nocasewordcmp(label, labels[i].label);
		//	cout << "cmp: " << cmp << endl;
		if (cmp == 0){
				cerr << "Error on line " << curfl << " duplicate label: " << label << "   " << labels[i].label << endl;
				return -2;
		}
	}
	
	return false;
}

//Prints out the stats about the program
void dispStats(opline opdata[], int opcount){
	int aluc = 0, cmpjmpc = 0, ldsdc = 0;
	int curopval;
	
	//for each opcode we have recorded
	for(int i=0; i < opcount; i++){
		//cout << "Opcode: " << opdata[i].opc << endl;
		curopval = opdata[i].opc;
		if(curopval==1 || curopval==2 || curopval==100 || curopval==200){
			ldsdc++;
		}
		else if((curopval >= 3 && curopval <= 6) || (curopval>=300&&curopval<=600)){
			//cout << "aluc++";
			aluc++;
		}
		else if((curopval >= 7 && curopval <= 13)){
			cmpjmpc++;
		}
		else{
			cout << "Invalid opcode found during counting" << curopval << endl;
		}
		
	}
	cout << endl;
	cout << "Total number of assembly instructions: " << aluc+cmpjmpc+ldsdc << endl;
	cout << "Number of Load/Store: " << ldsdc << endl;
	cout << "Number of ALU: " << aluc << endl;
	cout << "Number of Compare/Jump: " << cmpjmpc << endl;
	 
}

//Shows what data has been recorded for opcodes and structs
void dispData(opline opcodes[], labeldata labels[], int opnum, int labnum){
	for(int i = 0; i < opnum; i++){
		cout <<  opcodes[i].lnum << ": " << opcodes[i].opc << " " << opcodes[i].opd1 << " " << opcodes[i].opd2 << " " << opcodes[i].opd3 << " " << opcodes[i].optype << endl;
	}
	for(int k = 0; k < labnum; k++){
		cout << labels[k].line << ": " << labels[k].label << endl;
	}
}

/* √Can reprocess code into pure 2d array with multiple filters
 * √Commented line, whitespace line, etc
 * √Then for remaining lines strip out comments and leading/trailing whitespace
 * √Find code labels and get their addresses
 * √Find opcodes and and their operands
 * √Store it all in a nice clean struct array
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
 * 		√need to figure out operands and store them
 * 			mostly√check validity too
 * 			√first find out what OpType is happening and store that
 * 			√then find the actual number / register values and store those
 * 		√need to get current line number and store that with opdata
 * 
 * rest:
 * 		√get the statistics calculated and printed out
 * 
 * 
 */
