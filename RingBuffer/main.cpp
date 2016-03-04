//
//  main.cpp
//  RingBuffer
//
//  Created by Graham Barab on 3/2/16.
//  Copyright © 2016 Graham Barab. All rights reserved.
//

#include <iostream>
#ifdef __linux__
#include <ctime>
#endif
#include "rbuf.h"

using namespace std;

typedef struct BadStruct {
    char a;
    char b;
    char c;
} BadStruct;  /* sizeof(struct BadStruct) == 3 */

typedef struct GoodStruct {
    char a;
    char b;
    char c;
    char pad;
} GoodStruct;

int main(int argc, const char * argv[]) {

#ifdef __linux__
   srand(time(NULL));  /* On linux, seed random number generator before calling rbuf constructors */
#endif
    Gmb::Rbuf<double> rbuf(128);
    Gmb::Rbuf<double> rbuf2(128);
    double *head = rbuf.head();
    double *head2 = rbuf2.head();
    double *tail;
    double *tail2;
    size_t size = rbuf.size();
    unsigned long i;
   
    for (i = 0; i < size; ++i) {
        *head++ = (double)i;
    }
    rbuf.produce(i);    /* Move 'head' forward i elements */
    
    size = rbuf2.size();
    for (i = 0; i < size; ++i) {
        *head2++ = (double)(i + size);
    }

    rbuf2.produce(i);
    
    tail = rbuf.tail();
    tail2 = rbuf2.tail();
    
    /* Loop through 2x as many elements as "allowed" to prove that
     the shared memory trick worked */
    for (i = 0; i < size * 2; ++i) {
        cout << *tail++ << endl;
        cout << *tail2++ << endl;
    }

    rbuf.consume(i / 2);    /*  Move 'tail' forward i elements */
    rbuf2.consumeZero(i / 2);
    
    /* Test out the [] operator (does not depend on 'head' or 'tail') */
    for (i = 0; i < size * 2; ++i) {
            cout << rbuf[i] << endl; 
    }

    /* Now test error handling functionality */
    try {
            Gmb::Rbuf<BadStruct> badBuf(128);
    } catch (std::exception &exc) {
        cout << "Good, ring buffer allocation failed for structure whose size is not a power of 2" << endl;
    }

    try {
            Gmb::Rbuf<GoodStruct> goodBuf(128);
    } catch(std::exception &exc) {
        cerr << "Bad, ring buffer allocation raised an exception when it shouldn't have" << endl;
        return 1;
    }

    cout << "Good, ring buffer allocation succeeded for struct whose size is a power of 2" << endl;
    return 0;
}
