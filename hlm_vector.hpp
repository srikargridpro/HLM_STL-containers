#ifndef _HLM_SMART_VECTOR_HPP_
#define _HLM_SMART_VECTOR_HPP_
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#ifdef HLM_OMP_PARALLEL
#include <omp.h>
#endif

namespace HLM {

// Functor interfaces
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

/// @brief class HLM::Vector
/// A safe vector container to prevent dangling references or pointers
/// Never exposes the data pointer or reference outside 

template <typename T>
class Vector {
private:
    /// @brief struct Data
    // Data struct is a simple way to create & manage the vector container
    // Vector Will never expose the pointer or even a reference externaly 
    // Caution : Not meant for external use
    struct Data {

         static size_t& GlobalCount()
         {
           static size_t global_count = 0;
           return global_count;
         }

        static void checkGlobalCount() {
            if (GlobalCount() != 0) {
                // Throw an exception or print an error message
                throw std::runtime_error("Not all instances have been deleted");
            }
        }

        std::vector<T>* vector;
        size_t count;
        size_t UUID;

        Data() : vector(new std::vector<T>()), count(1) 
        { 
            std::atexit(Data::checkGlobalCount);
            UUID = (++GlobalCount());
            #ifdef HELIUM_API_DEBUG_PROFILE_ENABLE
            std::cout << "Created New Vector with ID = " << GlobalCount() << "\n"; 
            #endif
        }
        
        Data(const std::vector<T>& externalVector) : vector(new std::vector<T>(std::move(externalVector))), count(1)  
        { 
            std::atexit(Data::checkGlobalCount);
            UUID = (++GlobalCount());
            #ifdef HELIUM_API_DEBUG_PROFILE_ENABLE
            std::cout << "Created New Vector with ID = " << GlobalCount() << "\n"; 
            #endif
        }

        Data(const std::vector<T>&& externalVector) : vector(new std::vector<T>(std::move(externalVector))), count(1)  
        { 
            std::atexit(Data::checkGlobalCount);
            UUID = (++GlobalCount());
            #ifdef HELIUM_API_DEBUG_PROFILE_ENABLE
            std::cout << "Created New Vector with ID = " << GlobalCount() << "\n"; 
            #endif
        }
 
        // Delete the std::vector<T>* vector 
       ~Data() 
        {
           --GlobalCount();
           #ifdef HELIUM_API_DEBUG_PROFILE_ENABLE
           std::cout << "Deleted Vector with ID =  " << UUID << "\n";            
           #endif
           delete vector;
        }
    };

    // Caution : Not meant for external use
    Data* m_data_;

    // Caution : Not meant for external use
    void delete_vector() {
        try {
        if (m_data_ != nullptr) {
         {
             if(m_data_->vector != nullptr)
             delete m_data_->vector;
         }
         m_data_->vector = nullptr;
        }
        }
        catch(...)
        {
            throw std::runtime_error("Deleting and invalid std::vector Pointer");
        }
    }

    // Caution : Not meant for external use
    // Only meant for reassigning new vector
    // Delete the data and move new copy
    void force_delete_data() {
        if (m_data_ != nullptr) {
         {
             delete m_data_;
         }
         m_data_ = nullptr;
        }
    }

    // Release the data
    void release_reference() {
        if (m_data_ != nullptr) {
            --m_data_->count;
            if (m_data_->count == 0) {
                delete m_data_;
            }
        }
        // release the data
        m_data_ = nullptr;
    }

   // Protect address-of operator with no implementation
    T* operator&();

    // Protect reference operator and address-of operator
    T& operator*();
    const T& operator*() const;
    T* operator->();
    const T* operator->() const;

/*
 // Delete operator new and operator delete
    void* operator new(std::size_t) = delete;
    void  operator delete(void*) = delete;
*/

