#pragma once
#include "hlm_vector.h"

#ifndef _HLM_SMART_VECTOR_CPP_
#define _HLM_SMART_VECTOR_CPP_

template <typename T>
inline HELIUM_API::SharedVector<T>::Data::Data() : vector(new std::vector<T>()), count(1)
{
    std::atexit(Data::checkGlobalCount);
    UUID = (++GlobalCount());
#ifdef HELIUM_API_DEBUG_PROFILE_ENABLE
    std::cout << "Created New SharedVector with ID = " << GlobalCount() << "\n";
#endif
}

template <typename T>
inline HELIUM_API::SharedVector<T>::Data::Data(const std::vector<T>& externalVector) : vector(new std::vector<T>(std::move(externalVector))), count(1)
{
    std::atexit(Data::checkGlobalCount);
    UUID = (++GlobalCount());
#ifdef HELIUM_API_DEBUG_PROFILE_ENABLE
    std::cout << "Created New SharedVector with ID = " << GlobalCount() << "\n";
#endif
}

template <typename T>
inline HELIUM_API::SharedVector<T>::Data::Data(const std::vector<T>&& externalVector) : vector(new std::vector<T>(std::move(externalVector))), count(1)
{
    std::atexit(Data::checkGlobalCount);
    UUID = (++GlobalCount());
#ifdef HELIUM_API_DEBUG_PROFILE_ENABLE
    std::cout << "Created New SharedVector with ID = " << GlobalCount() << "\n";
#endif
}

template <typename T>
inline HELIUM_API::SharedVector<T>::Data::~Data()
{
    --GlobalCount();
#ifdef HELIUM_API_DEBUG_PROFILE_ENABLE
    std::cout << "Deleted SharedVector with ID =  " << UUID << "\n";
#endif
    delete vector;
}

// Caution : Not meant for external use
template <typename T>
inline void HELIUM_API::SharedVector<T>::delete_vector() {
    try {
        if (m_data_ != nullptr) {
            {
                if (m_data_->vector != nullptr)
                    delete m_data_->vector;
            }
            m_data_->vector = nullptr;
        }
    } catch (...) {
        throw std::runtime_error("Deleting an invalid std::vector Pointer");
    }
}

// Caution : Not meant for external use
// Only meant for reassigning new vector
// Delete the data and move new copy
template <typename T>
inline void HELIUM_API::SharedVector<T>::force_delete_data() {
    if (m_data_ != nullptr) {
        {
            delete m_data_;
        }
        m_data_ = nullptr;
    }
}

// Release the data
template <typename T>
inline void HELIUM_API::SharedVector<T>::release_reference() {
    if (m_data_ != nullptr) {
        --m_data_->count;
        if (m_data_->count == 0) {
            delete m_data_;
        }
    }
    // release the data
    m_data_ = nullptr;
}

// Crititcal functionality !!! Do not modify 
// check validity
template <typename T>
inline bool HELIUM_API::SharedVector<T>::is_valid() const
{
    if (m_data_ != nullptr && m_data_->vector != nullptr && (m_data_->count) != 0)
    {
        return true;
    }
    else
    {
        throw std::runtime_error("Accessing null or released vector");
        return false;
    }
}

template <typename T>
inline T& HELIUM_API::SharedVector<T>::DefaultValue()
{
    static T default_value;
    return default_value;
}

template <typename T>
inline size_t HELIUM_API::SharedVector<T>::ref_count()
{
    return m_data_->count;
}

template <typename T>
inline const size_t HELIUM_API::SharedVector<T>::ref_count() const
{
    return m_data_->count;
}

template <typename T>
inline size_t HELIUM_API::SharedVector<T>::data_id()
{
    return m_data_->UUID;
}

template <typename T>
inline const size_t HELIUM_API::SharedVector<T>::data_id() const
{
    return m_data_->UUID;
}

template <typename T>
inline HELIUM_API::SharedVector<T>::SharedVector() : m_data_(new Data()) {}

