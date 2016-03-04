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
    for (i = 0; i < size; ++i) {
        if (tail[i] != tail[size + i]) {
            cerr << "main(): Error -- second half of virtual memory region is not a mirror image of first half(1)." << endl;
            return 1;
        }
        if (tail2[i] != tail2[size + i]) {
            cerr << "main(): Error -- second half of virtual memory region is not a mirror image of first half(1)." << endl;
            return 1;
        }
    }

    cout << "main(): Virtual memory remap test: Passed!" << endl;

    rbuf.consume(i);        /*  Move 'tail' forward i elements */
    rbuf2.consumeZero(i);   /*  Same but also zero out the elements we just read for rbuf2 */

    tail2 = rbuf2.tail();
    double *iter = &rbuf2[0];
    while (iter != tail2) {
        if (*iter++) {
            cerr << "Main(): Gmb::Rbuf<t>::consumeZero test: Failed!" << endl;
            return 1;
        }
    }

    cout << "main(): Gmb::Rbuf<t>::consumeZero test: Passed!" << endl;
    
    /* Test out the [] operator (does not depend on 'head' or 'tail') */
    int j;
    for (j = 0; j > size * -1; --j) {
        if (rbuf[j] != rbuf[size + j - 1]) {
            cerr << "main(): Error, rbuf[" << j << "] != rbuf[" << size + j - 1 << "]" << endl;
            return 1;
        }
        if (rbuf2[j] != rbuf2[size + j - 1]) {
            cerr << "main(): Error, rbuf2[" << j << "] != rbuf2[" << size + j - 1 << "]" << endl;
            return 1;
        }
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
    cout << "----------------------------------" << endl;
    cout << "All tests passed!" << endl;
    return 0;
}
