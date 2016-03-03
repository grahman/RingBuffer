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
#include <unistd.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef __APPLE__
#include <mach/vm_map.h>
#include <mach/mach.h>
#endif
#ifdef DEBUG
#include <string.h>
#include <cerrno>
#include <iostream>
#endif

template <typename t>
Gmb::Rbuf<t>::Rbuf(size_t elements)
{
    size_t numBytes = elements * sizeof(t);
    unsigned long pageSize;
    size_t remainder;
    size_t numPages;
    size_t alloc_size;
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
#ifdef DEBUG
    std::cout << "alloc_size is " << alloc_size << std::endl;
#endif
    
#ifdef __linux__
    int shm = shm_open("/rbuf", O_RDWR | O_CREAT, 0777);
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
    addr = (char *)mmap(NULL, alloc_size * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!addr || addr == MAP_FAILED) {
#ifdef DEBUG
    std::cerr << "mmap() failed (1) " << strerror(errno) <<  std::endl;
#endif
            throw std::exception();
    }
    if (munmap(addr, alloc_size * 2) < 0) {
#ifdef DEBUG
    std::cerr << "munmap() failed for addr " << std::hex << (unsigned long)addr;
    std::cerr << " and length " << std::dec << alloc_size * 2 << " " <<  strerror(errno) <<  std::endl;
#endif
            throw std::exception();
    }
    /* Now allocate the real buffers */
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
    
#endif /* Linux */
    
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
    
#endif /* Apple */
    
    _size = alloc_size;
    _head = addr;
    _tail = addr;
    arr = (double *)addr;   /* For [] operator */
#ifdef DEBUG
  std::cout << "head is " << std::hex << _head << std::endl;
#endif
  if(shm_unlink("/rbuf") < 0) {
#ifdef DEBUG
        std::cerr << "shm_unlink() failed with error " << strerror(errno) << std::endl;
#endif
        throw std::exception();
  }
  
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
        return arr[i % _size];
    } else {
        return arr[(i % _size) + _size];
    }
}