    // Protect operator new and operator delete
    void* operator new(std::size_t);
    void  operator delete(void*);

public:    
////////////////////////////////////////////////////////////////////////////////////////
////////  Smart & Safety check functions //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////  
    // Crititcal functionality !!! Do not modify 
    // check validity
    bool is_valid() const
    {
         if(m_data_ != nullptr && m_data_->vector != nullptr  && (m_data_->count) != 0 ) 
         {
            return true;
         }
         else
         {
          throw std::runtime_error("Accessing null or released vector"); 
          return false;  
         }
    }
    
    static T& DefaultValue()
    {  
      static T default_value;
      return default_value; 
    }

    size_t ref_count()
    {
        return m_data_->count;
    }
    
    const size_t ref_count() const
    {
        return m_data_->count;
    }

    size_t data_id()
    {
        return m_data_->UUID;
    }

    const size_t data_id() const
    {
        return m_data_->UUID;
    }

///////////////////////////////////////////////////////////////////////////////////////
///// Fancy Ways to Construct vector data with interoperability with std::vector //////
////  Default Preference for Move Semantic !!!!!!! ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

    // Constructor
    Vector() : m_data_(new Data()) {}

    #define HLM_MOVE 1
    #define HLM_COPY 0

    Vector(const std::vector<T>&& externalVector, const bool& move_semantic = HLM_MOVE)
    {
        if(move_semantic == HLM_COPY)
        {
           m_data_ = new Data(externalVector);
        }
        else
        {
            m_data_ = (new Data());
           *(m_data_->vector) = std::move(externalVector);      
        }
    }

    Vector(const std::vector<T>& externalVector, const bool& move_semantic = HLM_MOVE)   
    {
        if(move_semantic == HLM_COPY)
        {
           m_data_ = new Data(externalVector);
        }
        else
        {
            m_data_ = (new Data());
           *(m_data_->vector) = std::move(externalVector);      
        }
    }

    Vector(const Vector<T>& externalVector, const bool& move_semantic = HLM_MOVE) 
    {
         std::atexit(Data::checkGlobalCount);
         if(move_semantic == HLM_MOVE) {
           this->m_data_ = externalVector.m_data_;
           ++(this->m_data_->count); 
         }
         else 
         {
             m_data_ = (new Data((*externalVector.m_data_->vector)));
         }      
    }            

    // Destructor
    ~Vector() {
        release_reference();
    }

/////////////////////////////////////////////////////////////////////////////////////
///// Public Operators 
////////////////////////////////////////////////////////////////////////////////////

    // Assignment operator from external vector using move semantics
    const Vector& operator=(const std::vector<T>&& externalVector) {
        delete_vector();
        (m_data_->vector) = new std::vector<T>(std::move(externalVector));
        return *this;
    }

    const Vector& operator=(const std::vector<T>& externalVector) {
        delete_vector();
        (m_data_->vector) = new std::vector<T>(std::move(externalVector));
        return *this;
    } 

    const Vector& operator=(const Vector<T>& externalVector) {
        release_reference();
        this->m_data_ = externalVector.m_data_;
        ++(this->m_data_->count); 
        return *this;
    } 

    // Equality operator
    bool operator==(const Vector& other) const {
        return (m_data_ == other.data_);
    }

    // Inequality operator
    bool operator!=(const Vector& other) const {
        return !(m_data_ == other.m_data_);
    }

    // Overload + operator for concatenation
    Vector operator+(const Vector& other) const {
        Vector result;

        if (is_valid()) {
            // Copy the elements of the current vector
            result.m_data_->vector = new std::vector<T>(*m_data_->vector);

            // Concatenate the elements of the other vector
            if (other.is_valid()) {
                result.m_data_->vector->insert(result.m_data_->vector->end(), other.m_data_->vector->begin(), other.m_data_->vector->end());
            }
        }
        return result;
    }

    // Conversion operator to std::vector<T>
    // Pass a Copy to external vec if the data is valid 
    operator std::vector<T>() const {
        if (is_valid()) {
            return *(m_data_->vector);
        }
        else 
        {
          std::vector<T> empty_copy_vec;
          return empty_copy_vec;
        }
    }

    T& fast_access(const size_t& index)
    {
        return (*m_data_->vector)[static_cast<size_t>(index)]; 
    }

