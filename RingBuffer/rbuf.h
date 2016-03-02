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
    template <typename t>
    class Rbuf {
    public:
        Rbuf(size_t elements);
        ~Rbuf();
        t *head() const;
        t *tail() const;
        void consume(size_t elements);
        void produce(size_t elements);
        size_t size() const;
        
        t &operator [](int i);
    private:
        t *addr;
        size_t _size;   /* In bytes */
    };
}

#endif /* rbuf_hpp */
