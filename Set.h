/*
 *      Summary: AA Tree
 *         Date: 2022.01.30
 *   Programmer: Kurdun Andrei
 *   Code Style: Yandex
 */
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <algorithm>

template<class TValueType>
struct TNode;
template<class TValueType>
class TIterator;

// Set_ based on AA tree structure
template<class TValueType>
class Set {
public:
    Set() = default;

    Set(Set& set) {
        for(auto currentValue : set) {
            insert(currentValue);
        }
    }

    Set(const std::initializer_list<TValueType>& initializerList) {
        for (const auto& currentValue : initializerList) {
            Root_ = Insert(Root_, currentValue);
        }
    }

    template<typename Iterator>
    Set(Iterator first, Iterator last) {
        while (first != last) {
            Root_ = Insert(Root_, *first);
            ++first;
        }
    }

    ~Set() {
        if (Root_ != nullptr) {
            Root_->Clear();
        }
    }

    Set& operator=(const Set& set);

    using iterator = TIterator<TValueType>;

    friend struct TNode<TValueType>;
    friend class TIterator<TValueType>;

    inline size_t size() const;
    inline bool empty() const;

    iterator begin() const;
    iterator end() const;

    iterator lower_bound(const TValueType& wantedValue) const;
    iterator find(const TValueType& wantedValue) const;

    void insert(const TValueType& insertedValue);
    void erase(const TValueType& erasedValue);

protected:
    static inline TNode<TValueType>* Predecessor(TNode<TValueType>* currentNode);
    static inline TNode<TValueType>* Successor(TNode<TValueType>* currentNode);
    static inline bool IsLeaf(TNode<TValueType>* currentNode);

private:
    inline TNode<TValueType>* Insert(TNode<TValueType>* currentNode, const TValueType& insertedValue);
    inline TNode<TValueType>* Erase(TNode<TValueType>* currentNode, const TValueType& erasedValue);
    static inline TNode<TValueType>* Skew(TNode<TValueType>* currentNode);
    static inline TNode<TValueType>* Split(TNode<TValueType>* currentNode);
    static inline TNode<TValueType>* DecreaseLevel(TNode<TValueType>* currentNode);

private:
    TNode<TValueType>* Root_ = nullptr;
    size_t Size_ = 0;
};

template<class TValueType>
Set<TValueType>& Set<TValueType>::operator=(const Set& set) {
    if (this == &set) {
        return *this;
    }

    if(Root_ != nullptr) {
        Root_->Clear();
        Root_ = nullptr;
        Size_ = 0;
    }

    for(auto currentValue : set) {
        insert(currentValue);
    }

    return *this;
}

template<class TValueType>
size_t Set<TValueType>::size() const {
    return Size_;
}

template<class TValueType>
bool Set<TValueType>::empty() const {
    return (Size_ == 0);
}

template<class TValueType>
typename Set<TValueType>::iterator Set<TValueType>::begin() const {
    if (Root_ == nullptr) {
        return end();
    }
    TNode<TValueType>* currentNode = Root_;
    while (currentNode->LeftNode != nullptr) {
        currentNode = currentNode->LeftNode;
    }
    return iterator(this, currentNode);
}

template<class TValueType>
typename Set<TValueType>::iterator Set<TValueType>::end() const {
    return iterator(this);
}

template<class TValueType>
typename Set<TValueType>::iterator Set<TValueType>::lower_bound(const TValueType& wantedValue) const {
    TNode<TValueType>* currentNode = Root_;
    while(currentNode != nullptr) {
        if(currentNode->Value < wantedValue) {
            if (currentNode->RightNode == nullptr) {
                return ++iterator(this, currentNode);
            }
            currentNode = currentNode->RightNode;
        } else {
            if (currentNode->LeftNode != nullptr && wantedValue < currentNode->Value) {
                currentNode = currentNode->LeftNode;
            } else {
                break;
            }
        }
    }
    return iterator(this, currentNode);
}

template<class TValueType>
typename Set<TValueType>::iterator Set<TValueType>::find(const TValueType& wantedValue) const {
    iterator iter = lower_bound(wantedValue);
    if (iter != end() && (!(*iter < wantedValue) && !(wantedValue < *iter))) {
        return iter;
    }
    return end();
}

template<class TValueType>
void Set<TValueType>::insert(const TValueType& insertedValue) {
    Root_ = Insert(Root_, insertedValue);
}

