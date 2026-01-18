#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <sstream>
#include "Sequence.h"
#include "ArraySequence.h"
#include "Optional.h"

using namespace std;

enum class Direction { Forward, Backward };

template<typename T>
class LazySequence;

template<typename T>
class Generator {
private:
    LazySequence<T>* owner;
    std::function<T(Sequence<T>*)> rule;
    size_t arity;
    unique_ptr<Sequence<T>> buffer;
    Direction direction;
    bool infinite = true;

    template<typename> friend class LazySequence;

public:
    Generator(LazySequence<T>* owner,
              std::function<T(Sequence<T>*)> rule,
              size_t arity,
              Direction dir = Direction::Forward)
        : owner(owner), rule(rule), arity(arity), direction(dir)
    {
        buffer = make_unique<ArraySequence<T>>();
    }

    void InitBuffer(const Sequence<T>* initSeq) {
        buffer = make_unique<ArraySequence<T>>();
        int len = initSeq->GetLength();
        int start = (int)max(0, len - (int)arity);
        for (int i = start; i < len; ++i)
            buffer->Append(initSeq->Get(i));
    }

    Optional<T> TryGetNext() {
        if (!owner || !infinite)
            return Optional<T>();

        Sequence<T>* materialized = owner->GetMaterializedSequence();
        T nextVal = rule(materialized);

        if (direction == Direction::Forward) {
            owner->MaterializeAppend(nextVal);
            if (buffer->GetLength() == (int)arity) {
                auto newBuf = make_unique<ArraySequence<T>>();
                for (int i = 1; i < buffer->GetLength(); ++i)
                    newBuf->Append(buffer->Get(i));
                newBuf->Append(nextVal);
                buffer = move(newBuf);
            } else {
                buffer->Append(nextVal);
            }
        } else {
            owner->MaterializePrepend(nextVal);
            if (buffer->GetLength() == (int)arity) {
                auto newBuf = make_unique<ArraySequence<T>>();
                newBuf->Append(nextVal);
                for (int i = 0; i < buffer->GetLength() - 1; ++i)
                    newBuf->Append(buffer->Get(i));
                buffer = move(newBuf);
            } else {
                buffer->Prepend(nextVal);
            }
        }

        return Optional<T>(nextVal);
    }

    std::function<T(Sequence<T>*)> GetRule() const { return rule; }
    size_t GetArity() const { return arity; }
};

template<typename T>
class LazySequence {
private:
    unique_ptr<Sequence<T>> items;
    Generator<T>* forwardGen;
    Generator<T>* backwardGen;
    mutable int zeroIndex;

public:
    LazySequence() {
        items = make_unique<ArraySequence<T>>();
        forwardGen = nullptr;
        backwardGen = nullptr;
        zeroIndex = 0;
    }

    LazySequence(T* data, int count) {
        items = make_unique<ArraySequence<T>>();
        for (int i = 0; i < count; ++i)
            items->Append(data[i]);
        forwardGen = nullptr;
        backwardGen = nullptr;
        zeroIndex = 0;
    }

    LazySequence(unique_ptr<Sequence<T>> seq) {
        items = move(seq);
        forwardGen = nullptr;
        backwardGen = nullptr;
        zeroIndex = 0;
    }

    LazySequence(std::function<T(Sequence<T>*)> forwardRule,
                 unique_ptr<Sequence<T>> startItems,
                 size_t arity,
                 std::function<T(Sequence<T>*)> backwardRule = nullptr)
    {
        items = move(startItems);
        zeroIndex = items->GetLength() > 0 ? 0 : -1;
        forwardGen = new Generator<T>(this, forwardRule, arity, Direction::Forward);
        forwardGen->InitBuffer(items.get());
        if (backwardRule != nullptr) {
            backwardGen = new Generator<T>(this, backwardRule, arity, Direction::Backward);
            backwardGen->InitBuffer(items.get());
        } else backwardGen = nullptr;
    }

    LazySequence(const LazySequence<T>& other)
    {
        items = make_unique<ArraySequence<T>>();
        for (int i = 0; i < other.items->GetLength(); ++i)
            items->Append(other.items->Get(i));

        zeroIndex = other.zeroIndex;

        if (other.forwardGen != nullptr) {
            forwardGen = new Generator<T>(
                this,
                other.forwardGen->GetRule(),
                other.forwardGen->GetArity(),
                Direction::Forward
                );
            forwardGen->InitBuffer(items.get());
        } else {
            forwardGen = nullptr;
        }

        if (other.backwardGen != nullptr) {
            backwardGen = new Generator<T>(
                this,
                other.backwardGen->GetRule(),
                other.backwardGen->GetArity(),
                Direction::Backward
                );
            backwardGen->InitBuffer(items.get());
        } else {
            backwardGen = nullptr;
        }
    }

