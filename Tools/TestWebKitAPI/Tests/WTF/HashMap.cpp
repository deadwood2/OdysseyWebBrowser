/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "Counters.h"
#include "DeletedAddressOfOperator.h"
#include "MoveOnly.h"
#include "RefLogger.h"
#include "Test.h"
#include <string>
#include <wtf/HashMap.h>
#include <wtf/Ref.h>
#include <wtf/text/StringHash.h>

namespace TestWebKitAPI {

typedef WTF::HashMap<int, int> IntHashMap;

TEST(WTF_HashMap, HashTableIteratorComparison)
{
    IntHashMap map;
    map.add(1, 2);
    ASSERT_TRUE(map.begin() != map.end());
    ASSERT_FALSE(map.begin() == map.end());

    IntHashMap::const_iterator begin = map.begin();
    ASSERT_TRUE(begin == map.begin());
    ASSERT_TRUE(map.begin() == begin);
    ASSERT_TRUE(begin != map.end());
    ASSERT_TRUE(map.end() != begin);
    ASSERT_FALSE(begin != map.begin());
    ASSERT_FALSE(map.begin() != begin);
    ASSERT_FALSE(begin == map.end());
    ASSERT_FALSE(map.end() == begin);
}

struct TestDoubleHashTraits : HashTraits<double> {
    static const int minimumTableSize = 8;
};

typedef HashMap<double, int64_t, DefaultHash<double>::Hash, TestDoubleHashTraits> DoubleHashMap;

static int bucketForKey(double key)
{
    return DefaultHash<double>::Hash::hash(key) & (TestDoubleHashTraits::minimumTableSize - 1);
}

TEST(WTF_HashMap, DoubleHashCollisions)
{
    // The "clobber" key here is one that ends up stealing the bucket that the -0 key
    // originally wants to be in. This makes the 0 and -0 keys collide and the test then
    // fails unless the FloatHash::equals() implementation can distinguish them.
    const double clobberKey = 6;
    const double zeroKey = 0;
    const double negativeZeroKey = -zeroKey;

    DoubleHashMap map;

    map.add(clobberKey, 1);
    map.add(zeroKey, 2);
    map.add(negativeZeroKey, 3);

    ASSERT_EQ(bucketForKey(clobberKey), bucketForKey(negativeZeroKey));
    ASSERT_EQ(map.get(clobberKey), 1);
    ASSERT_EQ(map.get(zeroKey), 2);
    ASSERT_EQ(map.get(negativeZeroKey), 3);
}

TEST(WTF_HashMap, MoveOnlyValues)
{
    HashMap<unsigned, MoveOnly> moveOnlyValues;

    for (size_t i = 0; i < 100; ++i) {
        MoveOnly moveOnly(i + 1);
        moveOnlyValues.set(i + 1, WTFMove(moveOnly));
    }

    for (size_t i = 0; i < 100; ++i) {
        auto it = moveOnlyValues.find(i + 1);
        ASSERT_FALSE(it == moveOnlyValues.end());
    }

    for (size_t i = 0; i < 50; ++i)
        ASSERT_EQ(moveOnlyValues.take(i + 1).value(), i + 1);

    for (size_t i = 50; i < 100; ++i)
        ASSERT_TRUE(moveOnlyValues.remove(i + 1));

    ASSERT_TRUE(moveOnlyValues.isEmpty());
}

TEST(WTF_HashMap, MoveOnlyKeys)
{
    HashMap<MoveOnly, unsigned> moveOnlyKeys;

    for (size_t i = 0; i < 100; ++i) {
        MoveOnly moveOnly(i + 1);
        moveOnlyKeys.set(WTFMove(moveOnly), i + 1);
    }

    for (size_t i = 0; i < 100; ++i) {
        auto it = moveOnlyKeys.find(MoveOnly(i + 1));
        ASSERT_FALSE(it == moveOnlyKeys.end());
    }

    for (size_t i = 0; i < 100; ++i)
        ASSERT_FALSE(moveOnlyKeys.add(MoveOnly(i + 1), i + 1).isNewEntry);

    for (size_t i = 0; i < 100; ++i)
        ASSERT_TRUE(moveOnlyKeys.remove(MoveOnly(i + 1)));

    ASSERT_TRUE(moveOnlyKeys.isEmpty());
}

TEST(WTF_HashMap, InitializerList)
{
    HashMap<unsigned, std::string> map = {
        { 1, "one" },
        { 2, "two" },
        { 3, "three" },
        { 4, "four" },
    };

    EXPECT_EQ(4u, map.size());

    EXPECT_EQ("one", map.get(1));
    EXPECT_EQ("two", map.get(2));
    EXPECT_EQ("three", map.get(3));
    EXPECT_EQ("four", map.get(4));
    EXPECT_EQ(std::string(), map.get(5));
}

TEST(WTF_HashMap, EfficientGetter)
{
    HashMap<unsigned, CopyMoveCounter> map;
    map.set(1, CopyMoveCounter());

    {
        CopyMoveCounter::TestingScope scope;
        map.get(1);
        EXPECT_EQ(0U, CopyMoveCounter::constructionCount);
        EXPECT_EQ(1U, CopyMoveCounter::copyCount);
        EXPECT_EQ(0U, CopyMoveCounter::moveCount);
    }

    {
        CopyMoveCounter::TestingScope scope;
        map.get(2);
        EXPECT_EQ(1U, CopyMoveCounter::constructionCount);
        EXPECT_EQ(0U, CopyMoveCounter::copyCount);
        EXPECT_EQ(1U, CopyMoveCounter::moveCount);
    }
}

TEST(WTF_HashMap, UniquePtrKey)
{
    ConstructorDestructorCounter::TestingScope scope;

    HashMap<std::unique_ptr<ConstructorDestructorCounter>, int> map;

    auto uniquePtr = std::make_unique<ConstructorDestructorCounter>();
    map.add(WTFMove(uniquePtr), 2);

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(0u, ConstructorDestructorCounter::destructionCount);

    map.clear();
    
    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(1u, ConstructorDestructorCounter::destructionCount);
}

TEST(WTF_HashMap, UniquePtrKey_CustomDeleter)
{
    ConstructorDestructorCounter::TestingScope constructorDestructorCounterScope;
    DeleterCounter<ConstructorDestructorCounter>::TestingScope deleterCounterScope;

    HashMap<std::unique_ptr<ConstructorDestructorCounter, DeleterCounter<ConstructorDestructorCounter>>, int> map;

    std::unique_ptr<ConstructorDestructorCounter, DeleterCounter<ConstructorDestructorCounter>> uniquePtr(new ConstructorDestructorCounter(), DeleterCounter<ConstructorDestructorCounter>());
    map.add(WTFMove(uniquePtr), 2);

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(0u, ConstructorDestructorCounter::destructionCount);

    EXPECT_EQ(0u, DeleterCounter<ConstructorDestructorCounter>::deleterCount());

    map.clear();
    
    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(1u, ConstructorDestructorCounter::destructionCount);

    EXPECT_EQ(1u, DeleterCounter<ConstructorDestructorCounter>::deleterCount());
}

TEST(WTF_HashMap, UniquePtrKey_FindUsingRawPointer)
{
    HashMap<std::unique_ptr<int>, int> map;

    auto uniquePtr = std::make_unique<int>(5);
    int* ptr = uniquePtr.get();
    map.add(WTFMove(uniquePtr), 2);

    auto it = map.find(ptr);
    ASSERT_TRUE(it != map.end());
    EXPECT_EQ(ptr, it->key.get());
    EXPECT_EQ(2, it->value);
}

TEST(WTF_HashMap, UniquePtrKey_ContainsUsingRawPointer)
{
    HashMap<std::unique_ptr<int>, int> map;

    auto uniquePtr = std::make_unique<int>(5);
    int* ptr = uniquePtr.get();
    map.add(WTFMove(uniquePtr), 2);

    EXPECT_EQ(true, map.contains(ptr));
}

TEST(WTF_HashMap, UniquePtrKey_GetUsingRawPointer)
{
    HashMap<std::unique_ptr<int>, int> map;

    auto uniquePtr = std::make_unique<int>(5);
    int* ptr = uniquePtr.get();
    map.add(WTFMove(uniquePtr), 2);

    int value = map.get(ptr);
    EXPECT_EQ(2, value);
}

TEST(WTF_HashMap, UniquePtrKey_RemoveUsingRawPointer)
{
    ConstructorDestructorCounter::TestingScope scope;

    HashMap<std::unique_ptr<ConstructorDestructorCounter>, int> map;

    auto uniquePtr = std::make_unique<ConstructorDestructorCounter>();
    ConstructorDestructorCounter* ptr = uniquePtr.get();
    map.add(WTFMove(uniquePtr), 2);

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(0u, ConstructorDestructorCounter::destructionCount);

    bool result = map.remove(ptr);
    EXPECT_EQ(true, result);

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(1u, ConstructorDestructorCounter::destructionCount);
}

TEST(WTF_HashMap, UniquePtrKey_TakeUsingRawPointer)
{
    ConstructorDestructorCounter::TestingScope scope;

    HashMap<std::unique_ptr<ConstructorDestructorCounter>, int> map;

    auto uniquePtr = std::make_unique<ConstructorDestructorCounter>();
    ConstructorDestructorCounter* ptr = uniquePtr.get();
    map.add(WTFMove(uniquePtr), 2);

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(0u, ConstructorDestructorCounter::destructionCount);

    int result = map.take(ptr);
    EXPECT_EQ(2, result);

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(1u, ConstructorDestructorCounter::destructionCount);
}

TEST(WTF_HashMap, RefPtrKey_Add)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");
    RefPtr<RefLogger> ptr(&a);
    map.add(ptr, 0);