template<class TValueType>
void Set<TValueType>::erase(const TValueType& erasedValue) {
    Root_ = Erase(Root_, erasedValue);
}

/*
 *  ==================================================================================
 *                                    Skew(node D)
 *  ==================================================================================
 *
 *                             |                        |
 *                             |                        |
 *        node B <—————————— node D                   node B  ——————————> node D
 *        /   \                  \         ==>        /                   /   \
 *       /     \                  \                  /                   /     \
 *      /       \                  \                /                   /       \
 *   node A    node C            node F           node A             node C    node F
 *  ==================================================================================
 *                             Elimination of the left son
 *  ==================================================================================
 */
template<class TValueType>
TNode<TValueType>* Set<TValueType>::Skew(TNode<TValueType>* currentNode) {
    if (
           currentNode == nullptr
        || currentNode->LeftNode == nullptr
        || currentNode->Level != currentNode->LeftNode->Level
    ) {
        return currentNode;
    }

    auto* bufferNode = new TNode<TValueType>(
          currentNode->LeftNode->Value
        , currentNode->LeftNode->Level
        , currentNode->PreviousNode
        , currentNode->LeftNode->LeftNode
        , new TNode<TValueType>(
              currentNode->Value
            , currentNode->Level
            , nullptr
            , currentNode->LeftNode->RightNode
            , currentNode->RightNode
        )
    );
    bufferNode->RightNode->PreviousNode = bufferNode;
    if (bufferNode->RightNode->LeftNode != nullptr) {
        bufferNode->RightNode->LeftNode->PreviousNode = bufferNode->RightNode;
    }
    if (bufferNode->RightNode->RightNode != nullptr) {
        bufferNode->RightNode->RightNode->PreviousNode = bufferNode->RightNode;
    }
    if (bufferNode->LeftNode != nullptr) {
        bufferNode->LeftNode->PreviousNode = bufferNode;
    }
    delete currentNode->LeftNode;
    delete currentNode;

    return bufferNode;
}

/*
 *  ==================================================================================
 *                                    Split(node B)
 *  ==================================================================================
 *
 *                                                                     |
 *          |                                                          |
 *          |                                                        node C
 *        node B  ——> node C  ——> node E                             /   \
 *        /           /                         ==>                 /     \
 *       /           /                                             /       \
 *      /           /                                           node B    node E
 *   node A      node D                                         /   \
 *                                                             /     \
 *                                                            /       \
 *                                                         node A    node D
 *  ==================================================================================
 *                 Elimination of two consecutive right horizontal edges
 *  ==================================================================================
 */
template<class TValueType>
TNode<TValueType>* Set<TValueType>::Split(TNode<TValueType>* currentNode) {
    if (
           currentNode == nullptr
        || currentNode->RightNode == nullptr
        || currentNode->RightNode->RightNode == nullptr
        || currentNode->Level != currentNode->RightNode->RightNode->Level
    ) {
        return currentNode;
    }

    auto* bufferNode = new TNode<TValueType>(
          currentNode->RightNode->Value
        , currentNode->RightNode->Level + 1
        , currentNode->PreviousNode
        , new TNode<TValueType>(
              currentNode->Value
            , currentNode->Level
            , nullptr
            , currentNode->LeftNode
            , currentNode->RightNode->LeftNode
        )
        , currentNode->RightNode->RightNode
    );
    bufferNode->LeftNode->PreviousNode = bufferNode;
    if (bufferNode->RightNode != nullptr) {
        bufferNode->RightNode->PreviousNode = bufferNode;
    }
    if (bufferNode->LeftNode->RightNode != nullptr) {
        bufferNode->LeftNode->RightNode->PreviousNode = bufferNode->LeftNode;
    }
    if (bufferNode->LeftNode->LeftNode != nullptr) {
        bufferNode->LeftNode->LeftNode->PreviousNode = bufferNode->LeftNode;
    }
    delete currentNode->RightNode;
    delete currentNode;

    return bufferNode;
}