    // Access element at index (allowing negative indices for reverse access)
    T& operator[](const int& index) {
        if (is_valid()) {
            if (index >= 0 && static_cast<size_t>(index) < m_data_->vector->size()) {
                return (*m_data_->vector)[static_cast<size_t>(index)];
            } else if (index < 0 && static_cast<size_t>(-index) <= m_data_->vector->size()) {
                return (*m_data_->vector)[m_data_->vector->size() - static_cast<size_t>(-index)];
            } else {
                std::cerr << "\nWarning : Index " << index << " out of bound. Returning default value \n";
                return back(); 
            }
        }
        else 
        {
           return DefaultValue();
        }
    }

    // Access element at index
    T& operator[](const size_t& index) {
        if (is_valid()) {
            if ((index) < m_data_->vector->size()) {
                return (*m_data_->vector)[(index)];
            }
            else {
                std::cerr << "\nWarning : Index " << index << " out of bound. Returning back or default value \n";
                return back(); 
            }
        }
        else 
        {
           return DefaultValue();
        }
    }

    // Access element at index (allowing negative indices for reverse access)
    const T& operator[](const int index) const {
        if (is_valid()) {
            if (index >= 0 && static_cast<size_t>(index) < m_data_->vector->size()) {
                return (*m_data_->vector)[static_cast<size_t>(index)];
            } else if (index < 0 && static_cast<size_t>(-index) <= m_data_->vector->size()) {
                return (*m_data_->vector)[m_data_->vector->size() - static_cast<size_t>(-index)];
            } else {
                std::cerr << "\nWarning : Index " << index << " out of bound. Returning default value \n";
                return back(); 
            }
        }
        else 
        {
           return DefaultValue();
        }
    }
//////////////////////////////////////////////////////////////////////////////////////////
/////////// Wrapper functions around std::vector member functions
/////////////////////////////////////////////////////////////////////////////////////////

    // Get data ie... the first element
    const T* data() const {
        return (is_valid() && size()) ? &(*(m_data_->vector))[0]  : &(DefaultValue());
    }

    // Get data ie... the first element
    T* data() {
        return (is_valid() && size()) ? &(*(m_data_->vector))[0]  : &(DefaultValue());
    }

    // Get the last element of the vector
    T& back() {
        return (is_valid() && size()) ? m_data_->vector->back()  : DefaultValue();
    }

    // Get the last element of the vector
    const T& back() const {
        return (is_valid() && size()) ? m_data_->vector->back()  : DefaultValue();
    }

    // Get the front element of the vector
    T& front() {
        return (is_valid() && size()) ? m_data_->vector->front() : DefaultValue();
    }

    // Get const version of the const element of the vector
    const T& front() const {
        return (is_valid() && size()) ? m_data_->vector->front() : DefaultValue();
    }

    // Get the size of the vector
    size_t size() const {
        return (is_valid()) ? m_data_->vector->size() : 0;
    }

    // Get the capacity of the vector
    size_t capacity() const {
        return (is_valid()) ? m_data_->vector->capacity() : 0;
    }

    // Get the max_capacity of the vector
    size_t max_capacity() const {
        return (is_valid()) ? m_data_->vector->max_capacity() : 0;
    }    

    // Clear the vector
    void clear() {
        if (is_valid()) {
            m_data_->vector->clear();
        }   
    }

    // Push an element to the back of the vector
    void push_back(const T& value) {
        if (is_valid()) {
            m_data_->vector->push_back(value);
        }
    }

    // emplace back an element to the back of the vector
    void emplace_back(const T& value) {
        if (is_valid()) {
            m_data_->vector->emplace_back(value);
        }
       
    }

    // Pop back an element to the back of the vector
    T pop_back(const T& value) {
        if (is_valid()) {
            T poped_data = m_data_->vector->back();
            m_data_->vector->pop_back(value);
            return poped_data;
        }
        else
        {
            return back();
        }
       
    }