    ASSERT_STREQ("ref(a) ref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_AddUsingRelease)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");
    RefPtr<RefLogger> ptr(&a);
    map.add(WTFMove(ptr), 0);

    EXPECT_STREQ("ref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_AddUsingMove)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");
    RefPtr<RefLogger> ptr(&a);
    map.add(WTFMove(ptr), 0);

    EXPECT_STREQ("ref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_AddUsingRaw)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");
    RefPtr<RefLogger> ptr(&a);
    map.add(ptr.get(), 0);

    EXPECT_STREQ("ref(a) ref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_AddKeyAlreadyPresent)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");

    {
        RefPtr<RefLogger> ptr(&a);
        map.add(ptr, 0);
    }

    EXPECT_STREQ("ref(a) ref(a) deref(a) ", takeLogStr().c_str());

    {
        RefPtr<RefLogger> ptr2(&a);
        auto addResult = map.add(ptr2, 0);
        EXPECT_FALSE(addResult.isNewEntry);
    }

    EXPECT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_AddUsingReleaseKeyAlreadyPresent)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");

    {
        RefPtr<RefLogger> ptr(&a);
        map.add(ptr, 0);
    }

    EXPECT_STREQ("ref(a) ref(a) deref(a) ", takeLogStr().c_str());

    {
        RefPtr<RefLogger> ptr2(&a);
        auto addResult = map.add(WTFMove(ptr2), 0);
        EXPECT_FALSE(addResult.isNewEntry);
    }

    EXPECT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_AddUsingMoveKeyAlreadyPresent)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");

    {
        RefPtr<RefLogger> ptr(&a);
        map.add(ptr, 0);
    }

    EXPECT_STREQ("ref(a) ref(a) deref(a) ", takeLogStr().c_str());

    {
        RefPtr<RefLogger> ptr2(&a);
        auto addResult = map.add(WTFMove(ptr2), 0);
        EXPECT_FALSE(addResult.isNewEntry);
    }

    EXPECT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_Set)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");
    RefPtr<RefLogger> ptr(&a);
    map.set(ptr, 0);

    ASSERT_STREQ("ref(a) ref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_SetUsingRelease)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");
    RefPtr<RefLogger> ptr(&a);
    map.set(WTFMove(ptr), 0);

    EXPECT_STREQ("ref(a) ", takeLogStr().c_str());
}