template <typename T>
inline HELIUM_API::SharedVector<T>::SharedVector(const std::vector<T>&& externalVector, const bool& move_semantic)
{
    if (move_semantic == HLM_COPY)
    {
        m_data_ = new Data(externalVector);
    }
    else
    {
        m_data_ = (new Data());
        *(m_data_->vector) = std::move(externalVector);
    }
}

template <typename T>
inline HELIUM_API::SharedVector<T>::SharedVector(const std::vector<T>& externalVector, const bool& move_semantic)
{
    if (move_semantic == HLM_COPY)
    {
        m_data_ = new Data(externalVector);
    }
    else
    {
        m_data_ = (new Data());
        *(m_data_->vector) = std::move(externalVector);
    }
}

template <typename T>
inline HELIUM_API::SharedVector<T>::SharedVector(const HELIUM_API::SharedVector<T>& externalVector, const bool& move_semantic)
{
    std::atexit(Data::checkGlobalCount);
    if (move_semantic == HLM_MOVE)
    {
        this->m_data_ = externalVector.m_data_;
        ++(this->m_data_->count);
    }
    else
    {
        m_data_ = (new Data((*externalVector.m_data_->vector)));
    }
}

template <typename T>
inline HELIUM_API::SharedVector<T>::~SharedVector()
{
    release_reference();
}

template <typename T>
inline const HELIUM_API::SharedVector<T>& HELIUM_API::SharedVector<T>::operator=(const std::vector<T>&& externalVector)
{
    delete_vector();
    (m_data_->vector) = new std::vector<T>(std::move(externalVector));
    return *this;
}

template <typename T>
inline const HELIUM_API::SharedVector<T>& HELIUM_API::SharedVector<T>::operator=(const std::vector<T>& externalVector)
{
    delete_vector();
    (m_data_->vector) = new std::vector<T>(std::move(externalVector));
    return *this;
}

template <typename T>
inline const HELIUM_API::SharedVector<T>& HELIUM_API::SharedVector<T>::operator=(const HELIUM_API::SharedVector<T>& externalVector)
{
    release_reference();
    this->m_data_ = externalVector.m_data_;
    ++(this->m_data_->count);
    return *this;
}

template <typename T>
inline bool HELIUM_API::SharedVector<T>::operator==(const SharedVector& other) const
{
    return (m_data_ == other.m_data_);
}

template <typename T>
inline bool HELIUM_API::SharedVector<T>::operator!=(const SharedVector& other) const
{
    return !(m_data_ == other.m_data_);
}

template <typename T>
inline HELIUM_API::SharedVector<T> HELIUM_API::SharedVector<T>::operator+(const SharedVector& other) const
{
    SharedVector result;

    if (is_valid())
    {
        result.m_data_->vector = new std::vector<T>(*m_data_->vector);

        if (other.is_valid())
        {
            result.m_data_->vector->insert(result.m_data_->vector->end(), other.m_data_->vector->begin(), other.m_data_->vector->end());
        }
    }
    return result;
}

template <typename T>
inline HELIUM_API::SharedVector<T>::operator std::vector<T>() const
{
    if (is_valid())
    {
        return *(m_data_->vector);
    }
    else
    {
        std::vector<T> empty_copy_vec;
        return empty_copy_vec;
    }
}

template <typename T>
inline T& HELIUM_API::SharedVector<T>::fast_access(const size_t& index)
{
    return (*m_data_->vector)[static_cast<size_t>(index)];
}

template <typename T>
inline T& HELIUM_API::SharedVector<T>::operator[](const int& index)
{
    if (is_valid())
    {
        if (index >= 0 && static_cast<size_t>(index) < m_data_->vector->size())
        {
            return (*m_data_->vector)[static_cast<size_t>(index)];
        }
        else if (index < 0 && static_cast<size_t>(-index) <= m_data_->vector->size())
        {
            return (*m_data_->vector)[m_data_->vector->size() - static_cast<size_t>(-index)];
        }
        else
        {
            std::cerr << "\nWarning : Index " << index << " out of bound. Returning default value \n";
            return back();
        }
    }
    else
    {
        return DefaultValue();
    }
}

