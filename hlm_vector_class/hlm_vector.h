#pragma once
#ifndef _HLM_SMART_VECTOR_HPP_
#define _HLM_SMART_VECTOR_HPP_
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <atomic>
#ifdef HLM_OMP_PARALLEL
#include <omp.h>
#endif

#define USE_HEADER_ONLY_IMPLEMENTATION

namespace HELIUM_API {

// Makes sense :) sometimes
#define until while

template <typename T>
class ReduceFunctor {
public:
    virtual T operator()(const T& acc, const T& element) const = 0;
    virtual ~ReduceFunctor() = default;
};

template <typename T>
class BroadcastFunctor {
public:
    virtual void operator()(T& element) = 0;
    virtual ~BroadcastFunctor() = default;
};

/// @brief class HLM::SharedVector
/// A safe vector container to prevent dangling references or pointers
/// Never exposes the data pointer or reference outside 
template <typename T>
class SharedVector {
private:
    /// @brief struct Data
    // Data struct is a simple way to create & manage the vector container
    // SharedVector Will never expose the pointer or even a reference externaly 
    // Caution : Not meant for external use
    struct Data {
        static size_t& GlobalCount() {
            static std::atomic<size_t> global_count = 0;
            return global_count;
        }

        static void checkGlobalCount() {
            if (GlobalCount() != 0) {
                throw std::runtime_error("Not all instances have been deleted");
            }
        }
        // Caution : Not meant for external use
        std::vector<T>* vector;
        std::atomic<size_t> count;
        size_t UUID;

        inline Data();
        inline Data(const std::vector<T>&  externalVector);
        inline Data(const std::vector<T>&& externalVector);
        inline ~Data();
    };

    Data* m_data_;

    // Caution : Not meant for external use
    // Only meant for reassigning new vector
    // Delete the data and move new copy
    inline void delete_vector();
    inline void force_delete_data();
    inline void release_reference();

    inline T* operator&();
    inline T& operator*();
    inline const T& operator*() const;
    inline T* operator->();
    inline const T* operator->() const;

    void* operator new(std::size_t);
    void  operator delete(void*);

public:

////////////////////////////////////////////////////////////////////////////////////////
////////  Smart & Safety check functions //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////  
    // Crititcal functionality !!! Do not modify 
    // check validity

    inline bool is_valid() const;
    inline static T& DefaultValue();
    inline size_t ref_count();
    inline const size_t ref_count() const;
    inline size_t data_id();
    inline const size_t data_id() const;

///////////////////////////////////////////////////////////////////////////////////////
///// Fancy Ways to Construct vector data with interoperability with std::vector //////
////  Default Preference for Move Semantic !!!!!!! ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
    
    #define HLM_MOVE 1
    #define HLM_COPY 0

    inline SharedVector();
    inline SharedVector(const std::vector<T>&  externalVector, const bool& move_semantic = HLM_MOVE);    
    inline SharedVector(const std::vector<T>&& externalVector, const bool& move_semantic = HLM_MOVE);
    inline SharedVector(const SharedVector<T>& externalVector, const bool& move_semantic = HLM_MOVE);
    inline ~SharedVector();


/////////////////////////////////////////////////////////////////////////////////////
///// Public Operators 
////////////////////////////////////////////////////////////////////////////////////

    // Assignment operator from external vector using move semantics
    inline const SharedVector& operator=(const std::vector<T>&  externalVector);
    inline const SharedVector& operator=(const std::vector<T>&& externalVector);
    inline const SharedVector& operator=(const SharedVector<T>& externalVector);

    // Equality operator
    inline bool operator==(const SharedVector& other) const;
    // Inequality operator    
    inline bool operator!=(const SharedVector& other) const;
    // Overload + operator for concatenation
    inline SharedVector operator+(const SharedVector& other) const;
    // Conversion operator to std::vector<T>
    // Pass a Copy to external vec if the data is valid 
    inline operator std::vector<T>() const;

    inline T& fast_access(const size_t& index);
    // Access element at index (allowing negative indices for reverse access)
    inline T& operator[](const int& index);
    inline const T& operator[](const int index) const;

    // Access element at index
    inline T& operator[](const size_t& index);
    
//////////////////////////////////////////////////////////////////////////////////////////
/////////// Wrapper functions around std::vector member functions
//////////////////////////////////////////////////////////////////////////////////////////

    // Get data ie... the first element
    inline const T* data() const;
    inline T* data();
    // Get the last element of the vector
    inline T& back();
    inline const T& back() const;
    // Get the first element of the vector
    inline T& front();
    inline const T& front() const;
    // All others are just wrapper arounf std::vector member functions :)
    inline size_t size() const;
    inline size_t capacity() const;
    inline size_t max_capacity() const;
    inline void clear();
    inline void push_back(const T& value);
    inline void emplace_back(const T& value);
    inline T pop_back(const T& value);
    inline void emplace(const T& value);
    inline void resize(const size_t& value = 0);
    inline void shrink_to_fit(const T& value);
    // insert an another vector to the front of this vector
    inline void insert(const SharedVector& other) const;
    // Traditional std::vector::iterators
    inline typename std::vector<T>::iterator begin();
    inline typename std::vector<T>::const_iterator begin() const;
    inline typename std::vector<T>::iterator end();
    inline typename std::vector<T>::const_iterator end() const;

////////////////////////////////////////////////////////////////////
////////// Fancy Functions  ////////////////////////////////////////
////////////////////////////////////////////////////////////////////

    // Broadcast a value to all elements in the vector
    inline void broadcast(const T& value);
    // Broadcast a functor to all elements in the vector
    inline void broadcast(BroadcastFunctor<T>& functor);
    // Reduce the vector using a functor
    inline T reduce(const ReduceFunctor<T>& functor);
    // Filter the vector to remove duplicates
    inline void filter();
    // Replace elements in the vector equal to oldVal with a new value
    inline void replace_with(const T& oldVal, const T& newVal);
    // Find the iterator to the first occurrence of a value
    inline typename std::vector<T>::iterator find_iter(const T& value);
    inline typename std::vector<T>::const_iterator find_iter(const T& value) const;
    // Find the value by ref to the first occurrence of a value
    inline T& find(const T& value);
    inline const T& find(const T& value) const;
    // Swap external vector with internal uisng move semantics
    inline void swap(const SharedVector&    externalVector);
    inline void swap(const std::vector<T>&  externalVector);
    
    // Display the vector content
    inline void display() const;
};

}  // namespace HELIUM_API

#ifdef USE_HEADER_ONLY_IMPLEMENTATION
#include "hlm_vector.cpp"
#endif

#endif
