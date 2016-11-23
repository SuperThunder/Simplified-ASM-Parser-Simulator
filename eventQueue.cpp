#include "eventQueue.h"
#include <iostream>
using namespace std;

//this may not be working properly
bool alessb(float a, float b){
	float temp;
	float eps = 0.00001; //this should be precise enough
	temp = a-b;
	if(temp<0.0){temp*=-1.0;}
	//cout << "temp: " << temp << endl;
	cout << a << " less " << b << ": " << (temp > eps) << endl;
	return temp > eps;
}

//linked list of events
struct evl{
	Event ev;
	evl* next; //remember to initialize these to 0
};

//float nowtime; //technically we should keep track of this to avoid queueing events too early
evl* first = 0; //globally accesible first event
evl* last = first; //always the last new event added, will be empty

// Enqueue event e
// Return true if sucessful and false otherwise
//appends an item to the queue end
//events queued with same time: event added first stays first
bool enqueue(const Event* e){
	//handle negative time given
	if((e->time) < 0.0){
		cerr << "Invalid time" << endl;
		return false;
	}
	
	//first event is null pointer, so empty queue	
	//Allocate memory for first link
	if(first == 0){
		first = new evl;
		first->ev.time=e->time;
		first->ev.type=e->type;
		//cout << "First link: " << first->ev.time << " " << first->ev.type << endl;
		first->next = 0;
		last = first; //no idea why this is needed but by god it makes things work
	}
	else{
		//Set up the memory for the next event
		//These SHOULD be deallocated at the end
		evl* newlink = new evl;
		newlink->ev.time = e->time;
		newlink->ev.type = e->type;
		newlink->next = 0;
		//cout << "New link: " << newlink->ev.time << " " << newlink->ev.type << endl;
		last->next = newlink;
		//cout << "First pointer: " << first->next << endl;
		last = newlink;
	}
	
	//cout << "Queuing: " << e->time << endl;
	
	//cout << "First: " << first->ev.time << endl;
	//Set the next event to NULL so we know this is the lsat one
	
	return true;
	
}

// Dequeue
// Return the next event on the queue, ordered by time
// Lowest value of time is output first
// Return 0 if queue is empty
//pops the item with closest time off the queue
//no passing parameter, so I guess the queue is global
//return NUL is no more events, otherwise pointer to event at front (in terms of time)
const Event* dequeue(){
	if(first == 0){
		return 0;
	}
	//only one event, smallest by default
	else if(first == last){
		first = 0;
		//cout << "Returning: " << &last->ev.time;
		cout << "Smallest (and only): " << last->ev.time << endl;
		const Event* nomoreplease = new Event;
		nomoreplease = &last->ev;
		return nomoreplease;
	}
	
	evl* current;
	evl* prev = 0;
	evl* prevtosmallest = 0;
	evl* smallestevl = 0;
	
	current = first;
	//cout << "current link: " << current->ev.time << " " <<  current->ev.type << endl;
	smallestevl = first; //assume the first has the closest time as a reference
	
	//cout << "First: " << first->ev.time << endl;
	//Finding the link with smallest time
	//hopefully this much simplified sort works out
	int ind = 0;
	int smallind;
	prev = current;
	while(current != 0){
		if(current->ev.time < smallestevl->ev.time){
			smallestevl = current;
			prevtosmallest = prev;
			smallind = ind;
		}
		prev = current;
		current = current->next;
		ind++;
		
	}
	cout << "Smallest: " << smallestevl->ev.time << endl;
	//cout << "index: " << smallind << endl;
	//cout << "Number of links: " << ind << endl;
	/*
	while(current != 0){
		if(current->ev.time < smallestevl->ev.time){
		//if(alessb(current->ev.time, smallestevl->ev.time)){
			cout << "curtime: " << current->ev.time << endl;
			smallestevl = current;
			prevtosmallest = prev;
			cout << "Next from current: " << current->next << endl;
			current = current->next;
		}
		//current time not smaller than smallest time
		else{
			prev = current;
			cout << "Next from current: " << current->next << endl;
			current = current->next;
		}
		
	}*/
	cout << "Dequeing: " << smallestevl->ev.time << endl;
	//'remove' the current event by redoing the links
	//cout << "linking: " << prevtosmallest->ev.time << " to " << smallestevl->next->ev.time << endl;
	if(smallestevl == first){
		cout << "Setting FIRST to: " << first->next->ev.time << endl;
		first = first->next;
		//return &smallestevl->ev;
	}
	else if(smallestevl == last){
		//if the smallest is the last one
		//we need to set the second last next to 0
		//and the global last to second last
		current = first;
		//find the address before the last
		while(true){
			if(current->next == last){
				cout << "now: " << current->ev.time << endl;
				cout << "next: " << current->next->ev.time << endl;
				last = current;
				break;
			}
			else{
				current = current->next;
			}
		}
		last = current;
		last->next = 0;
		cout << "NEW LAST: " << last->ev.time << endl;
		//return &smallestevl->ev;
	}
	//this should handle all middle operations
	else{
		//cout << "Prev: " << prevtosmallest->ev.time << endl;
		cout << "REMOVING: " << smallestevl->ev.time << endl;
		prevtosmallest->next = smallestevl->next;
	} 
	
	const Event* retev = new Event;
	retev = &smallestevl->ev;
	
	return retev;
}

//NOTE: Wrong type of events may be being returned
/*
int main(){
	Event e;
	e.type=ALU;
	
	e.time=30.0;
	const Event ec = e;
	e.time=2750.5;
	const Event e2 = e;
	e.time = 3861;
	const Event e3 = e;
	e.type=LOADSTORE;
	e.time= 2704.98;
	const Event e4 = e;
	enqueue(&ec);
	//cout << "First link: " << first->ev.time << " " << first->ev.type << endl;
	enqueue(&e2);
	//cout << "First link: " << first->ev.time << " " << first->ev.type << endl;
	enqueue(&e3);
	enqueue(&e4);
	cout << endl;
	cout << "Dequeue 1: ";
	cout << dequeue()->time << endl;
	cout << "Dequeue 2: ";
	cout << dequeue()->time << endl;
	cout << "Dequeue 3: ";
	cout << dequeue()->time << endl;
	cout << "Dequeue 4: ";
	cout << dequeue()->time << endl;
	
	for(int i =100000; i >= 3; i--){
		e.time=float(i)*1.5;
		const Event ec = e;
		enqueue(&ec);
	}
	for(int i = 0; i < 10; i++){
		cout << dequeue()->time << endl;
	}
}*/