template<class TValueType>
TNode<TValueType>* Set<TValueType>::Insert(TNode<TValueType>* currentNode, const TValueType& insertedValue) {
    if (currentNode == nullptr) {
        ++Size_;
        return new TNode<TValueType>(
              insertedValue
            , 1
            , nullptr
            , nullptr
            , nullptr
        );
    }
    if (insertedValue < currentNode->Value) {
        currentNode->LeftNode = Insert(currentNode->LeftNode, insertedValue);
        if (currentNode->LeftNode != nullptr) {
            currentNode->LeftNode->PreviousNode = currentNode;
        }
    }
    if (currentNode->Value < insertedValue) {
        currentNode->RightNode = Insert(currentNode->RightNode, insertedValue);
        if (currentNode->RightNode != nullptr) {
            currentNode->RightNode->PreviousNode = currentNode;
        }
    }
    currentNode = Skew(currentNode);
    currentNode = Split(currentNode);
    return currentNode;
}

template<class TValueType>
TNode<TValueType>* Set<TValueType>::Erase(TNode<TValueType>* currentNode, const TValueType& erasedValue) {
    if (currentNode == nullptr) {
        return currentNode;
    }
    if (erasedValue < currentNode->Value) {
        currentNode->LeftNode = Erase(currentNode->LeftNode, erasedValue);
        if (currentNode->LeftNode != nullptr) {
            currentNode->LeftNode->PreviousNode = currentNode;
        }
    } else {
        if (currentNode->Value < erasedValue) {
            currentNode->RightNode = Erase(currentNode->RightNode, erasedValue);
            if (currentNode->RightNode != nullptr) {
                currentNode->RightNode->PreviousNode = currentNode;
            }
        } else {
            if (IsLeaf(currentNode)) {
                delete currentNode;
                --Size_;
                return nullptr;
            }
            if (currentNode->LeftNode == nullptr) {
                TValueType successorValue = Successor(currentNode)->Value;
                currentNode->RightNode = Erase(currentNode->RightNode, successorValue);
                if (currentNode->RightNode != nullptr) {
                    currentNode->RightNode->PreviousNode = currentNode;
                }
                currentNode->Value = successorValue;
            } else {
                TValueType predecessorValue = Predecessor(currentNode)->Value;
                currentNode->LeftNode = Erase(currentNode->LeftNode, predecessorValue);
                if (currentNode->LeftNode != nullptr) {
                    currentNode->LeftNode->PreviousNode = currentNode;
                }
                currentNode->Value = predecessorValue;
            }
        }
    }

    currentNode = DecreaseLevel(currentNode);
    currentNode = Skew(currentNode);
    currentNode->RightNode = Skew(currentNode->RightNode);
    if (currentNode->RightNode != nullptr) {
        currentNode->RightNode->RightNode = Skew(currentNode->RightNode->RightNode);
    }
    currentNode = Split(currentNode);
    currentNode->RightNode = Split(currentNode->RightNode);
    return currentNode;
}

template<class TValueType>
TNode<TValueType>* Set<TValueType>::DecreaseLevel(TNode<TValueType>* currentNode) {
    if (currentNode->LeftNode != nullptr && currentNode->RightNode != nullptr) {
        uint32_t expectedLevel = std::min(currentNode->LeftNode->Level, currentNode->RightNode->Level) + 1;
        if (expectedLevel < currentNode->Level) {
            currentNode->Level = expectedLevel;
            if (expectedLevel < currentNode->RightNode->Level) {
                currentNode->RightNode->Level = expectedLevel;
            }
        }
    }
    return currentNode;
}

template<class TValueType>
TNode<TValueType>* Set<TValueType>::Predecessor(TNode<TValueType>* currentNode) {
    currentNode = currentNode->LeftNode;
    while (currentNode->RightNode != nullptr) {
        currentNode = currentNode->RightNode;
    }
    return currentNode;
}

template<class TValueType>
TNode<TValueType>* Set<TValueType>::Successor(TNode<TValueType>* currentNode) {
    currentNode = currentNode->RightNode;
    while (currentNode->LeftNode != nullptr) {
        currentNode = currentNode->LeftNode;
    }
    return currentNode;
}

template<class TValueType>
bool Set<TValueType>::IsLeaf(TNode<TValueType>* currentNode) {
    return (currentNode != nullptr && currentNode->LeftNode == nullptr && currentNode->RightNode == nullptr);
}

//----------------TNode----------------