    ~LazySequence() {
        delete forwardGen;
        delete backwardGen;
    }

    int GetLength() const {
        return items->GetLength();
    }

    T GetFirst() {
        if (items->GetLength() == 0)
            throw out_of_range("Empty sequence");
        if (!backwardGen)
            return items->GetFirst();
        while (true)
            backwardGen->TryGetNext();
    }

    T GetLast() {
        if (!forwardGen)
            return items->GetLast();
        while (true)
            forwardGen->TryGetNext();
    }

    T Get(int index) const {
        int L = items->GetLength();
        int z = zeroIndex;
        if (index >= -z && index <= L - 1 - z)
            return items->Get(index + z);

        if (index >= 0) {
            while (index > L - 1 - z) {
                if (!forwardGen) throw out_of_range("No forward generator available");
                forwardGen->TryGetNext();
                L = items->GetLength();
            }
            return items->Get(index + z);
        } else {
            int needed = -index - 1;
            while (needed >= z) {
                if (!backwardGen) throw out_of_range("No backward generator available");
                backwardGen->TryGetNext();
                L = items->GetLength();
                z = zeroIndex;
            }
            return items->Get(index + z);
        }
    }


    shared_ptr<LazySequence<T>> Append(T item) {
        auto start = make_unique<ArraySequence<T>>();
        int mat = GetMaterializedCount();
        for (int i = 0; i < mat; ++i) start->Append(Get(i));
        start->Append(item);
        auto rule = [self = shared_ptr<LazySequence<T>>(this, [](LazySequence<T>*) {})](Sequence<T>* prev) -> T {
            int n = prev->GetLength();
            return self->Get(n);
        };
        return make_shared<LazySequence<T>>(rule, move(start), 1);
    }

    shared_ptr<LazySequence<T>> Prepend(T item) {
        auto start = make_unique<ArraySequence<T>>();
        start->Append(item);
        int mat = GetMaterializedCount();
        for (int i = 0; i < mat; ++i) start->Append(Get(i));
        auto rule = [self = shared_ptr<LazySequence<T>>(this, [](LazySequence<T>*) {})](Sequence<T>* prev) -> T {
            int n = prev->GetLength();
            return self->Get(n - 1);
        };
        return make_shared<LazySequence<T>>(rule, move(start), 1);
    }

    shared_ptr<LazySequence<T>> InsertAt(T item, int index) {
        auto start = make_unique<ArraySequence<T>>();
        int mat = GetMaterializedCount();

        if (index >= 0) {
            if (index > mat) {
                for (int i = mat; i <= index; ++i) (void)Get(i);
            }
            for (int i = 0; i < index; ++i) start->Append(Get(i));
            start->Append(item);
            for (int i = index; i < mat; ++i) start->Append(Get(i));
        } else {
            int target = zeroIndex + index;
            while (target < 0) {
                if (!backwardGen) throw out_of_range("Cannot insert before generated range");
                backwardGen->TryGetNext();
                target = zeroIndex + index;
            }
            for (int i = 0; i < target; ++i) start->Append(Get(i - zeroIndex));
            start->Append(item);
            for (int i = target; i < mat; ++i) start->Append(Get(i - zeroIndex));
        }

        auto rule = [self = shared_ptr<LazySequence<T>>(this, [](LazySequence<T>*) {})](Sequence<T>* prev) -> T {
            int n = prev->GetLength();
            return self->Get(n - 1);
        };

        return make_shared<LazySequence<T>>(rule, move(start), 1);
    }

    shared_ptr<LazySequence<T>> Concat(shared_ptr<LazySequence<T>> other) {
        auto start = make_unique<ArraySequence<T>>();
        int matA = GetMaterializedCount();
        for (int i = 0; i < matA; ++i) start->Append(Get(i));
        auto rule = [self = shared_ptr<LazySequence<T>>(this, [](LazySequence<T>*) {}), other](Sequence<T>* prev) -> T {
            int n = prev->GetLength();
            int aLen = self->GetMaterializedCount();
            if (n < aLen)
                return self->Get(n);
            return other->Get(n - aLen);
        };
        return make_shared<LazySequence<T>>(rule, move(start), 1);
    }

