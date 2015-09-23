#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "dcomm.h"

using namespace std;

bool IsFull(QTYPE Q){
	return (Q.count == Q.maxsize);
}

int EmptySpace(QTYPE Q) {
	return Q.maxsize - Q.count;
}

void CreateQueue(QTYPE *Q, unsigned int size){
	(*Q).maxsize = size;
	(*Q).rear = -1;
	(*Q).front = 0; 
	(*Q).count = 0;
	(*Q).data = (Byte *) malloc (size * sizeof (Byte));
}

void Add(QTYPE *Q, Byte b){
	if ( (*Q).rear == (*Q).maxsize - 1 ) {
		(*Q).rear = 0;
	} else {
		(*Q).rear += 1;
	}
	(*Q).data[(*Q).rear] = b;
	(*Q).count++;
}

Byte Del(QTYPE *Q) {
	Byte temp = (*Q).data[(*Q).front];
	(*Q).data[(*Q).front] = 0x00000000;
	if ( (*Q).front == (*Q).maxsize - 1 ) {
		(*Q).front = 0;
	} else {
		(*Q).front += 1;
	}
	(*Q).count--;
	return temp;	
}

void ViewContent(QTYPE *Q) {
	int i;

	for ( i = 0; i <= (*Q).maxsize-1; i++ ) {
		cout << " * " << (*Q).data[i] << endl; 
	}

	cout << " [ Front: " << (*Q).front << " ] " << endl;
	cout << " [ Rear: " << (*Q).rear << " ] " << endl;
}
