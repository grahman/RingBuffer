//
//  rbuf.cpp
//  RingBuffer
//
//  Created by Graham Barab on 3/2/16.
//  Copyright © 2016 Graham Barab. All rights reserved.
//
//#define DEBUG

#include "rbuf.h"
#include <exception>
#include <stdexcept>
#include <cstdlib>
#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cerrno>
#endif
#ifdef __APPLE__
#include <mach/vm_map.h>
#include <mach/mach.h>
#endif
#ifdef DEBUG
#include <string.h>
#include <iostream>
#endif

#ifdef __linux__
static const char alphanum[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i' , 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

static std::string random_string(size_t length)
{
    size_t i;
    std::string str;
    
    str.push_back('/');
    for (i = 1; i < length; ++i) {
        str.push_back(alphanum[rand() % 62]);
    }
    
    return str;
}
#endif /* Linux */
static inline bool isPowerOf2(size_t x)
{
    size_t i;
    int bitset = 0;
    
    for (i = 0; i < sizeof(x) * 4; ++i) {
        if ((x & (0x1 << i) && bitset))
            return false;
        else if ((x & (0x1 << i) && !bitset))
            bitset = 1;
    }
    return true;
}
template <typename t>
Gmb::Rbuf<t>::Rbuf(size_t elements)
{
    size_t numBytes = elements * sizeof(t);
    unsigned long pageSize;
    size_t remainder;
    size_t numPages;
    size_t alloc_size;
    
    /* If sizeof(t) is not a power of 2, bail now, otherwise
     * this ring buffer will not fit an integral number of elements */
    if (!isPowerOf2(sizeof(t))) {
            throw std::invalid_argument("Gmb::Rbuf<t>::Rbuf(n) - Size of type not a power of 2");
    }
    addr = NULL; 

#ifdef __linux__
    pageSize = sysconf(_SC_PAGESIZE);
#endif
    
#ifdef __APPLE__
    pageSize = (unsigned long)getpagesize();
#endif
    numPages = numBytes / pageSize;
    remainder = numBytes % pageSize;
    alloc_size = ((numPages + ((remainder) ? 1 : 0)) * pageSize);
    
#ifdef __linux__
    std::string shm_name = random_string(64);
    int shm = shm_open(shm_name.c_str(), O_RDWR | O_CREAT, 0700);
    char *origAddress;
    char *remapStart;
    if (shm < 0) {
#ifdef DEBUG
    std::cerr << "shm_open() failed with error " << shm << " " << strerror(errno) <<   std::endl;
#endif
            throw std::exception();
    }
    /*  Must use ftruncate to allocate space for shared memory, otherwise will get sigbus */
    if(ftruncate(shm, alloc_size) < 0) {
#ifdef DEBUG
            std::cerr << "ftruncate() failed with error " << strerror(errno) << std::endl;
#endif
            throw std::exception();
    }
    /* Try to reserve a contiguous vm range in our va space */
    addr = (char *)mmap(NULL, alloc_size * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!addr || addr == MAP_FAILED) {
#ifdef DEBUG
    std::cerr << "mmap() failed (1) " << strerror(errno) <<  std::endl;
#endif
            throw std::exception();
    }
    /* Now that we know the starting address, deallocate it */
    if (munmap(addr, alloc_size * 2) < 0) {
#ifdef DEBUG
    std::cerr << "munmap() failed for addr " << std::hex << (unsigned long)addr;
    std::cerr << " and length " << std::dec << alloc_size * 2 << " " <<  strerror(errno) <<  std::endl;
#endif
            throw std::exception();
    }
    /* Now allocate the real shared memory buffers */
    origAddress = addr;
    addr = (char *)mmap(origAddress, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE | MAP_FIXED, shm, 0);
    if (!addr || addr == MAP_FAILED) {
#ifdef DEBUG
    std::cerr << "mmap() failed (2) " << strerror(errno) <<  std::endl;
#endif
            throw std::exception();
    }
    if (addr != origAddress) {
#ifdef DEBUG
    std::cerr << "mmap() returned a new address" << std::endl;
#endif
            throw std::exception();
    }
    
    /* Remap the second half of the contiguous vm region to same shared memory as first half */
    remapStart = addr + alloc_size;
    origAddress = remapStart;
    remapStart = (char *)mmap(remapStart, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE | MAP_FIXED, shm, 0);
    if (!remapStart || remapStart == MAP_FAILED) { 
#ifdef DEBUG
    std::cerr << "mmap() failed for virtual buffer" << std::endl;
#endif
            throw std::exception();
    }
    if (remapStart != origAddress) { 
#ifdef DEBUG
    std::cerr << "mmap() returned new address for virtual buffer" << std::endl;
#endif
            throw std::exception();
    }
  /* Try to minimize security risks by unlinking the "file" in /dev/shm, although this 
   * entire constructor has been one long race condition for Linux... */
    if(shm_unlink(shm_name.c_str()) < 0) {
#ifdef DEBUG
        std::cerr << "shm_unlink() failed with error " << strerror(errno) << std::endl;
#endif
        throw std::exception();
  }
    
#endif /* Linux */
    
#ifdef __APPLE__
    kern_return_t result = ERR_SUCCESS;
    vm_size_t memoryEntryLength;

        
    vm_address_t bufferAddress;
    vm_address_t origAddress;
    vm_address_t remapStart;
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
    
#endif /* Apple */
    
    _size = alloc_size;
    _head = addr;
    _tail = addr;
    _n = _size / sizeof(t);
    arr = (t *)addr;   /* For [] operator */
    _fillcount = 0;
}

template <typename t>
void Gmb::Rbuf<t>::consume(size_t elements)
{
    if (elements * sizeof(t) > _fillcount) {
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
void Gmb::Rbuf<t>::consumeZero(size_t elements)
{
    if (elements * sizeof(t) > _fillcount) {
        throw std::invalid_argument("Gmb::Rbuf::consume() - elements > fillcount");
    }
    size_t bytesConsumed = elements * sizeof(t);
    _fillcount -= bytesConsumed;
    memset(_tail, 0, bytesConsumed);
    
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
    if (munmap(addr, _size * 2) < 0) {
#ifdef DEBUG
    std::cerr << "munmap() failed for addr " << std::hex << (unsigned long)addr;
    std::cerr << " and length " << std::dec << _size * 2 << " " <<  strerror(errno) <<  std::endl;
#endif
            throw std::exception();
    }

#endif
}

template <typename t>
t& Gmb::Rbuf<t>::operator[](int i)
{
    if (i >= 0) {
        return arr[i % _n];
    } else {
        return arr[(i % _n) + _n];
    }
}