    // emplace an element to the front of the vector
    void emplace(const T& value) {
        if (is_valid()) {
            m_data_->vector->emplace(value);
        }
    }

    // resize the vector
    void resize(const size_t& value = 0) {
        if (is_valid()) {
            m_data_->vector->resize(value);
        }
    }

    // shrink_to_fit the vector
    void shrink_to_fit(const T& value) {
        if (is_valid()) {
            m_data_->vector->shrink_to_fit(value);
        }
    }

    // insert an element to the front of the vector
    void insert(const Vector& other) const {
          if (other.is_valid() && this->is_valid()) {
                this->m_data_->vector->insert(this->m_data_->vector->end(), other.m_data_->vector->begin(), other.m_data_->vector->end());
            }
    }

    // Begin iterator of the vector
    typename std::vector<T>::iterator begin() {    // Swap external vector with internal
        if (is_valid()) {
            return m_data_->vector->begin();
        }
    }

    // Const Begin iterator of the vector
    typename std::vector<T>::const_iterator begin() const {
        if (is_valid()) {
            return m_data_->vector->begin();
        }
    }

    // End iterator of the vector
    typename std::vector<T>::iterator end() {
        if (is_valid()) {
            return m_data_->vector->end();
        }
    }

    // Const End iterator of the vector
    typename std::vector<T>::const_iterator end() const {
        if (is_valid()) {
            return m_data_->vector->end();
        }
    }

////////////////////////////////////////////////////////////////////
////////// Fancy Functions  ///////////////////////////////////////
///////////////////////////////////////////////////////////////////

    // Broadcast a value to all elements in the vector
    void broadcast(const T& value) {
        if (is_valid()) {
            m_data_->vector->assign(size(), value);
        }
    }

     // Broadcast a functor to all elements in the vector
    void broadcast(BroadcastFunctor<T>& functor) {
        if (is_valid()) {
            #ifdef HLM_OMP_PARALLEL
            #pragma omp parallel for schedule(dynamic , 8)
            #endif
            for (size_t i = 0; i < size(); ++i) 
            { 
                functor((*(m_data_->vector))[i]);
            }
        }
    }

    // Replace elements in the vector equal to oldVal with a new value
    void replace_with(const T& oldVal, const T& newVal) {
        if (is_valid()) {
            std::replace(m_data_->vector->begin(), m_data_->vector->end(), oldVal, newVal);
        }
    }

    // Find the iterator to the first occurrence of a value
    typename std::vector<T>::iterator find_iter(const T& value) {
        if (is_valid()) {
            return (std::find(m_data_->vector->begin(), m_data_->vector->end(), value));
        }
        else
        {
            return m_data_->vector->end();
        }
    }

    // Find the iterator to the first occurrence of a value
    typename std::vector<T>::const_iterator find_iter(const T& value) const {
        if (is_valid()) {
            return (std::find(m_data_->vector->begin(), m_data_->vector->end(), value));
        }
        else
        {
            return m_data_->vector->end();
        }
    }

    // Find the value by ref to the first occurrence of a value
    T& find(const T& value) {
        if (is_valid()) {
            return *(std::find(m_data_->vector->begin(), m_data_->vector->end(), value));
        }
        else
        {
            return *(m_data_->vector->end());
        }
    }

    // Find the value by const ref to the first occurrence of a value
    const T& find(const T& value) const {
        if (is_valid()) {
            return *(std::find(m_data_->vector->begin(), m_data_->vector->end(), value));
        }
        else
        {
            return *(m_data_->vector->end());
        }        
    }

    // Reduce the vector using a functor
    T reduce(const ReduceFunctor<T>& functor)  {
        if (is_valid()) {
            
            T accum = (*this)[0];

            for (size_t i = 1; i < size(); ++i) 
            {
                accum  = functor( (*this)[i] , accum);   
            }
            return accum;
        }
        else
        {
          return DefaultValue();
        }
    }

