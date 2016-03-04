//
//  rbuf.hpp
//  RingBuffer
//
//  Created by Graham Barab on 3/2/16.
//  Copyright © 2016 Graham Barab. All rights reserved.
//

#ifndef rbuf_hpp
#define rbuf_hpp

#include <cstdlib>

namespace Gmb {
    /* Note: If this is compiled on Linux, it is extremely important to seed
     * the random number generator with srand() prior to calling the constructors.
     * If this is not done, the likelihood of one process that uses this class 
     * corrupting the ring buffers of another process that uses this class 
     * becomes MUCH more likely. This is due to the fact that Linux uses the filesystem
     * abstraction to manage shared memory, so "filenames" must be as random as possible.
     * If the rnd number generator is seeded, the temporary filename will be one out
     * of 64^62 possibilities. */
    template <class t>
    class Rbuf {
    public:
        Rbuf(size_t minimum_elements);  /* Actual size will be rounded to up to multpiles of page size */
        ~Rbuf();
        t *head() const {return (t *)_head;};
        t *tail() const {return (t *)_tail;};
        void consume(size_t elements);  /* Moves tail forward by specified number of elements */
        void consumeZero(size_t elements);  /* Moves tail forward by specified number of elements and zeroes them out */
        void produce(size_t elements);  /* Moves head forward by specified number of elements */
        size_t size() const {return _n;};       /* Number of elements ring buffer can accomodate */
        size_t freeSpace() const {return (_size - _fillcount) / sizeof(t);};
        t &operator [](int i);          /* Does not depend on position of head or tail */
    private:
        char *addr;
        char *_head;
        char *_tail;
        t *arr;                 /* Just a pointer to addr */
        size_t _size;           /* In bytes */
        size_t _n;              /*  _size / sizeof(t) */
        size_t _fillcount;      /* In bytes */
    };
}

#include "rbuf.hpp"

#endif /* rbuf_hpp */
