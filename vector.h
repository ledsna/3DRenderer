#ifndef VECTOR_H
#define VECTOR_H

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace VectorImpl {
    template <typename T>
    class Vector {
    public:
        Vector() = default;

        Vector(size_t size) {
            if (size > 0) {
                _capacity = size;
                _elements = new T[_capacity]();
                _size = size;
            }
        }

        Vector(size_t size, const T& value) : Vector(size) {
            for (size_t i = 0; i < _size; ++i) {
                _elements[i] = value;
            }
        }

        Vector(Vector&& other) noexcept {
            _capacity = other._capacity;
            _elements = other._elements;
            _size = other._size;

            other._elements = nullptr;
            other._size = 0;
        }

        Vector(const Vector& other) {
            _capacity = other._capacity;
            _size = other.size;
            _elements = new T[_capacity];
            for (size_t i = 0; i < _size; ++i) {
                _elements[i] = other._elements[i];
            }
        }

        Vector& operator=(const Vector& other) {
            if (this != &other) {
                if (_capacity != other._capacity) { 
                    _size = 0;
                    reallocate(other._capacity - _capacity);
                }
                for (size_t i = 0; i < other._size; ++i) {
                    _elements[i] = other._elements[i];
                }
                _size = other._size;
            }
            return *this;
        }

        Vector& operator=(Vector&& other) noexcept {
            if (this != &other) {
                _capacity = other._capacity;
                delete[] _elements;
                _elements = other._elements;
                _size = other._size;

                other._elements = nullptr;
                other._size = 0;
            }
            return *this;
        }

        ~Vector() {
            delete[] _elements;
            _elements = nullptr;
            _size = 0;
        }

        void pop_back() {
            if (_size < 1)
                return;
            _size--;
            // resize(...)?
        }

        void push_back(const T& value) {
            if (_capacity == _size) {
                reallocate(_capacity > 0 ? _capacity : 1);
            }
            _elements[_size] = value;
            _size++;
        }

        void push_back(T&& value) {
            if (_capacity == _size) {
                reallocate(_capacity > 0 ? _capacity : 1);
            }
            _elements[_size] = std::move(value);
            _size++;
        }

        void insert(size_t index, const T& value) {
            if (_capacity == _size) {
                reallocate(_capacity);
            }

            for (size_t i = index + 1; i < _size; ++i) {
                _elements[i] = std::move(_elements[i - 1]);
            }

            _elements[index] = value;
            _size++;
        }

        void erase(size_t index) {
            if (index + 1 > _size || index < 0)
                return;
            _size--;
            for (size_t i = index; i < _size; ++i) {
                _elements[i] = _elements[i + 1];
            }
            // resize(...)?
        }

        void reserve(size_t new_cap) {
            if (new_cap <= _capacity) {
                return;
            }
            reallocate(new_cap - _capacity);
        }

        void resize(size_t new_size) {
            if (new_size == _size)
                return;
            reallocate(new_size - _capacity);
            _size = new_size;
        }

        void resize(size_t new_size, const T& value) {
            if (new_size == _size)
                return;
            reallocate(new_size - _capacity);
            for (size_t i = _size; i < new_size; ++i) {
                _elements[i] = value;
            }
            _size = new_size;
        }

        T& operator[](size_t index) {
            return _elements[index];
        }

        const T& operator[](size_t index) const {
            return _elements[index];
        }

        const T& at(int index) const {
            if (0 > index || index >= _size)
                throw std::out_of_range("Vector");
            return _elements[index];
        }

        const T& front() const {
            return _elements[0];
        }

        const T& back() const {
            return _elements[_size - 1];
        }

        void clear() {
            delete[] _elements;
            _elements = nullptr;
            _size = 0;
            _capacity = 0;
        }

        const size_t size() const { return _size; }

        const size_t capacity() const { return _capacity; }

        const bool empty() const {
            return _size == 0;
        }

    private:
        size_t _capacity = 0;
        size_t _size = 0;
        T* _elements = nullptr;

        void reallocate(int term) {
            if (term < 0 && _capacity < -term) {
                _capacity = 0;
            }
            else { 
                _capacity += term;
            }
            T* new_elements = new T[_capacity];
            for (size_t i = 0; i != _size && i != _capacity; ++i) {
                new_elements[i] = std::move(_elements[i]);
            }
            delete[] _elements;
            _elements = new_elements;
        }
    };
} // namespace

#endif // VECTOR_H
