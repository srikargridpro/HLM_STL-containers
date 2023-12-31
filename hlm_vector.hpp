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
protected:
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
            UUID = (++GlobalCount());
            std::cout << "Created New Vector with ID = " << GlobalCount() << "\n"; 
        }
        
        Data(const std::vector<T>& externalVector) : vector(new std::vector<T>(externalVector)), count(1) 
        { 
            UUID = (++GlobalCount());
            std::cout << "Created New Vector with ID = " << GlobalCount() << "\n"; 
        }

        Data(const std::vector<T>&& externalVector) : vector(new std::vector<T>(externalVector)), count(1) 
        { 
            UUID = (++GlobalCount());
            std::cout << "Created New Vector with ID = " << GlobalCount() << "\n"; 
        }

        ~Data() 
        {
            (--GlobalCount());
            std::cout << "Deleted Vector with ID =  " << UUID << "\n";            
            delete vector;
        }
    };

    // Caution : Not meant for external use
    Data* data_;

    void delete_vector() {
        try {
        if (data_ != nullptr) {
         {
             if(data_->vector != nullptr)
             delete data_->vector;
         }
         data_->vector = nullptr;
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
        if (data_ != nullptr) {
         {
             delete data_;
         }
         data_ = nullptr;
        }
    }

    // Release the data
    void release_reference() {
        if (data_) {
            --data_->count;
            if (data_->count == 0) {
                delete data_;
            }
            data_ = nullptr;
        }
    }

    // Private address-of operator with no implementation
    T* operator&();

    // Private reference operator and address-of operator
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

    // Public alternative constructors or methods
    static Vector<T> createVector() {
        return Vector<T>();
    }
    
    // Crititcal functionality !!! Do not modify 
    // check validity
    bool is_valid() const
    {
         if(data_ != nullptr && data_->vector != nullptr  && (data_->count) != 0 ) 
         {
            return true;
         }
         else
         {
            throw std::runtime_error("Accessing null or released vector"); 
            return false;  
         }
    }

    size_t ref_count()
    {
        return data_->count;
    }
    
    size_t data_id()
    {
        return data_->UUID;
    }

    // Constructor
    Vector() : data_(new Data()) {std::atexit(Data::checkGlobalCount);}

    #define HLM_MOVE 1
    #define HLM_COPY 0

    Vector(const std::vector<T>&& externalVector, const bool& move_semantic = HLM_MOVE) : data_(new Data()) 
    {
        std::atexit(Data::checkGlobalCount);
        if(move_semantic == HLM_COPY)
          *(data_->vector) = (externalVector);
        else
          *(data_->vector) = std::move(externalVector);
    }



    Vector(const std::vector<T>& externalVector, const bool& move_semantic = HLM_MOVE)   
    {
        std::atexit(Data::checkGlobalCount);
        if(move_semantic == HLM_COPY)
        {
           data_ = new Data(externalVector);
          //*(data_->vector) = (externalVector);
        }
        else
        {
            data_ = (new Data());
           *(data_->vector) = std::move(externalVector);      
        }
    }

    Vector(const Vector<T>& externalVector, const bool& move_semantic = HLM_MOVE) 
    {
         std::atexit(Data::checkGlobalCount);
         if(move_semantic == HLM_MOVE) {
           this->data_ = externalVector.data_;
           ++(this->data_->count); 
         }
         else 
         {
            Data defaultdata;
            (*this->data_) = (externalVector.is_valid() )? (*(externalVector.data_)) : defaultdata ;  
         }      
    }            

    // Destructor
    ~Vector() {
        release_reference();
    }

    static T& DefaultValue()
    {  
      static T default_value;
      return default_value; 
    }

    // Assignment operator from external vector using move semantics
    const Vector& operator=(const std::vector<T>&& externalVector) {
        delete_vector();
        *(data_->vector) = std::move(externalVector);
        return *this;
    }

    const Vector& operator=(const std::vector<T>& externalVector) {
        delete_vector();
        data_->vector = new std::vector<T>(std::move(externalVector));
        return *this;
    } 

    const Vector& operator=(const Vector<T>& externalVector) {
        release_reference();
        this->data_ = externalVector.data_;
        ++(this->data_->count); 
        return *this;
    } 

    // Overload + operator for concatenation
    Vector operator+(const Vector& other) const {
        Vector result;

        if (is_valid()) {
            // Copy the elements of the current vector
            result.data_->vector = new std::vector<T>(*data_->vector);

            // Concatenate the elements of the other vector
            if (other.is_valid()) {
                result.data_->vector->insert(result.data_->vector->end(), other.data_->vector->begin(), other.data_->vector->end());
            }
        }
        return result;
    }

    // Conversion operator to std::vector<T>
    // Pass a Copy to external vec if the data is valid 
    operator std::vector<T>() const {
        if (is_valid()) {
            return *(data_->vector);
        }
        else 
        {
          std::vector<T> empty_copy_vec;
          return empty_copy_vec;
        }
    }

    T& fast_access(const size_t& index)
    {
        return (*data_->vector)[static_cast<size_t>(index)]; 
    }

    // Access element at index (allowing negative indices for reverse access)
    T& operator[](int index) {
        if (is_valid()) {
            if (index >= 0 && static_cast<size_t>(index) < data_->vector->size()) {
                return (*data_->vector)[static_cast<size_t>(index)];
            } else if (index < 0 && static_cast<size_t>(-index) <= data_->vector->size()) {
                return (*data_->vector)[data_->vector->size() - static_cast<size_t>(-index)];
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

    // Access element at index (allowing negative indices for reverse access)
    T& operator[](const size_t index) {
        if (is_valid()) {
            if (index >= 0 && (index) < data_->vector->size()) {
                return (*data_->vector)[(index)];
            }
            else {
                std::cerr << "\nWarning : Index " << index << " out of bound. Returning default value \n";
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
            if (index >= 0 && static_cast<size_t>(index) < data_->vector->size()) {
                return (*data_->vector)[static_cast<size_t>(index)];
            } else if (index < 0 && static_cast<size_t>(-index) <= data_->vector->size()) {
                return (*data_->vector)[data_->vector->size() - static_cast<size_t>(-index)];
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

    // Get the size of the vector
    T& back() const {
        return (is_valid() && size()) ? data_->vector->back()  : DefaultValue();
    }

    T& front() const {
        return (is_valid() && size()) ? data_->vector->front() : DefaultValue();
    }

    // Get the size of the vector
    size_t size() const {
        return (is_valid()) ? data_->vector->size() : 0;
    }

    // Get the capacity of the vector
    size_t capacity() const {
        return (is_valid()) ? data_->vector->capacity() : 0;
    }

    // Get the max_capacity of the vector
    size_t max_capacity() const {
        return (is_valid()) ? data_->vector->max_capacity() : 0;
    }    

    // Clear the vector
    void clear() {
        if (is_valid()) {
            data_->vector->clear();
        }   
    }

    // Push an element to the back of the vector
    void push_back(const T& value) {
        if (is_valid()) {
            data_->vector->push_back(value);
        }
    }

    // emplace back an element to the back of the vector
    void emplace_back(const T& value) {
        if (is_valid()) {
            data_->vector->emplace_back(value);
        }
       
    }

    // Pop back an element to the back of the vector
    T pop_back(const T& value) {
        if (is_valid()) {
            T poped_data = data_->vector->back();
            data_->vector->pop_back(value);
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
            data_->vector->emplace(value);
        }
    }

    // resize the vector
    void resize(const size_t& value = 0) {
        if (is_valid()) {
            data_->vector->resize(value);
        }
    }

    // shrink_to_fit the vector
    void shrink_to_fit(const T& value) {
        if (is_valid()) {
            data_->vector->shrink_to_fit(value);
        }
    }

    // insert an element to the front of the vector
    void insert(const Vector& other) const {
          if (other.is_valid() && this->is_valid()) {
                this->data_->vector->insert(this->data_->vector->end(), other.data_->vector->begin(), other.data_->vector->end());
            }
    }

    // Begin iterator of the vector
    typename std::vector<T>::iterator begin() {
        if (is_valid()) {
            return data_->vector->begin();
        }
    }

    // Const Begin iterator of the vector
    typename std::vector<T>::const_iterator begin() const {
        if (is_valid()) {
            return data_->vector->begin();
        }
    }

    // End iterator of the vector
    typename std::vector<T>::iterator end() {
        if (is_valid()) {
            return data_->vector->end();
        }
    }

    // Const End iterator of the vector
    typename std::vector<T>::const_iterator end() const {
        if (is_valid()) {
            return data_->vector->end();
        }
    }

    // Broadcast a value to all elements in the vector
    void broadcast(const T& value) {
        if (is_valid()) {
            data_->vector->assign(size(), value);
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
                functor((*(data_->vector))[i]);
            }
        }
    }

    // Replace elements in the vector equal to oldVal with a new value
    void replace_with(const T& oldVal, const T& newVal) {
        if (is_valid()) {
            std::replace(data_->vector->begin(), data_->vector->end(), oldVal, newVal);
        }
    }

    // Find the iterator to the first occurrence of a value
    typename std::vector<T>::iterator find_iter(const T& value) {
        if (is_valid()) {
            return (std::find(data_->vector->begin(), data_->vector->end(), value));
        }
        else
        {
            return data_->vector->end();
        }
    }

    // Find the iterator to the first occurrence of a value
    typename std::vector<T>::const_iterator find_iter(const T& value) const {
        if (is_valid()) {
            return (std::find(data_->vector->begin(), data_->vector->end(), value));
        }
        else
        {
            return data_->vector->end();
        }
    }

    // Find the value by ref to the first occurrence of a value
    T& find(const T& value) {
        if (is_valid()) {
            return *(std::find(data_->vector->begin(), data_->vector->end(), value));
        }
        else
        {
            return *(data_->vector->end());
        }
    }

    // Find the value by const ref to the first occurrence of a value
    const T& find(const T& value) const {
        if (is_valid()) {
            return *(std::find(data_->vector->begin(), data_->vector->end(), value));
        }
        else
        {
            return *(data_->vector->end());
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
            std::sort(data_->vector->begin(), data_->vector->end());
            data_->vector->erase(std::unique(data_->vector->begin(), data_->vector->end()), data_->vector->end());
        }
    }

    void swap(const Vector& externalVector)
    {
        const Data* temp = this->data_;
        this->data = externalVector.data_;
        externalVector.data_ =  temp;
    }

    void swap(const std::vector<T>& externalVector)
    {
        std::vector<T> temp  = std::move(this->data_->vector);
        this->data_->vector  = std::move(externalVector.data_->vector);
        externalVector.data_ = std::move(temp);
    }

    void swap(const std::vector<T>&& externalVector)
    {
        std::vector<T> temp  = std::move(this->data_->vector);
        this->data_->vector  = std::move(externalVector.data_->vector);
        externalVector.data_ = std::move(temp);
    }

    // Display the vector content
    void display() const {
        if (is_valid()) {
            std::cout << "Vector content: ";
            const Vector<T>& temp = *this;
            for (int i = 0; i < size(); ++i) 
            {
                std::cout << (temp[i]) << " ";  
            }            
            std::cout << "\n";
        } else {
            std::cout << "Vector is null or released." << "\n" ;
        }
    }
};

} // namespace HLM