template<class TValueType>
struct TNode : public Set<TValueType> {
    TNode(
          TValueType value
        , uint32_t level
        , TNode* previousNode
        , TNode* leftNode
        , TNode* rightNode
    )
        : Value(value)
        , Level(level)
        , PreviousNode(previousNode)
        , LeftNode(leftNode)
        , RightNode(rightNode)
    {
    }

    ~TNode() = default;

    inline void Clear();

    TValueType Value;
    uint32_t Level = 0;
    TNode* PreviousNode = nullptr;
    TNode* LeftNode = nullptr;
    TNode* RightNode = nullptr;
};

template<class TValueType>
void TNode<TValueType>::Clear() {
    if (LeftNode != nullptr) {
        LeftNode->Clear();
    }
    if (RightNode != nullptr) {
        RightNode->Clear();
    }
    delete this;
}

//----------------TIterator----------------

template<class TValueType>
class TIterator {
public:
    TIterator() = default;

    explicit TIterator(const Set<TValueType>* set) : Set_(set) {
    }

    TIterator(const Set<TValueType>* set, TNode<TValueType>* iteratorNode) : Set_(set), IteratorNode_(iteratorNode) {
    }

    TIterator& operator=(const TIterator& iter);

    TIterator& operator++();
    const TIterator operator++(int);

    TIterator& operator--();
    const TIterator operator--(int);

    constexpr inline TValueType& operator*() const;
    constexpr inline TValueType* operator->() const;

    constexpr inline bool operator==(const TIterator& iter) const;
    constexpr inline bool operator!=(const TIterator& iter) const;

private:
    const Set<TValueType>* Set_ = nullptr;
    TNode<TValueType>* IteratorNode_ = nullptr;
};

template<class TValueType>
TIterator<TValueType>& TIterator<TValueType>::operator=(const TIterator<TValueType>& iter) {
    if (this == &iter) {
        return *this;
    }

    Set_ = iter.Set_;
    IteratorNode_ = iter.IteratorNode_;

    return *this;
}

template<class TValueType>
TIterator<TValueType>& TIterator<TValueType>::operator++() {
    if (IteratorNode_->RightNode != nullptr) {
        IteratorNode_ = ::Set<TValueType>::Successor(IteratorNode_);
    } else {
        while (IteratorNode_->PreviousNode != nullptr && IteratorNode_->PreviousNode->Value < IteratorNode_->Value) {
            IteratorNode_ = IteratorNode_->PreviousNode;
        }
        IteratorNode_ = IteratorNode_->PreviousNode;
    }
    return *this;
}

template<class TValueType>
const TIterator<TValueType> TIterator<TValueType>::operator++(int) {
    TIterator oldIter = *this;
    operator++();
    return oldIter;
}

template<class TValueType>
TIterator<TValueType>& TIterator<TValueType>::operator--() {
    if (IteratorNode_ == nullptr) {
        IteratorNode_ = Set_->Root_;
        if (IteratorNode_ != nullptr) {
            while (IteratorNode_->RightNode != nullptr) {
                IteratorNode_ = IteratorNode_->RightNode;
            }
        }
        return *this;
    }
    if (IteratorNode_->LeftNode != nullptr) {
        IteratorNode_ = ::Set<TValueType>::Predecessor(IteratorNode_);
    } else {
        while (IteratorNode_->PreviousNode != nullptr && IteratorNode_->Value < IteratorNode_->PreviousNode->Value) {
            IteratorNode_ = IteratorNode_->PreviousNode;
        }
        IteratorNode_ = IteratorNode_->PreviousNode;
    }
    return *this;
}

template<class TValueType>
const TIterator<TValueType> TIterator<TValueType>::operator--(int) {
    TIterator oldIter = *this;
    operator--();
    return oldIter;
}

template<class TValueType>
constexpr TValueType& TIterator<TValueType>::operator*() const {
    return IteratorNode_->Value;
}

template<class TValueType>
constexpr TValueType* TIterator<TValueType>::operator->() const {
    return &IteratorNode_->Value;
}

template<class TValueType>
constexpr bool TIterator<TValueType>::operator==(const TIterator<TValueType>& iter) const {
    return (Set_ == iter.Set_ && IteratorNode_ == iter.IteratorNode_);
}

template<class TValueType>
constexpr bool TIterator<TValueType>::operator!=(const TIterator<TValueType>& iter) const {
    return (Set_ != iter.Set_ || IteratorNode_ != iter.IteratorNode_);
}