TEST(WTF_HashMap, RefPtrKey_SetUsingMove)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");
    RefPtr<RefLogger> ptr(&a);
    map.set(WTFMove(ptr), 0);

    EXPECT_STREQ("ref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_SetUsingRaw)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");
    RefPtr<RefLogger> ptr(&a);
    map.set(ptr.get(), 0);

    EXPECT_STREQ("ref(a) ref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_SetKeyAlreadyPresent)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");

    RefPtr<RefLogger> ptr(&a);
    map.set(ptr, 0);

    EXPECT_STREQ("ref(a) ref(a) ", takeLogStr().c_str());

    {
        RefPtr<RefLogger> ptr2(&a);
        auto addResult = map.set(ptr2, 1);
        EXPECT_FALSE(addResult.isNewEntry);
        EXPECT_EQ(1, map.get(ptr.get()));
    }

    EXPECT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_SetUsingReleaseKeyAlreadyPresent)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");

    RefPtr<RefLogger> ptr(&a);
    map.set(ptr, 0);

    EXPECT_STREQ("ref(a) ref(a) ", takeLogStr().c_str());

    {
        RefPtr<RefLogger> ptr2(&a);
        auto addResult = map.set(WTFMove(ptr2), 1);
        EXPECT_FALSE(addResult.isNewEntry);
        EXPECT_EQ(1, map.get(ptr.get()));
    }

    EXPECT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, RefPtrKey_SetUsingMoveKeyAlreadyPresent)
{
    HashMap<RefPtr<RefLogger>, int> map;

    DerivedRefLogger a("a");

    RefPtr<RefLogger> ptr(&a);
    map.set(ptr, 0);

    EXPECT_STREQ("ref(a) ref(a) ", takeLogStr().c_str());

    {
        RefPtr<RefLogger> ptr2(&a);
        auto addResult = map.set(WTFMove(ptr2), 1);
        EXPECT_FALSE(addResult.isNewEntry);
        EXPECT_EQ(1, map.get(ptr.get()));
    }

    EXPECT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, Ensure)
{
    HashMap<unsigned, unsigned> map;
    {
        auto addResult = map.ensure(1, [] { return 1; });
        EXPECT_EQ(1u, addResult.iterator->value);
        EXPECT_EQ(1u, addResult.iterator->key);
        EXPECT_TRUE(addResult.isNewEntry);
        auto addResult2 = map.ensure(1, [] { return 2; });
        EXPECT_EQ(1u, addResult2.iterator->value);
        EXPECT_EQ(1u, addResult2.iterator->key);
        EXPECT_FALSE(addResult2.isNewEntry);
    }
}

