//
//  rbuf.cpp
//  RingBuffer
//
//  Created by Graham Barab on 3/2/16.
//  Copyright © 2016 Graham Barab. All rights reserved.
//

#include "rbuf.h"
#include <exception>
#include <stdexcept>

#ifdef __APPLE__
#include <mach/vm_map.h>
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
    alloc_size = ((numPages + (remainder) ? 1 : 0) * pageSize);
   
    
#ifdef __linux__
    /* TODO: linux syscall for remapping vm */
#endif
    
#ifdef __APPLE__
    kern_return_t result = ERR_SUCCESS;
    vm_size_t memoryEntryLength;

        
    vm_address_t bufferAddress;
    vm_address_t origAddress;
    vm_address_t remapStart;
    vm_prot_t cur_prot, max_prot;
    mach_port_t memoryEntry;

    result = vm_allocate(mach_task_self(),
                         &bufferAddress,
                         alloc_size * 2,
                         VM_FLAGS_ANYWHERE);
    if ( result != ERR_SUCCESS ) {
            throw std::exception();
    }
    /* Deallocate now that we know the address of the area */
    result = vm_deallocate(mach_task_self(), bufferAddress, alloc_size * 2);
    if ( result != ERR_SUCCESS ) {
        throw std::exception();
    }
    
    /* Allocate real buffer */
    origAddress = bufferAddress;
    result = vm_allocate(mach_task_self(), &bufferAddress, alloc_size, FALSE);
    if ( result != ERR_SUCCESS ) {
        throw std::exception();
    }
    if (bufferAddress != origAddress) {
        throw std::exception();
    }
    memoryEntryLength = alloc_size;
    result = mach_make_memory_entry(mach_task_self(),
                                    &memoryEntryLength,
                                    bufferAddress,
                                    VM_PROT_READ | VM_PROT_WRITE,
                                    &memoryEntry,
                                    NULL);
    if (result != ERR_SUCCESS ) {
        throw std::exception();
    }
    if (!memoryEntry) {
        throw std::exception();
    }
    if (memoryEntryLength != alloc_size) {
        throw std::exception();
    }
    remapStart = bufferAddress + alloc_size;
    result = vm_map(mach_task_self(),
                    &remapStart,
                    alloc_size,
                    0,
                    FALSE,
                    memoryEntry,
                    0,
                    FALSE,
                    VM_PROT_READ | VM_PROT_WRITE,
                    VM_PROT_READ | VM_PROT_WRITE,
                    VM_INHERIT_DEFAULT);
    if (result != ERR_SUCCESS ) {
        throw std::exception();
    }
    if (remapStart != bufferAddress + alloc_size) {
        throw std::exception();
    }
    
    this->addr = (char *)bufferAddress;


    _size = alloc_size;
    _head = addr;
    _tail = addr;

    
#endif
    
    arr = (double *)addr;   /* For [] operator */
}

template <typename t>
void Gmb::Rbuf<t>::consume(size_t elements)
{
    if (elements * sizeof(t) > _size - _fillcount) {
        throw std::invalid_argument("Gmb::Rbuf::consume() - elements > fillcount");
    }
    size_t bytesConsumed = elements * sizeof(t);
    _fillcount -= bytesConsumed;
    
    if (_tail + bytesConsumed > addr + _size)
        _tail = addr + ((_tail - addr + bytesConsumed) % _size);
    else
        _tail += bytesConsumed;
}

template <typename t>
void Gmb::Rbuf<t>::produce(size_t elements)
{
    if (elements * sizeof(t) > _size - _fillcount) {
        throw std::invalid_argument("Gmb::Rbuf<t>::produce() - too many elements");
    }
    
    _fillcount += (elements * sizeof(t));
    if (_head + _fillcount > this->addr + _size)
        _head = (char *)(unsigned long)addr + ((_fillcount + (unsigned long)_head) % _size);
    else
        _head += _fillcount;
}

template <typename t>
Gmb::Rbuf<t>::~Rbuf()
{
#ifdef __APPLE__
    kern_return_t result;
    result = vm_deallocate(mach_task_self(), (vm_address_t)this->addr, this->_size * 2);
    if ( result != ERR_SUCCESS ) {
        throw std::exception();
    }
   
#endif
#ifdef __linux__
    /* TODO: linux dealloc implementation */
#endif
}

template <typename t>
t& Gmb::Rbuf<t>::operator[](int i)
{
    if (i >= 0) {
        return arr[i % _size];
    } else {
        return arr[(i % _size) + _size];
    }
}



