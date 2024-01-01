#ifndef _HLM_SMART_VECTOR_HPP_
#define _HLM_SMART_VECTOR_HPP_
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#ifdef HLM_OMP_PARALLEL
#include <omp.h>
#endif

#include<unordered_map>

namespace HLM {

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

template <typename T>
class Vector {
private:
    struct Data {
        static size_t& GlobalCount() {
            static size_t global_count = 0;
            return global_count;
        }

        static void checkGlobalCount() {
            if (GlobalCount() != 0) {
                throw std::runtime_error("Not all instances have been deleted");
            }
        }

        std::vector<T>* vector;
        size_t count;
        size_t UUID;

        inline Data();
        inline Data(const std::vector<T>& externalVector);
        inline Data(const std::vector<T>&& externalVector);
        inline ~Data();
    };

    Data* m_data_;

    inline void delete_vector();
    inline void force_delete_data();
    inline void release_reference();

    inline T* operator&();
    inline T& operator*();
    inline const T& operator*() const;
    inline T* operator->();
    inline const T* operator->() const;

    void* operator new(std::size_t);
    void operator delete(void*);

public:
    inline bool is_valid() const;
    inline static T& DefaultValue();
    inline size_t ref_count();
    inline const size_t ref_count() const;
    inline size_t data_id();
    inline const size_t data_id() const;
    
    #define HLM_MOVE 1
    #define HLM_COPY 0

    inline Vector();
    inline Vector(const std::vector<T>&& externalVector, const bool& move_semantic = HLM_MOVE);
    inline Vector(const std::vector<T>& externalVector, const bool& move_semantic = HLM_MOVE);
    inline Vector(const Vector<T>& externalVector, const bool& move_semantic = HLM_MOVE);
    inline ~Vector();

    inline const Vector& operator=(const std::vector<T>&& externalVector);
    inline const Vector& operator=(const std::vector<T>& externalVector);
    inline const Vector& operator=(const Vector<T>& externalVector);

    inline bool operator==(const Vector& other) const;
    inline bool operator!=(const Vector& other) const;
    inline Vector operator+(const Vector& other) const;

    inline operator std::vector<T>() const;

    inline T& fast_access(const size_t& index);
    inline T& operator[](const int& index);
    inline T& operator[](const size_t& index);
    inline const T& operator[](const int index) const;

    inline const T* data() const;
    inline T* data();
    inline T& back();
    inline const T& back() const;
    inline T& front();
    inline const T& front() const;
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
    inline void insert(const Vector& other) const;

    inline typename std::vector<T>::iterator begin();
    inline typename std::vector<T>::const_iterator begin() const;
    inline typename std::vector<T>::iterator end();
    inline typename std::vector<T>::const_iterator end() const;

    inline void broadcast(const T& value);
    inline void broadcast(BroadcastFunctor<T>& functor);
    inline void replace_with(const T& oldVal, const T& newVal);
    inline typename std::vector<T>::iterator find_iter(const T& value);
    inline typename std::vector<T>::const_iterator find_iter(const T& value) const;
    inline T& find(const T& value);
    inline const T& find(const T& value) const;
    inline T reduce(const ReduceFunctor<T>& functor);
    inline void filter();

    inline void swap(const Vector& externalVector);
    inline void swap(const std::vector<T>& externalVector);
    inline void swap(const std::vector<T>&& externalVector);

    inline void display() const;
};

}  // namespace HLM

#ifdef USE_HEADER_ONLY_IMPLEMENTATION
#include "hlm_vector.cpp"
#endif

#endif