TEST(WTF_HashMap, Ensure_MoveOnlyValues)
{
    HashMap<unsigned, MoveOnly> moveOnlyValues;
    {
        auto addResult = moveOnlyValues.ensure(1, [] { return MoveOnly(1); });
        EXPECT_EQ(1u, addResult.iterator->value.value());
        EXPECT_EQ(1u, addResult.iterator->key);
        EXPECT_TRUE(addResult.isNewEntry);
        auto addResult2 = moveOnlyValues.ensure(1, [] { return MoveOnly(2); });
        EXPECT_EQ(1u, addResult2.iterator->value.value());
        EXPECT_EQ(1u, addResult2.iterator->key);
        EXPECT_FALSE(addResult2.isNewEntry);
    }
}

TEST(WTF_HashMap, Ensure_UniquePointer)
{
    HashMap<unsigned, std::unique_ptr<unsigned>> map;
    {
        auto addResult = map.ensure(1, [] { return std::make_unique<unsigned>(1); });
        EXPECT_EQ(1u, *map.get(1));
        EXPECT_EQ(1u, *addResult.iterator->value.get());
        EXPECT_EQ(1u, addResult.iterator->key);
        EXPECT_TRUE(addResult.isNewEntry);
        auto addResult2 = map.ensure(1, [] { return std::make_unique<unsigned>(2); });
        EXPECT_EQ(1u, *map.get(1));
        EXPECT_EQ(1u, *addResult2.iterator->value.get());
        EXPECT_EQ(1u, addResult2.iterator->key);
        EXPECT_FALSE(addResult2.isNewEntry);
    }
}

