#pragma once
#include "Sequence.h"
#include "DynamicArray.h"
#include <stdexcept>
#include <iterator>
#include <utility>

template <class T>
class ArraySequence : public Sequence<T> {
public:
    class Iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        Iterator(pointer ptr = nullptr) : ptr(ptr) {}

        reference operator*() const { return *ptr; }
        pointer operator->() const { return ptr; }

        Iterator& operator++() { ++ptr; return *this; }
        Iterator operator++(int) { Iterator tmp(*this); ++ptr; return tmp; }
        Iterator& operator--() { --ptr; return *this; }
        Iterator operator--(int) { Iterator tmp(*this); --ptr; return tmp; }

        Iterator operator+(difference_type n) const { return Iterator(ptr + n); }
        Iterator operator-(difference_type n) const { return Iterator(ptr - n); }
        difference_type operator-(const Iterator& other) const { return ptr - other.ptr; }

        bool operator==(const Iterator& other) const { return ptr == other.ptr; }
        bool operator!=(const Iterator& other) const { return ptr != other.ptr; }
        bool operator<(const Iterator& other) const { return ptr < other.ptr; }

    private:
        pointer ptr;
    };

    ArraySequence() {
        data = new DynamicArray<T>(0);
    }

    ArraySequence(T* items, int count) {
        data = new DynamicArray<T>(items, count);
    }

    ArraySequence(const ArraySequence<T>& other) {
        data = new DynamicArray<T>(*other.data);
    }

    ArraySequence(ArraySequence<T>&& other) noexcept {
        data = other.data;
        other.data = nullptr;
    }

    ArraySequence<T>& operator=(const ArraySequence<T>& other)
    {
        if (this == &other)
            return *this;

        delete data;
        data = new DynamicArray<T>(*other.data);
        return *this;
    }


    Iterator begin() { return Iterator(data ? data->begin() : nullptr); }
    Iterator end()   { return Iterator(data ? data->end()   : nullptr); }

    Iterator begin() const { return Iterator(data ? data->begin() : nullptr); }
    Iterator end()   const { return Iterator(data ? data->end()   : nullptr); }

    T GetFirst() override {
        if (GetLength() == 0)
            throw std::out_of_range("IndexOutOfRange");
        return data->Get(0);
    }

    T GetLast() override {
        if (GetLength() == 0)
            throw std::out_of_range("IndexOutOfRange");
        return data->Get(GetLength() - 1);
    }

    T Get(int index) const override {
        if (index < 0 || index >= GetLength())
            throw std::out_of_range("IndexOutOfRange");
        return data->Get(index);
    }

    ArraySequence<T>* GetSubsequence(int startIndex, int endIndex) override {
        if (startIndex < 0 || endIndex < 0 ||
            startIndex >= GetLength() ||
            endIndex >= GetLength())
            throw std::out_of_range("IndexOutOfRange");

        auto* other = new ArraySequence<T>();
        for (int i = startIndex; i <= endIndex; ++i)
            other->Append(data->Get(i));
        return other;
    }

    int GetLength() const override {
        return data ? data->GetSize() : 0;
    }

    ArraySequence<T>* Append(T item) override {
        data->Resize(GetLength() + 1);
        data->Set(GetLength() - 1, item);
        return this;
    }

    ArraySequence<T>* Prepend(T item) override {
        data->Resize(GetLength() + 1);
        for (int i = GetLength() - 2; i >= 0; --i)
            data->Set(i + 1, data->Get(i));
        data->Set(0, item);
        return this;
    }

    ArraySequence<T>* InsertAt(T item, int index) override {
        if (index < 0 || index >= GetLength())
            throw std::out_of_range("IndexOutOfRange");

        data->Resize(GetLength() + 1);
        for (int i = GetLength() - 2; i >= index; --i)
            data->Set(i + 1, data->Get(i));
        data->Set(index, item);
        return this;
    }

    Sequence<T>* Concat(Sequence<T>* list) override {
        int oldSize = GetLength();
        data->Resize(oldSize + list->GetLength());
        for (int i = 0; i < list->GetLength(); ++i)
            data->Set(oldSize + i, list->Get(i));
        return this;
    }

    void Clear() {
        delete data;
        data = new DynamicArray<T>(0);
    }

    void Reserve(int capacity) {
        if (!data)
            data = new DynamicArray<T>(0);
        data->Reserve(capacity);
    }

    T& operator[](int index) {
        if (index < 0 || index >= GetLength())
            throw std::out_of_range("IndexOutOfRange");
        return (*data)[index];
    }

    const T& operator[](int index) const {
        if (index < 0 || index >= GetLength())
            throw std::out_of_range("IndexOutOfRange");
        return (*data)[index];
    }

    void swap(ArraySequence<T>& other)
    {
        std::swap(data, other.data);
    }


private:
    DynamicArray<T>* data = nullptr;
};
