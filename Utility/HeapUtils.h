// Copyright 2015 XLGAMES Inc.
//
// Distributed under the MIT License (See
// accompanying file "LICENSE" or the website
// http://www.opensource.org/licenses/mit-license.php)

#pragma once

#include "../Core/Types.h"
#include "Threading/Mutex.h"
#include <vector>
#include <algorithm>
#include <memory>
#include <limits>
#include <assert.h>

namespace Utility
{
    class LRUQueue
    {
    public:
        unsigned GetOldestValue() const;
        void BringToFront(unsigned value);
        void DisconnectOldest();

        LRUQueue(unsigned maxValues);
        LRUQueue();
        ~LRUQueue();

        LRUQueue(LRUQueue&& moveFrom) never_throws;
        LRUQueue& operator=(LRUQueue&& moveFrom) never_throws;
        LRUQueue(const LRUQueue& copyFrom);
        LRUQueue& operator=(const LRUQueue& copyFrom);
    protected:
        std::vector<std::pair<unsigned, unsigned>>  _lruQueue;
        unsigned    _oldestBlock;
        unsigned    _newestBlock;
    };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    enum class LRUCacheInsertType { Add, Update, EvictAndReplace, Fail };
    template<typename Type> class LRUCache
    {
    public:
        LRUCacheInsertType Insert(uint64 hashName, std::shared_ptr<Type> object);
        std::shared_ptr<Type>& Get(uint64 hashName);

        LRUCache(unsigned cacheSize);
        ~LRUCache();
    protected:
        std::vector<std::shared_ptr<Type>>   _objects;
        std::vector<std::pair<uint64, unsigned>> _lookupTable;
        LRUQueue _queue;
        unsigned _cacheSize;
    };

    template<typename Type>
        LRUCacheInsertType LRUCache<Type>::Insert(uint64 hashName, std::shared_ptr<Type> object)
    {
            // try to insert this object into the cache (if it's not already here)
        auto i = std::lower_bound(_lookupTable.cbegin(), _lookupTable.cend(), hashName, CompareFirst<uint64, unsigned>());
        if (i != _lookupTable.cend() && i->first == hashName) {
                // already here! But we should replace, this might be an update operation
            _objects[i->second] = object;
            return LRUCacheInsertType::Update;
        }

        if (_objects.size() < _cacheSize) {
            _objects.push_back(object);
            _lookupTable.insert(i, std::make_pair(hashName, unsigned(_objects.size()-1)));
            _queue.BringToFront(unsigned(_objects.size()-1));
            return LRUCacheInsertType::Add;
        }

            // we need to evict an existing object.
        // SelectedModel = nullptr;

        unsigned eviction = _queue.GetOldestValue();
        if (eviction == ~unsigned(0x0)) {
            assert(0); 
            return LRUCacheInsertType::Fail;
        }

        _objects[eviction] = object;
        auto oldLookup = std::find_if(_lookupTable.cbegin(), _lookupTable.cend(), 
            [=](const std::pair<uint64, unsigned>& p) { return p.second == eviction; });
        assert(oldLookup != _lookupTable.cend());
        _lookupTable.erase(oldLookup);

            // have to search again after the erase above
        i = std::lower_bound(_lookupTable.cbegin(), _lookupTable.cend(), hashName, CompareFirst<uint64, unsigned>());
        _lookupTable.insert(i, std::make_pair(hashName, eviction));

        _queue.BringToFront(eviction);
        return LRUCacheInsertType::EvictAndReplace;
    }

    template<typename Type>
        std::shared_ptr<Type>& LRUCache<Type>::Get(uint64 hashName)
    {
            // find the given object, and move it to the front of the queue
        auto i = std::lower_bound(_lookupTable.cbegin(), _lookupTable.cend(), hashName, CompareFirst<uint64, unsigned>());
        if (i != _lookupTable.cend() && i->first == hashName) {
            _queue.BringToFront(i->second);
            return _objects[i->second];
        }
        static std::shared_ptr<Type> dummy;
        return dummy;
    }

    template<typename Type>
        LRUCache<Type>::LRUCache(unsigned cacheSize)
    : _queue(cacheSize) 
    , _cacheSize(cacheSize)
    {
        _lookupTable.reserve(cacheSize);
        _objects.reserve(cacheSize);
    }

    template<typename Type>
        LRUCache<Type>::~LRUCache()
    {}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename Marker>
        class MarkerHeap
    {
    public:
        static Marker      ToInternalSize(unsigned size);
        static unsigned    ToExternalSize(Marker size);
        static unsigned    AlignSize(unsigned size);
    };

    class DefragStep
    {
    public:
        unsigned _sourceStart, _sourceEnd;
        unsigned _destination;
    };

    template <typename Marker>
        class SpanningHeap : public MarkerHeap<Marker>
    {
    public:

            //
            //      It's a simple heap that deals only in spans. It doesn't record the 
            //      size of the blocks allocated from within it. It just knows what space
            //      is allocated, and what is unallocated. The client must deallocate the
            //      correct space from the buffer when it's done.
            //

        unsigned            Allocate(unsigned size);
        bool                Allocate(unsigned ptr, unsigned size);
        bool                Deallocate(unsigned ptr, unsigned size);
        
        unsigned            CalculateAvailableSpace() const;
        unsigned            CalculateLargestFreeBlock() const;
        unsigned            CalculateAllocatedSpace() const;
        unsigned            CalculateHeapSize() const;
        uint64              CalculateHash() const;
        bool                IsEmpty() const;

        unsigned            AppendNewBlock(unsigned size);

        std::vector<unsigned>       CalculateMetrics() const;
        std::vector<DefragStep>     CalculateDefragSteps() const;
        void                        PerformDefrag(const std::vector<DefragStep>& defrag);

        std::pair<std::unique_ptr<uint8[]>, size_t> Flatten() const;

        SpanningHeap();
        SpanningHeap(unsigned size);
        SpanningHeap(const uint8 flattened[], size_t flattenedSize);
        ~SpanningHeap();

        SpanningHeap(SpanningHeap&& moveFrom) never_throws;
        const SpanningHeap& operator=(SpanningHeap&& moveFrom) never_throws;
        SpanningHeap(const SpanningHeap& cloneFrom);
        const SpanningHeap& operator=(const SpanningHeap& cloneFrom);
    protected:
        std::vector<Marker>         _markers;
        mutable Threading::Mutex    _lock;
        mutable bool                _largestFreeBlockValid;
        mutable Marker              _largestFreeBlock;

        Marker      CalculateLargestFreeBlock_Internal() const;
        bool        BlockAdjust_Internal(unsigned ptr, unsigned size, bool allocateOperation);
    };

    typedef SpanningHeap<uint16> SimpleSpanningHeap;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename Marker>
        inline auto MarkerHeap<Marker>::ToInternalSize(unsigned size) -> Marker
    {
        assert((size>>4) <= std::numeric_limits<Marker>::max());
        return Marker(size>>4); 
    }

    template <typename Marker>
        inline unsigned MarkerHeap<Marker>::ToExternalSize(Marker size)      
    {
        assert(size <= (std::numeric_limits<unsigned>::max()>>4));
        return unsigned(size)<<4; 
    }

    template <typename Marker>
        inline unsigned MarkerHeap<Marker>::AlignSize(unsigned size)
    {
        assert((size>>4) <= std::numeric_limits<Marker>::max());
        return (size&(~((1<<4)-1)))+((size&((1<<4)-1))?(1<<4):0); 
    }
}

using namespace Utility;
