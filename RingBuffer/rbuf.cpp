//
//  rbuf.cpp
//  RingBuffer
//
//  Created by Graham Barab on 3/2/16.
//  Copyright © 2016 Graham Barab. All rights reserved.
//

#include "rbuf.h"
#include <exception>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

template <typename t>
Gmb::Rbuf<t>::Rbuf(size_t elements)
{
    size_t numBytes = elements * sizeof(t);
    unsigned long pageSize;
    size_t remainder;
    size_t numPages;
    size_t alloc_size;
    
#ifdef __linux__
    pageSize = sysconf(_SC_PAGESIZE);
#endif
    
#ifdef __APPLE__
    pageSize = (unsigned long)getpagesize();
#endif
    numPages = numBytes / pageSize;
    remainder = numBytes % pageSize;
    alloc_size = (numPages * numBytes) + ((remainder) ? pageSize - remainder : 0);
    this->addr = (t *)malloc(alloc_size);
    if (!this->addr) {
        throw std::exception();
    }
    
#ifdef __linux__
    /* TODO: linux syscall for remapping vm */
#endif
    
#ifdef __APPLE__
    kern_return_t result;
    vm_address_t remapStart = this->addr + alloc_size;
    vm_prot_t cur_prot, max_prot;
    result = vm_remap(mach_task_self(),
                      remapStart,   // mirror target
                      alloc_size,	// size of mirror
                      0,				 // auto alignment
                      0,				 // force remapping to virtualAddress
                      mach_task_self(),  // same task
                      this->addr,	 // mirror source
                      0,				 // MAP READ-WRITE, NOT COPY
                      &cur_prot,		 // unused protection struct
                      &max_prot,		 // unused protection struct
                      VM_INHERIT_DEFAULT);
    if ( result != ERR_SUCCESS ) {
        throw std::exception();
    }

    
#endif
}

template <typename t>
Gmb::Rbuf<t>::~Rbuf()
{
#ifdef __APPLE__
    kern_return_t result;
    result = vm_deallocate(mach_task_self(), (vm_address_t)this->addr + this->_size, this->_size);
    if ( result != ERR_SUCCESS ) {
        throw std::exception();
    }
    free(this->addr);
#endif
#ifdef __linux__
    /* TODO: linux dealloc implementation */
#endif
}