template <typename T>
inline T& HELIUM_API::SharedVector<T>::operator[](const size_t& index)
{
    if (is_valid())
    {
        if ((index) < m_data_->vector->size())
        {
            return (*m_data_->vector)[(index)];
        }
        else
        {
            std::cerr << "\nWarning : Index " << index << " out of bound. Returning back or default value \n";
            return back();
        }
    }
    else
    {
        return DefaultValue();
    }
}

template <typename T>
inline const T& HELIUM_API::SharedVector<T>::operator[](const int index) const
{
    if (is_valid())
    {
        if (index >= 0 && static_cast<size_t>(index) < m_data_->vector->size())
        {
            return (*m_data_->vector)[static_cast<size_t>(index)];
        }
        else if (index < 0 && static_cast<size_t>(-index) <= m_data_->vector->size())
        {
            return (*m_data_->vector)[m_data_->vector->size() - static_cast<size_t>(-index)];
        }
        else
        {
            std::cerr << "\nWarning : Index " << index << " out of bound. Returning default value \n";
            return back();
        }
    }
    else
    {
        return DefaultValue();
    }
}

template <typename T>
inline const T* HELIUM_API::SharedVector<T>::data() const
{
    return (is_valid() && size()) ? &(*(m_data_->vector))[0] : &(DefaultValue());
}

template <typename T>
inline T* HELIUM_API::SharedVector<T>::data()
{
    return (is_valid() && size()) ? &(*(m_data_->vector))[0] : &(DefaultValue());
}

template <typename T>
inline T& HELIUM_API::SharedVector<T>::back()
{
    return (is_valid() && size()) ? m_data_->vector->back() : DefaultValue();
}

template <typename T>
inline const T& HELIUM_API::SharedVector<T>::back() const
{
    return (is_valid() && size()) ? m_data_->vector->back() : DefaultValue();
}

template <typename T>
inline T& HELIUM_API::SharedVector<T>::front()
{
    return (is_valid() && size()) ? m_data_->vector->front() : DefaultValue();
}

template <typename T>
inline const T& HELIUM_API::SharedVector<T>::front() const
{
    return (is_valid() && size()) ? m_data_->vector->front() : DefaultValue();
}

template <typename T>
inline size_t HELIUM_API::SharedVector<T>::size() const
{
    return (is_valid()) ? m_data_->vector->size() : 0;
}

template <typename T>
inline size_t HELIUM_API::SharedVector<T>::capacity() const
{
    return (is_valid()) ? m_data_->vector->capacity() : 0;
}

