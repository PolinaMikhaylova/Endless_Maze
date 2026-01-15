#pragma once
#include <stdexcept>
#include <algorithm>

template <class T>
class DynamicArray {
public:
    DynamicArray(int _size = 0)
        : size(_size), capacity(_size)
    {
        data = capacity ? new T[capacity] : nullptr;
    }

    DynamicArray(T* items, int count)
        : size(count), capacity(count)
    {
        data = new T[capacity];
        for (int i = 0; i < count; ++i)
            data[i] = items[i];
    }

    DynamicArray(const DynamicArray<T>& other)
        : size(other.size), capacity(other.capacity)
    {
        data = capacity ? new T[capacity] : nullptr;
        for (int i = 0; i < size; ++i)
            data[i] = other.data[i];
    }

    ~DynamicArray() {
        delete[] data;
    }

    // ===== ACCESS =====
    T Get(int index) const {
        if (index < 0 || index >= size)
            throw std::out_of_range("IndexOutOfRange");
        return data[index];
    }

    void Set(int index, const T& value) {
        if (index < 0 || index >= size)
            throw std::out_of_range("IndexOutOfRange");
        data[index] = value;
    }

    int GetSize() const { return size; }
    int GetCapacity() const { return capacity; }

    // ===== RESERVE =====
    void Reserve(int newCapacity) {
        if (newCapacity <= capacity)
            return;
        T* newData = new T[newCapacity];
        for (int i = 0; i < size; ++i)
            newData[i] = data[i];
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

    // ===== RESIZE =====
    void Resize(int newSize) {
        if (newSize > capacity)
            Reserve(std::max(newSize, capacity * 2 + 1));
        size = newSize;
    }

    // ===== ITERATOR SUPPORT =====
    T* begin() { return data; }
    T* end()   { return data + size; }
    const T* begin() const { return data; }
    const T* end()   const { return data + size; }

    T& operator[](int index) {
        if (index < 0 || index >= size)
            throw std::out_of_range("IndexOutOfRange");
        return data[index];
    }

    const T& operator[](int index) const {
        if (index < 0 || index >= size)
            throw std::out_of_range("IndexOutOfRange");
        return data[index];
    }


private:
    T* data = nullptr;
    int size = 0;
    int capacity = 0;
};
