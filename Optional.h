#pragma once
#include <stdexcept>

template<typename T>
class Optional
{
private:
    bool hasValue;
    T value;
public:
    Optional() {
        hasValue = false;
    }

    Optional(const T& val) {
        value = val;
        hasValue = true;
    }

    Optional(const Optional<T>& other) {
        hasValue = other.hasValue;
        if (hasValue)
            value = other.value;
    }

    Optional<T>& operator=(const Optional<T>& other) {
        if (this == &other)
            return *this;
        hasValue = other.hasValue;
        if (hasValue)
            value = other.value;
        return *this;
    }

    void SetValue(const T& val) {
        value = val;
        hasValue = true;
    }

    void Reset() {
        hasValue = false;
    }

    bool HasValue() const {
        return hasValue;
    }

    T& Value() {
        if (!hasValue)
            throw std::logic_error("Optional: no value present");
        return value;
    }

    const T& Value() const {
        if (!hasValue)
            throw std::logic_error("Optional: no value present");
        return value;
    }

    operator bool() const {
        return hasValue;
    }

    T& operator*() {
        return Value();
    }

    const T& operator*() const {
        return Value();
    }

    T* operator->() {
        if (!hasValue)
            throw std::logic_error("Optional: no value present");
        return &value;
    }

    const T* operator->() const {
        if (!hasValue)
            throw std::logic_error("Optional: no value present");
        return &value;
    }

    bool operator==(const Optional<T>& other) const {
        if (!hasValue && !other.hasValue)
            return true;
        if (hasValue != other.hasValue)
            return false;
        return value == other.value;
    }

    bool operator!=(const Optional<T>& other) const {
        return !(*this == other);
    }
};