template <typename T>
inline size_t HELIUM_API::SharedVector<T>::max_capacity() const
{
    return (is_valid()) ? m_data_->vector->max_capacity() : 0;
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::clear()
{
    if (is_valid())
    {
        m_data_->vector->clear();
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::push_back(const T& value)
{
    if (is_valid())
    {
        m_data_->vector->push_back(value);
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::emplace_back(const T& value)
{
    if (is_valid())
    {
        m_data_->vector->emplace_back(value);
    }
}

template <typename T>
inline T HELIUM_API::SharedVector<T>::pop_back(const T& value)
{
    if (is_valid())
    {
        T poped_data = m_data_->vector->back();
        m_data_->vector->pop_back(value);
        return poped_data;
    }
    else
    {
        return back();
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::emplace(const T& value)
{
    if (is_valid())
    {
        m_data_->vector->emplace(value);
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::resize(const size_t& value)
{
    if (is_valid())
    {
        m_data_->vector->resize(value);
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::shrink_to_fit(const T& value)
{
    if (is_valid())
    {
        m_data_->vector->shrink_to_fit(value);
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::insert(const SharedVector& other) const
{
    if (other.is_valid() && this->is_valid())
    {
        this->m_data_->vector->insert(this->m_data_->vector->end(), other.m_data_->vector->begin(), other.m_data_->vector->end());
    }
}

template <typename T>
inline typename std::vector<T>::iterator HELIUM_API::SharedVector<T>::begin()
{
    if (is_valid())
    {
        return m_data_->vector->begin();
    }
}

template <typename T>
inline typename std::vector<T>::const_iterator HELIUM_API::SharedVector<T>::begin() const
{
    if (is_valid())
    {
        return m_data_->vector->begin();
    }
}

template <typename T>
inline typename std::vector<T>::iterator HELIUM_API::SharedVector<T>::end()
{
    if (is_valid())
    {
        return m_data_->vector->end();
    }
}

template <typename T>
inline typename std::vector<T>::const_iterator HELIUM_API::SharedVector<T>::end() const
{
    if (is_valid())
    {
        return m_data_->vector->end();
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::broadcast(const T& value)
{
    if (is_valid())
    {
        m_data_->vector->assign(size(), value);
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::broadcast(BroadcastFunctor<T>& functor)
{
    if (is_valid())
    {
#ifdef HLM_OMP_PARALLEL
#pragma omp parallel for schedule(dynamic, 8)
#endif
        for (size_t i = 0; i < size(); ++i)
        {
            functor((*(m_data_->vector))[i]);
        }
    }
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::replace_with(const T& oldVal, const T& newVal)
{
    if (is_valid())
    {
        std::replace(m_data_->vector->begin(), m_data_->vector->end(), oldVal, newVal);
    }
}

template <typename T>
inline typename std::vector<T>::iterator HELIUM_API::SharedVector<T>::find_iter(const T& value)
{
    if (is_valid())
    {
        return (std::find(m_data_->vector->begin(), m_data_->vector->end(), value));
    }
    else
    {
        return m_data_->vector->end();
    }
}

template <typename T>
inline typename std::vector<T>::const_iterator HELIUM_API::SharedVector<T>::find_iter(const T& value) const
{
    if (is_valid())
    {
        return (std::find(m_data_->vector->begin(), m_data_->vector->end(), value));
    }
    else
    {
        return m_data_->vector->end();
    }
}

template <typename T>
inline T& HELIUM_API::SharedVector<T>::find(const T& value)
{
    if (is_valid())
    {
        return *(std::find(m_data_->vector->begin(), m_data_->vector->end(), value));
    }
    else
    {
        return *(m_data_->vector->end());
    }
}

template <typename T>
inline const T& HELIUM_API::SharedVector<T>::find(const T& value) const
{
    if (is_valid())
    {
        return *(std::find(m_data_->vector->begin(), m_data_->vector->end(), value));
    }
    else
    {
        return *(m_data_->vector->end());
    }
}

// Reduce the vector using a functor
template <typename T>
inline T HELIUM_API::SharedVector<T>::reduce(const ReduceFunctor<T>& functor) {
    if (is_valid()) {
        T accum = (*this)[0];

        for (size_t i = 1; i < size(); ++i) {
            accum = functor((*this)[i], accum);
        }
        return accum;
    } else {
        return DefaultValue();
    }
}

// Filter the vector to remove duplicates
template <typename T>
inline void HELIUM_API::SharedVector<T>::filter() {
    if (is_valid()) {
        std::sort(m_data_->vector->begin(), m_data_->vector->end());
        m_data_->vector->erase(std::unique(m_data_->vector->begin(), m_data_->vector->end()), m_data_->vector->end());
    }
}

// Swap external vector with internal
template <typename T>
inline void HELIUM_API::SharedVector<T>::swap(const SharedVector& externalVector) {
    const Data* temp = this->m_data_;
    this->m_data_ = externalVector.m_data_;
    externalVector.m_data_ = temp;
}

template <typename T>
inline void HELIUM_API::SharedVector<T>::swap(const std::vector<T>& externalVector) {
    std::vector<T> temp    = std::move(*(this->m_data_->vector));
    this->m_data_->vector  = std::move(externalVector.m_data_->vector);
    externalVector.m_data_ = std::move(temp);
}

// Display the vector content
template <typename T>
inline void HELIUM_API::SharedVector<T>::display() const {
    if (is_valid()) {
        std::cout << "SharedVector content: ";
        const HELIUM_API::SharedVector<T>& temp = *this;
        for (size_t i = 0; i < size(); ++i) {
            std::cout << (temp[i]) << " ";
        }
        std::cout << "\n";
    } 
}
#endif
