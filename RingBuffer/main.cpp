//
//  main.cpp
//  RingBuffer
//
//  Created by Graham Barab on 3/2/16.
//  Copyright © 2016 Graham Barab. All rights reserved.
//

#include <iostream>
#include "rbuf.h"

using namespace std;
int main(int argc, const char * argv[]) {

    Gmb::Rbuf<double> rbuf(128);
    
    double *head = rbuf.head();
    double *tail;
    size_t size = rbuf.size();
    unsigned long i;
   
    for (i = 0; i < size; ++i) {
        *head++ = (double)i;
    }
#ifdef DEBUG
        cout << "made it past first for loop" << endl;
#endif
    
    rbuf.produce(i);
    
    tail = rbuf.tail();
    for (i = 0; i < size * 2; ++i) {
        cout << *tail++ << endl;
    }
    
    
    return 0;
}
