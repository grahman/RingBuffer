//
//  rbuf.hpp
//  RingBuffer
//
//  Created by Graham Barab on 3/2/16.
//  Copyright © 2016 Graham Barab. All rights reserved.
//

#ifndef rbuf_hpp
#define rbuf_hpp

#define CPU64

#include <cstdlib>

namespace Gmb {
    template <class t>
    class Rbuf {
    public:
        Rbuf(size_t elements);
        ~Rbuf();
        t *head() const {return (t *)_head;};
        t *tail() const {return (t *)_tail;};
        void consume(size_t elements);
        void produce(size_t elements);
        size_t size() const {return _size / sizeof(t);};
        size_t freeSpace() const {return (_size - _fillcount) / sizeof(t);};
        
        t &operator [](int i);
    private:
        char *addr;
        char *_head;
        char *_tail;
        t *arr;                 /* Just a pointer to addr */
        size_t _size;           /* In bytes */
        size_t _fillcount;      /* In bytes */
    };
}

#include "rbuf.hpp"

#endif /* rbuf_hpp */
