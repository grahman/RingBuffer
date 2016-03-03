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
    rbuf.produce(i);
    
    size = rbuf2.size();
    for (i = 0; i < size; ++i) {
        *head2++ = (double)(i + size);
    }

    rbuf2.produce(i);
    
    tail = rbuf.tail();
    tail2 = rbuf2.tail();
    for (i = 0; i < size * 2; ++i) {
        cout << *tail++ << endl;
        cout << *tail2++ << endl;
    }
    
    return 0;
}