    // Filter the vector to remove duplicates
    void filter() {
        if (is_valid()) {
            std::sort(m_data_->vector->begin(), m_data_->vector->end());
            m_data_->vector->erase(std::unique(m_data_->vector->begin(), m_data_->vector->end()), m_data_->vector->end());
        }
    }

    // Swap external vector with internal
    void swap(const Vector& externalVector)
    {
        const Data* temp = this->m_data_;
        this->m_data_ = externalVector.m_data_;
        externalVector.m_data_ =  temp;
    }

    void swap(const std::vector<T>& externalVector)
    {
        std::vector<T> temp    = std::move(*(this->m_data_->vector));
        this->m_data_->vector  = std::move(externalVector.m_data_->vector);
        externalVector.m_data_ = std::move(temp);
    }

    void swap(const std::vector<T>&& externalVector)
    {
        std::vector<T> temp    = std::move(*(this->m_data_->vector));
        this->m_data_->vector  = std::move(externalVector.m_data_->vector);
        externalVector.m_data_ = std::move(temp);
    }

    // Display the vector content
    void display() const {
        if (is_valid()) {
            std::cout << "Vector content: ";
            const Vector<T>& temp = *this;
            for (size_t i = 0; i < size(); ++i) 
            {
                std::cout << (temp[i]) << " ";  
            }            
            std::cout << "\n";
        } else {
            std::cout << "Vector is null or released." << "\n" ;
        }
    }

/////////////////////////////////////////////////////////////////////////////
}; // class Vector End

} // namespace HLM
 #endif
/*

/////////////////////////////////////////////////////////////
Tests & Demos
/////////////////////////////////////////////////////////////

// MultiplyFunctor implementation
template <typename T>
class MultiplyFunctor : public HLM::ReduceFunctor<T> {
public:
    T operator()(const T& acc, const T& element) const override {
        return acc * element;
    }
};

// AddFunctor implementation (example of another functor)
template <typename T>
class AddFunctor : public HLM::ReduceFunctor<T> {
public:
    T operator()(const T& acc, const T& element) const override {
        return (acc + element + val);
    }
    
    T val;
};

class replacer : public HLM::BroadcastFunctor<int> {
public:
     void operator()(int& element) override { element = val; }
     int val;
};

HLM::Vector<int> tester()
{
    HLM::Vector<int> x = std::vector<int>{99, 98, 97};
    return x;
}

int main() {

    // Tests and demo
    // Use Valgrind to check for memory leaks
    HLM::Vector<int> vectorRef0 =  std::vector<int>{1, 2, 3, 4, 5, 2, 4, 6, 8, 10};
    vectorRef0.display();
    std::cout << "ref_count = " << vectorRef0.ref_count() << "\n";
    
    HLM::Vector<int> vectorRef1(std::vector<int>{1, 2, 3, 4, 5, 2, 4, 6, 8, 10});
    vectorRef1.display();
    std::cout << "ref_count = " << vectorRef1.ref_count() << "\n";

    std::vector<int> init_temp = std::vector<int>{1, 2, 3, 4, 5, 2, 4, 6, 8, 10};
    HLM::Vector<int> vectorRef2(init_temp, HLM_COPY);
    vectorRef2.display();
    std::cout << "ref_count = " << vectorRef2.ref_count() << "\n";

    HLM::Vector<int> vectorRef3(vectorRef0);
    vectorRef3.display();
    std::cout << "ref_count = " << vectorRef3.ref_count() << "\n";

    HLM::Vector<int> vectorRef4;
    vectorRef4 = std::vector<int>{1, 2, 3, 4, 5, 2, 4, 6, 8, 10};
    vectorRef4.display();
    std::cout << "ref_count = " << vectorRef4.ref_count() << "\n";

    HLM::Vector<int> vectorRef5;
    vectorRef5 = init_temp;
    vectorRef5.display();
    std::cout << "ref_count = " << vectorRef5.ref_count() << "\n";

    HLM::Vector<int> vectorRef6;
    vectorRef6 = vectorRef5;
    vectorRef6.display();
    std::cout << "ref_count = " << vectorRef6.ref_count() << "\n";
}
*/