    shared_ptr<LazySequence<T>> GetSubsequence(int startIndex, int endIndex) {
        auto start = make_unique<ArraySequence<T>>();
        int mat = GetMaterializedCount();
        for (int i = startIndex; i <= min(endIndex, mat - 1); ++i)
            start->Append(Get(i));
        auto rule = [self = shared_ptr<LazySequence<T>>(this, [](LazySequence<T>*) {}), startIndex, endIndex](Sequence<T>* prev) -> T {
            int k = prev->GetLength();
            int idx = startIndex + k;
            if (endIndex >= 0 && idx > endIndex) throw out_of_range("Subsequence end reached");
            return self->Get(idx);
        };
        return make_shared<LazySequence<T>>(rule, move(start), 1);
    }

    Sequence<T>* GetSubsequenceStrict(int startIndex, int endIndex) {
        if (startIndex > endIndex)
            throw std::out_of_range("Invalid subsequence interval");
        auto* result = new ArraySequence<T>();
        for (int i = startIndex; i <= endIndex; ++i)
            result->Append(Get(i));
        return result;
    }

    void MaterializeAppend(const T& value) {
        items->Append(value);
    }
    void MaterializePrepend(const T& value) {
        items->Prepend(value);
        zeroIndex++;
    }

    template<typename T2>
    shared_ptr<LazySequence<T2>> Map(std::function<T2(T)> f) {
        auto mapper = [f](Sequence<T>* seq) -> T2 {
            return f(seq->Get(seq->GetLength() - 1));
        };

        auto start = make_unique<ArraySequence<T2>>();
        for (int i = 0; i < (int)GetMaterializedCount(); ++i)
            start->Append(f(Get(i)));

        return make_shared<LazySequence<T2>>(mapper, move(start), 1);
    }

    template<typename T2>
    T2 Reduce(std::function<T2(T2, T)> f, T2 init) {
        T2 acc = init;
        for (int i = 0; i < (int)GetMaterializedCount(); ++i)
            acc = f(acc, Get(i));
        return acc;
    }

    shared_ptr<LazySequence<T>> Where(std::function<bool(T)> pred) {
        auto result = make_shared<LazySequence<T>>();
        for (int i = 0; i < (int)GetMaterializedCount(); ++i)
            if (pred(Get(i))) result->Append(Get(i));
        return result;
    }

    template<typename U>
    shared_ptr<LazySequence<pair<T, U>>> Zip(shared_ptr<LazySequence<U>> other) {
        auto start = make_unique<ArraySequence<pair<T, U>>>();
        int minMat = min(GetMaterializedCount(), other->GetMaterializedCount());
        for (int i = 0; i < minMat; ++i)
            start->Append({ Get(i), other->Get(i) });

        auto rule = [self = shared_ptr<LazySequence<T>>(this, [](LazySequence<T>*) {}), other](Sequence<pair<T, U>>* prev) -> pair<T, U> {
            int n = prev->GetLength();
            T a = self->Get(n);
            U b = other->Get(n);
            return { a, b };
        };

        return make_shared<LazySequence<pair<T, U>>>(rule, move(start), 1);
    }

    Sequence<T>* GetMaterializedSequence() {
        return items.get();
    }

    int GetMaterializedCount() const {
        return items->GetLength();
    }

    std::string ToString(int from, int to) {
        std::ostringstream out;
        for (int i = from; i <= to; ++i) {
            try {
                out << Get(i);
            } catch (const std::exception&) {
                out << "?";
            }
            if (i < to) out << " ";
        }
        return out.str();
    }
    void swap(LazySequence& other) noexcept {
        std::swap(items, other.items);
        std::swap(forwardGen, other.forwardGen);
        std::swap(backwardGen, other.backwardGen);
        std::swap(zeroIndex, other.zeroIndex);

        if (forwardGen) forwardGen->owner = this;
        if (backwardGen) backwardGen->owner = this;
        if (other.forwardGen) other.forwardGen->owner = &other;
        if (other.backwardGen) other.backwardGen->owner = &other;
    }

    LazySequence<T>& operator=(LazySequence<T> other) {
        swap(other);
        return *this;
    }

};