TEST(WTF_HashMap, Ensure_RefPtr)
{
    HashMap<unsigned, RefPtr<RefLogger>> map;

    {
        DerivedRefLogger a("a");

        map.ensure(1, [&] { return RefPtr<RefLogger>(&a); });
        EXPECT_STREQ("ref(a) ", takeLogStr().c_str());

        map.ensure(1, [&] { return RefPtr<RefLogger>(&a); });
        EXPECT_STREQ("", takeLogStr().c_str());
    }
}

class ObjectWithRefLogger {
public:
    ObjectWithRefLogger(Ref<RefLogger>&& logger)
        : m_logger(WTFMove(logger))
    {
    }

    Ref<RefLogger> m_logger;
};


void testMovingUsingEnsure(Ref<RefLogger>&& logger)
{
    HashMap<unsigned, std::unique_ptr<ObjectWithRefLogger>> map;
    
    map.ensure(1, [&] { return std::make_unique<ObjectWithRefLogger>(WTFMove(logger)); });
}

void testMovingUsingAdd(Ref<RefLogger>&& logger)
{
    HashMap<unsigned, std::unique_ptr<ObjectWithRefLogger>> map;

    auto& slot = map.add(1, nullptr).iterator->value;
    slot = std::make_unique<ObjectWithRefLogger>(WTFMove(logger));
}

TEST(WTF_HashMap, Ensure_LambdasCapturingByReference)
{
    {
        DerivedRefLogger a("a");
        Ref<RefLogger> ref(a);
        testMovingUsingEnsure(WTFMove(ref));

        EXPECT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());
    }

    {
        DerivedRefLogger a("a");
        Ref<RefLogger> ref(a);
        testMovingUsingAdd(WTFMove(ref));

        EXPECT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());
    }
}


TEST(WTF_HashMap, ValueIsDestructedOnRemove)
{
    struct DestructorObserver {
        DestructorObserver() = default;

        DestructorObserver(bool* destructed)
            : destructed(destructed)
        {
        }

        ~DestructorObserver()
        {
            if (destructed)
                *destructed = true;
        }

        DestructorObserver(DestructorObserver&& other)
            : destructed(other.destructed)
        {
            other.destructed = nullptr;
        }

        DestructorObserver& operator=(DestructorObserver&& other)
        {
            destructed = other.destructed;
            other.destructed = nullptr;
            return *this;
        }

        bool* destructed { nullptr };
    };

    HashMap<int, DestructorObserver> map;

    bool destructed = false;
    map.add(5, DestructorObserver { &destructed });

    EXPECT_FALSE(destructed);

    bool removeResult = map.remove(5);

    EXPECT_TRUE(removeResult);
    EXPECT_TRUE(destructed);
}

TEST(WTF_HashMap, RefPtrNotZeroedBeforeDeref)
{
    struct DerefObserver {
        NEVER_INLINE void ref()
        {
            ++count;
        }
        NEVER_INLINE void deref()
        {
            --count;
            observedBucket = bucketAddress->get();
        }
        unsigned count { 1 };
        const RefPtr<DerefObserver>* bucketAddress { nullptr };
        const DerefObserver* observedBucket { nullptr };
    };

    auto observer = std::make_unique<DerefObserver>();

    HashMap<RefPtr<DerefObserver>, int> map;
    map.add(adoptRef(observer.get()), 5);

    auto iterator = map.find(observer.get());
    EXPECT_TRUE(iterator != map.end());

    observer->bucketAddress = &iterator->key;

    EXPECT_TRUE(observer->observedBucket == nullptr);
    EXPECT_TRUE(map.remove(observer.get()));

    // It if fine to either leave the old value intact at deletion or already set it to the deleted
    // value.
    // A zero would be a incorrect outcome as it would mean we nulled the bucket before an opaque
    // call.
    EXPECT_TRUE(observer->observedBucket == observer.get() || observer->observedBucket == RefPtr<DerefObserver>::hashTableDeletedValue());
    EXPECT_EQ(observer->count, 0u);
}

TEST(WTF_HashMap, Ref_Key)
{
    {
        HashMap<Ref<RefLogger>, int> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.add(WTFMove(ref), 1);
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<Ref<RefLogger>, int> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.set(WTFMove(ref), 1);
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<Ref<RefLogger>, int> map;

        RefLogger a("a");
        Ref<RefLogger> refA(a);
        map.add(WTFMove(refA), 1);

        Ref<RefLogger> refA2(a);
        map.set(WTFMove(refA2), 1);
    }

    ASSERT_STREQ("ref(a) ref(a) deref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<Ref<RefLogger>, int> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.ensure(WTFMove(ref), []() { 
            return 1; 
        });
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<Ref<RefLogger>, int> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.add(WTFMove(ref), 1);
        
        auto it = map.find(&a);
        ASSERT_TRUE(it != map.end());
        
        ASSERT_EQ(it->key.ptr(), &a);
        ASSERT_EQ(it->value, 1);
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<Ref<RefLogger>, int> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.add(WTFMove(ref), 1);

        map.remove(&a);
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<Ref<RefLogger>, int> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.add(WTFMove(ref), 1);

        int i = map.take(&a);
        ASSERT_EQ(i, 1);
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<Ref<RefLogger>, int> map;
        for (int i = 0; i < 64; ++i) {
            Ref<RefLogger> ref = adoptRef(*new RefLogger("a"));
            auto* pointer = ref.ptr();
            map.add(WTFMove(ref), i + 1);
            ASSERT_TRUE(map.contains(pointer));
        }
    }

    ASSERT_STREQ("deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, Ref_Value)
{
    {
        HashMap<int, Ref<RefLogger>> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.add(1, WTFMove(ref));
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<int, Ref<RefLogger>> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.set(1, WTFMove(ref));
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<int, Ref<RefLogger>> map;

        RefLogger a("a");
        Ref<RefLogger> refA(a);
        map.add(1, WTFMove(refA));

        RefLogger b("b");
        Ref<RefLogger> refB(b);
        map.set(1, WTFMove(refB));
    }

    ASSERT_STREQ("ref(a) ref(b) deref(a) deref(b) ", takeLogStr().c_str());

    {
        HashMap<int, Ref<RefLogger>> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.add(1, WTFMove(ref));
        
        auto aGet = map.get(1);
        ASSERT_EQ(aGet, &a);
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<int, Ref<RefLogger>> map;
        
        auto emptyGet = map.get(1);
        ASSERT_TRUE(emptyGet == nullptr);
    }

    {
        HashMap<int, Ref<RefLogger>> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.add(1, WTFMove(ref));
        
        auto aOut = map.take(1);
        ASSERT_TRUE(static_cast<bool>(aOut));
        ASSERT_EQ(&a, aOut.value().ptr());
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<int, Ref<RefLogger>> map;
        
        auto emptyTake = map.take(1);
        ASSERT_FALSE(static_cast<bool>(emptyTake));
    }

    {
        HashMap<int, Ref<RefLogger>> map;

        RefLogger a("a");
        Ref<RefLogger> ref(a);
        map.add(1, WTFMove(ref));
        map.remove(1);
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<int, Ref<RefLogger>> map;

        RefLogger a("a");
        map.ensure(1, [&]() mutable {
            Ref<RefLogger> ref(a);
            return ref; 
        });
    }

    ASSERT_STREQ("ref(a) deref(a) ", takeLogStr().c_str());

    {
        HashMap<int, Ref<RefLogger>> map;
        for (int i = 0; i < 64; ++i) {
            Ref<RefLogger> ref = adoptRef(*new RefLogger("a"));
            map.add(i + 1, WTFMove(ref));
            ASSERT_TRUE(map.contains(i + 1));
        }
    }

    ASSERT_STREQ("deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) deref(a) ", takeLogStr().c_str());
}

TEST(WTF_HashMap, DeletedAddressOfOperator)
{
    HashMap<int, DeletedAddressOfOperator> map1;
    for (auto& value : map1.values())
        (void)value;
}

} // namespace TestWebKitAPI
