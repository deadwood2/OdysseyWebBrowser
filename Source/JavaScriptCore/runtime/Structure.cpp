/*
 * Copyright (C) 2008-2019 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Structure.h"

#include "BuiltinNames.h"
#include "CodeBlock.h"
#include "DumpContext.h"
#include "JSCInlines.h"
#include "JSObject.h"
#include "JSPropertyNameEnumerator.h"
#include "Lookup.h"
#include "PropertyMapHashTable.h"
#include "PropertyNameArray.h"
#include "StructureChain.h"
#include "StructureRareDataInlines.h"
#include "WeakGCMapInlines.h"
#include <wtf/CommaPrinter.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/ProcessID.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

#define DUMP_STRUCTURE_ID_STATISTICS 0

namespace JSC {

#if DUMP_STRUCTURE_ID_STATISTICS
static HashSet<Structure*>& liveStructureSet = *(new HashSet<Structure*>);
#endif

class SingleSlotTransitionWeakOwner final : public WeakHandleOwner {
    void finalize(Handle<Unknown>, void* context) override
    {
        StructureTransitionTable* table = reinterpret_cast<StructureTransitionTable*>(context);
        ASSERT(table->isUsingSingleSlot());
        WeakSet::deallocate(table->weakImpl());
        table->m_data = StructureTransitionTable::UsingSingleSlotFlag;
    }
};

static SingleSlotTransitionWeakOwner& singleSlotTransitionWeakOwner()
{
    static NeverDestroyed<SingleSlotTransitionWeakOwner> owner;
    return owner;
}

inline Structure* StructureTransitionTable::singleTransition() const
{
    ASSERT(isUsingSingleSlot());
    if (WeakImpl* impl = this->weakImpl()) {
        if (impl->state() == WeakImpl::Live)
            return jsCast<Structure*>(impl->jsValue().asCell());
    }
    return nullptr;
}

inline void StructureTransitionTable::setSingleTransition(Structure* structure)
{
    ASSERT(isUsingSingleSlot());
    if (WeakImpl* impl = this->weakImpl())
        WeakSet::deallocate(impl);
    WeakImpl* impl = WeakSet::allocate(structure, &singleSlotTransitionWeakOwner(), this);
    m_data = bitwise_cast<intptr_t>(impl) | UsingSingleSlotFlag;
}

bool StructureTransitionTable::contains(UniquedStringImpl* rep, unsigned attributes) const
{
    if (isUsingSingleSlot()) {
        Structure* transition = singleTransition();
        return transition && transition->m_nameInPrevious == rep && transition->attributesInPrevious() == attributes;
    }
    return map()->get(std::make_pair(rep, attributes));
}

inline Structure* StructureTransitionTable::get(UniquedStringImpl* rep, unsigned attributes) const
{
    if (isUsingSingleSlot()) {
        Structure* transition = singleTransition();
        return (transition && transition->m_nameInPrevious == rep && transition->attributesInPrevious() == attributes) ? transition : 0;
    }
    return map()->get(std::make_pair(rep, attributes));
}

void StructureTransitionTable::add(VM& vm, Structure* structure)
{
    if (isUsingSingleSlot()) {
        Structure* existingTransition = singleTransition();

        // This handles the first transition being added.
        if (!existingTransition) {
            setSingleTransition(structure);
            return;
        }

        // This handles the second transition being added
        // (or the first transition being despecified!)
        setMap(new TransitionMap(vm));
        add(vm, existingTransition);
    }

    // Add the structure to the map.

    // Newer versions of the STL have an std::make_pair function that takes rvalue references.
    // When either of the parameters are bitfields, the C++ compiler will try to bind them as lvalues, which is invalid. To work around this, use unary "+" to make the parameter an rvalue.
    // See https://bugs.webkit.org/show_bug.cgi?id=59261 for more details
    map()->set(std::make_pair(structure->m_nameInPrevious.get(), +structure->attributesInPrevious()), structure);
}

void Structure::dumpStatistics()
{
#if DUMP_STRUCTURE_ID_STATISTICS
    unsigned numberLeaf = 0;
    unsigned numberUsingSingleSlot = 0;
    unsigned numberSingletons = 0;
    unsigned numberWithPropertyMaps = 0;
    unsigned totalPropertyMapsSize = 0;

    HashSet<Structure*>::const_iterator end = liveStructureSet.end();
    for (HashSet<Structure*>::const_iterator it = liveStructureSet.begin(); it != end; ++it) {
        Structure* structure = *it;

        switch (structure->m_transitionTable.size()) {
            case 0:
                ++numberLeaf;
                if (!structure->previousID())
                    ++numberSingletons;
                break;

            case 1:
                ++numberUsingSingleSlot;
                break;
        }

        if (PropertyTable* table = structure->propertyTableOrNull()) {
            ++numberWithPropertyMaps;
            totalPropertyMapsSize += table->sizeInMemory();
        }
    }

    dataLogF("Number of live Structures: %d\n", liveStructureSet.size());
    dataLogF("Number of Structures using the single item optimization for transition map: %d\n", numberUsingSingleSlot);
    dataLogF("Number of Structures that are leaf nodes: %d\n", numberLeaf);
    dataLogF("Number of Structures that singletons: %d\n", numberSingletons);
    dataLogF("Number of Structures with PropertyMaps: %d\n", numberWithPropertyMaps);

    dataLogF("Size of a single Structures: %d\n", static_cast<unsigned>(sizeof(Structure)));
    dataLogF("Size of sum of all property maps: %d\n", totalPropertyMapsSize);
    dataLogF("Size of average of all property maps: %f\n", static_cast<double>(totalPropertyMapsSize) / static_cast<double>(liveStructureSet.size()));
#else
    dataLogF("Dumping Structure statistics is not enabled.\n");
#endif
}

Structure::Structure(VM& vm, JSGlobalObject* globalObject, JSValue prototype, const TypeInfo& typeInfo, const ClassInfo* classInfo, IndexingType indexingType, unsigned inlineCapacity)
    : JSCell(vm, vm.structureStructure.get())
    , m_blob(vm.heap.structureIDTable().allocateID(this), indexingType, typeInfo)
    , m_outOfLineTypeFlags(typeInfo.outOfLineTypeFlags())
    , m_inlineCapacity(inlineCapacity)
    , m_bitField(0)
    , m_globalObject(vm, this, globalObject, WriteBarrier<JSGlobalObject>::MayBeNull)
    , m_prototype(vm, this, prototype)
    , m_classInfo(classInfo)
    , m_transitionWatchpointSet(IsWatched)
    , m_offset(invalidOffset)
    , m_propertyHash(0)
{
    setDictionaryKind(NoneDictionaryKind);
    setIsPinnedPropertyTable(false);
    setHasGetterSetterProperties(classInfo->hasStaticSetterOrReadonlyProperties());
    setHasCustomGetterSetterProperties(false);
    setHasReadOnlyOrGetterSetterPropertiesExcludingProto(classInfo->hasStaticSetterOrReadonlyProperties());
    setHasUnderscoreProtoPropertyExcludingOriginalProto(false);
    setIsQuickPropertyAccessAllowedForEnumeration(true);
    setAttributesInPrevious(0);
    setDidPreventExtensions(false);
    setDidTransition(false);
    setStaticPropertiesReified(false);
    setTransitionWatchpointIsLikelyToBeFired(false);
    setHasBeenDictionary(false);
    setIsAddingPropertyForTransition(false);
 
    ASSERT(inlineCapacity <= JSFinalObject::maxInlineCapacity());
    ASSERT(static_cast<PropertyOffset>(inlineCapacity) < firstOutOfLineOffset);
    ASSERT(!hasRareData());
    ASSERT(hasReadOnlyOrGetterSetterPropertiesExcludingProto() || !m_classInfo->hasStaticSetterOrReadonlyProperties());
    ASSERT(hasGetterSetterProperties() || !m_classInfo->hasStaticSetterOrReadonlyProperties());
    ASSERT(!this->typeInfo().overridesGetCallData() || m_classInfo->methodTable.getCallData != &JSCell::getCallData);
}

const ClassInfo Structure::s_info = { "Structure", nullptr, nullptr, nullptr, CREATE_METHOD_TABLE(Structure) };

Structure::Structure(VM& vm)
    : JSCell(CreatingEarlyCell)
    , m_inlineCapacity(0)
    , m_bitField(0)
    , m_prototype(vm, this, jsNull())
    , m_classInfo(info())
    , m_transitionWatchpointSet(IsWatched)
    , m_offset(invalidOffset)
    , m_propertyHash(0)
{
    setDictionaryKind(NoneDictionaryKind);
    setIsPinnedPropertyTable(false);
    setHasGetterSetterProperties(m_classInfo->hasStaticSetterOrReadonlyProperties());
    setHasCustomGetterSetterProperties(false);
    setHasReadOnlyOrGetterSetterPropertiesExcludingProto(m_classInfo->hasStaticSetterOrReadonlyProperties());
    setHasUnderscoreProtoPropertyExcludingOriginalProto(false);
    setIsQuickPropertyAccessAllowedForEnumeration(true);
    setAttributesInPrevious(0);
    setDidPreventExtensions(false);
    setDidTransition(false);
    setStaticPropertiesReified(false);
    setTransitionWatchpointIsLikelyToBeFired(false);
    setHasBeenDictionary(false);
    setIsAddingPropertyForTransition(false);
 
    TypeInfo typeInfo = TypeInfo(CellType, StructureFlags);
    m_blob = StructureIDBlob(vm.heap.structureIDTable().allocateID(this), 0, typeInfo);
    m_outOfLineTypeFlags = typeInfo.outOfLineTypeFlags();

    ASSERT(hasReadOnlyOrGetterSetterPropertiesExcludingProto() || !m_classInfo->hasStaticSetterOrReadonlyProperties());
    ASSERT(hasGetterSetterProperties() || !m_classInfo->hasStaticSetterOrReadonlyProperties());
    ASSERT(!this->typeInfo().overridesGetCallData() || m_classInfo->methodTable.getCallData != &JSCell::getCallData);
}

Structure::Structure(VM& vm, Structure* previous, DeferredStructureTransitionWatchpointFire* deferred)
    : JSCell(vm, vm.structureStructure.get())
    , m_inlineCapacity(previous->m_inlineCapacity)
    , m_bitField(0)
    , m_prototype(vm, this, previous->m_prototype.get())
    , m_classInfo(previous->m_classInfo)
    , m_transitionWatchpointSet(IsWatched)
    , m_offset(invalidOffset)
    , m_propertyHash(previous->m_propertyHash)
{
    setDictionaryKind(previous->dictionaryKind());
    setIsPinnedPropertyTable(false);
    setHasBeenFlattenedBefore(previous->hasBeenFlattenedBefore());
    setHasGetterSetterProperties(previous->hasGetterSetterProperties());
    setHasCustomGetterSetterProperties(previous->hasCustomGetterSetterProperties());
    setHasReadOnlyOrGetterSetterPropertiesExcludingProto(previous->hasReadOnlyOrGetterSetterPropertiesExcludingProto());
    setHasUnderscoreProtoPropertyExcludingOriginalProto(previous->hasUnderscoreProtoPropertyExcludingOriginalProto());
    setIsQuickPropertyAccessAllowedForEnumeration(previous->isQuickPropertyAccessAllowedForEnumeration());
    setAttributesInPrevious(0);
    setDidPreventExtensions(previous->didPreventExtensions());
    setDidTransition(true);
    setStaticPropertiesReified(previous->staticPropertiesReified());
    setHasBeenDictionary(previous->hasBeenDictionary());
    setIsAddingPropertyForTransition(false);
 
    TypeInfo typeInfo = previous->typeInfo();
    m_blob = StructureIDBlob(vm.heap.structureIDTable().allocateID(this), previous->indexingModeIncludingHistory(), typeInfo);
    m_outOfLineTypeFlags = typeInfo.outOfLineTypeFlags();

    ASSERT(!previous->typeInfo().structureIsImmortal());
    setPreviousID(vm, previous);

    previous->didTransitionFromThisStructure(deferred);
    
    // Copy this bit now, in case previous was being watched.
    setTransitionWatchpointIsLikelyToBeFired(previous->transitionWatchpointIsLikelyToBeFired());

    if (previous->m_globalObject)
        m_globalObject.set(vm, this, previous->m_globalObject.get());
    ASSERT(hasReadOnlyOrGetterSetterPropertiesExcludingProto() || !m_classInfo->hasStaticSetterOrReadonlyProperties());
    ASSERT(hasGetterSetterProperties() || !m_classInfo->hasStaticSetterOrReadonlyProperties());
    ASSERT(!this->typeInfo().overridesGetCallData() || m_classInfo->methodTable.getCallData != &JSCell::getCallData);
}

Structure::~Structure()
{
    if (typeInfo().structureIsImmortal())
        return;
    Heap::heap(this)->structureIDTable().deallocateID(this, m_blob.structureID());
}

void Structure::destroy(JSCell* cell)
{
    static_cast<Structure*>(cell)->Structure::~Structure();
}

Structure* Structure::create(PolyProtoTag, VM& vm, JSGlobalObject* globalObject, JSObject* prototype, const TypeInfo& typeInfo, const ClassInfo* classInfo, IndexingType indexingType, unsigned inlineCapacity)
{
    Structure* result = create(vm, globalObject, prototype, typeInfo, classInfo, indexingType, inlineCapacity);

    unsigned oldOutOfLineCapacity = result->outOfLineCapacity();
    result->addPropertyWithoutTransition(
        vm, vm.propertyNames->builtinNames().polyProtoName(), static_cast<unsigned>(PropertyAttribute::DontEnum),
        [&] (const GCSafeConcurrentJSLocker&, PropertyOffset offset, PropertyOffset newLastOffset) {
            RELEASE_ASSERT(Structure::outOfLineCapacity(newLastOffset) == oldOutOfLineCapacity);
            RELEASE_ASSERT(offset == knownPolyProtoOffset);
            RELEASE_ASSERT(isInlineOffset(knownPolyProtoOffset));
            result->m_prototype.setWithoutWriteBarrier(JSValue());
            result->setLastOffset(newLastOffset);
        });

    return result;
}

bool Structure::isValidPrototype(JSValue prototype)
{
    return prototype.isNull() || (prototype.isObject() && prototype.getObject()->mayBePrototype());
}

void Structure::findStructuresAndMapForMaterialization(Vector<Structure*, 8>& structures, Structure*& structure, PropertyTable*& table)
{
    ASSERT(structures.isEmpty());
    table = 0;

    for (structure = this; structure; structure = structure->previousID()) {
        structure->m_lock.lock();
        
        table = structure->propertyTableOrNull();
        if (table) {
            // Leave the structure locked, so that the caller can do things to it atomically
            // before it loses its property table.
            return;
        }
        
        structures.append(structure);
        structure->m_lock.unlock();
    }
    
    ASSERT(!structure);
    ASSERT(!table);
}

PropertyTable* Structure::materializePropertyTable(VM& vm, bool setPropertyTable)
{
    ASSERT(structure(vm)->classInfo() == info());
    ASSERT(!isAddingPropertyForTransition());
    
    DeferGC deferGC(vm.heap);
    
    Vector<Structure*, 8> structures;
    Structure* structure;
    PropertyTable* table;
    
    findStructuresAndMapForMaterialization(structures, structure, table);
    
    unsigned capacity = numberOfSlotsForLastOffset(m_offset, m_inlineCapacity);
    if (table) {
        table = table->copy(vm, capacity);
        structure->m_lock.unlock();
    } else
        table = PropertyTable::create(vm, capacity);
    
    // Must hold the lock on this structure, since we will be modifying this structure's
    // property map. We don't want getConcurrently() to see the property map in a half-baked
    // state.
    GCSafeConcurrentJSLocker locker(m_lock, vm.heap);
    if (setPropertyTable)
        this->setPropertyTable(vm, table);

    for (size_t i = structures.size(); i--;) {
        structure = structures[i];
        if (!structure->m_nameInPrevious)
            continue;
        PropertyMapEntry entry(structure->m_nameInPrevious.get(), structure->m_offset, structure->attributesInPrevious());
        table->add(entry, m_offset, PropertyTable::PropertyOffsetMustNotChange);
    }
    
    checkOffsetConsistency(
        table,
        [&] () {
            dataLog("Detected in materializePropertyTable.\n");
            dataLog("Found structure = ", RawPointer(structure), "\n");
            dataLog("structures = ");
            CommaPrinter comma;
            for (Structure* structure : structures)
                dataLog(comma, RawPointer(structure));
            dataLog("\n");
        });
    
    return table;
}

Structure* Structure::addPropertyTransitionToExistingStructureImpl(Structure* structure, UniquedStringImpl* uid, unsigned attributes, PropertyOffset& offset)
{
    ASSERT(!structure->isDictionary());
    ASSERT(structure->isObject());

    if (Structure* existingTransition = structure->m_transitionTable.get(uid, attributes)) {
        validateOffset(existingTransition->m_offset, existingTransition->inlineCapacity());
        offset = existingTransition->m_offset;
        return existingTransition;
    }

    return 0;
}

Structure* Structure::addPropertyTransitionToExistingStructure(Structure* structure, PropertyName propertyName, unsigned attributes, PropertyOffset& offset)
{
    ASSERT(!isCompilationThread());
    return addPropertyTransitionToExistingStructureImpl(structure, propertyName.uid(), attributes, offset);
}

Structure* Structure::addPropertyTransitionToExistingStructureConcurrently(Structure* structure, UniquedStringImpl* uid, unsigned attributes, PropertyOffset& offset)
{
    ConcurrentJSLocker locker(structure->m_lock);
    return addPropertyTransitionToExistingStructureImpl(structure, uid, attributes, offset);
}

bool Structure::holesMustForwardToPrototype(VM& vm, JSObject* base) const
{
    ASSERT(base->structure(vm) == this);

    if (this->mayInterceptIndexedAccesses())
        return true;

    JSValue prototype = this->storedPrototype(base);
    if (!prototype.isObject())
        return false;
    JSObject* object = asObject(prototype);

    while (true) {
        Structure& structure = *object->structure(vm);
        if (hasIndexedProperties(object->indexingType()) || structure.mayInterceptIndexedAccesses())
            return true;
        prototype = structure.storedPrototype(object);
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }

    RELEASE_ASSERT_NOT_REACHED();
    return false;
}

Structure* Structure::addPropertyTransition(VM& vm, Structure* structure, PropertyName propertyName, unsigned attributes, PropertyOffset& offset)
{
    Structure* newStructure = addPropertyTransitionToExistingStructure(
        structure, propertyName, attributes, offset);
    if (newStructure)
        return newStructure;

    return addNewPropertyTransition(
        vm, structure, propertyName, attributes, offset, PutPropertySlot::UnknownContext);
}

Structure* Structure::addNewPropertyTransition(VM& vm, Structure* structure, PropertyName propertyName, unsigned attributes, PropertyOffset& offset, PutPropertySlot::Context context, DeferredStructureTransitionWatchpointFire* deferred)
{
    ASSERT(!structure->isDictionary());
    ASSERT(structure->isObject());
    ASSERT(!Structure::addPropertyTransitionToExistingStructure(structure, propertyName, attributes, offset));
    
    int maxTransitionLength;
    if (context == PutPropertySlot::PutById)
        maxTransitionLength = s_maxTransitionLengthForNonEvalPutById;
    else
        maxTransitionLength = s_maxTransitionLength;
    if (structure->transitionCount() > maxTransitionLength) {
        ASSERT(!isCopyOnWrite(structure->indexingMode()));
        Structure* transition = toCacheableDictionaryTransition(vm, structure, deferred);
        ASSERT(structure != transition);
        offset = transition->add(vm, propertyName, attributes);
        return transition;
    }
    
    Structure* transition = create(vm, structure, deferred);

    transition->m_cachedPrototypeChain.setMayBeNull(vm, transition, structure->m_cachedPrototypeChain.get());
    
    // While we are adding the property, rematerializing the property table is super weird: we already
    // have a m_nameInPrevious and attributesInPrevious but the m_offset is still wrong. If the
    // materialization algorithm runs, it'll build a property table that already has the property but
    // at a bogus offset. Rather than try to teach the materialization code how to create a table under
    // those conditions, we just tell the GC not to blow the table away during this period of time.
    // Holding the lock ensures that we either do this before the GC starts scanning the structure, in
    // which case the GC will not blow the table away, or we do it after the GC already ran in which
    // case all is well.  If it wasn't for the lock, the GC would have TOCTOU: if could read
    // isAddingPropertyForTransition before we set it to true, and then blow the table away after.
    {
        ConcurrentJSLocker locker(transition->m_lock);
        transition->setIsAddingPropertyForTransition(true);
    }

    transition->m_blob.setIndexingModeIncludingHistory(structure->indexingModeIncludingHistory() & ~CopyOnWrite);
    transition->m_nameInPrevious = propertyName.uid();
    transition->setAttributesInPrevious(attributes);
    transition->setPropertyTable(vm, structure->takePropertyTableOrCloneIfPinned(vm));
    transition->m_offset = structure->m_offset;

    offset = transition->add(vm, propertyName, attributes);

    // Now that everything is fine with the new structure's bookkeeping, the GC is free to blow the
    // table away if it wants. We can now rebuild it fine.
    WTF::storeStoreFence();
    transition->setIsAddingPropertyForTransition(false);

    checkOffset(transition->m_offset, transition->inlineCapacity());
    {
        ConcurrentJSLocker locker(structure->m_lock);
        DeferGC deferGC(vm.heap);
        structure->m_transitionTable.add(vm, transition);
    }
    transition->checkOffsetConsistency();
    structure->checkOffsetConsistency();
    return transition;
}

Structure* Structure::removePropertyTransition(VM& vm, Structure* structure, PropertyName propertyName, PropertyOffset& offset)
{
    // NOTE: There are some good reasons why this goes directly to uncacheable dictionary rather than
    // caching the removal. We can fix all of these things, but we must remember to do so, if we ever try
    // to optimize this case.
    //
    // - Cached transitions usually steal the property table, and assume that this is possible because they
    //   can just rebuild the table by looking at past transitions. That code assumes that the table only
    //   grew and never shrank. To support removals, we'd have to change the property table materialization
    //   code to handle deletions. Also, we have logic to get the list of properties on a structure that
    //   lacks a property table by just looking back through the set of transitions since the last
    //   structure that had a pinned table. That logic would also have to be changed to handle cached
    //   removals.
    //
    ASSERT(!structure->isUncacheableDictionary());

    Structure* transition = toUncacheableDictionaryTransition(vm, structure);

    offset = transition->remove(propertyName);

    transition->checkOffsetConsistency();
    return transition;
}

Structure* Structure::changePrototypeTransition(VM& vm, Structure* structure, JSValue prototype, DeferredStructureTransitionWatchpointFire& deferred)
{
    ASSERT(isValidPrototype(prototype));

    DeferGC deferGC(vm.heap);
    Structure* transition = create(vm, structure, &deferred);

    transition->m_prototype.set(vm, transition, prototype);

    PropertyTable* table = structure->copyPropertyTableForPinning(vm);
    transition->pin(holdLock(transition->m_lock), vm, table);
    transition->m_offset = structure->m_offset;
    
    transition->checkOffsetConsistency();
    return transition;
}

Structure* Structure::attributeChangeTransition(VM& vm, Structure* structure, PropertyName propertyName, unsigned attributes)
{
    if (!structure->isUncacheableDictionary()) {
        Structure* transition = create(vm, structure);

        PropertyTable* table = structure->copyPropertyTableForPinning(vm);
        transition->pin(holdLock(transition->m_lock), vm, table);
        transition->m_offset = structure->m_offset;
        
        structure = transition;
    }

    PropertyMapEntry* entry = structure->ensurePropertyTable(vm)->get(propertyName.uid());
    ASSERT(entry);
    entry->attributes = attributes;

    structure->checkOffsetConsistency();
    return structure;
}

Structure* Structure::toDictionaryTransition(VM& vm, Structure* structure, DictionaryKind kind, DeferredStructureTransitionWatchpointFire* deferred)
{
    ASSERT(!structure->isUncacheableDictionary());
    DeferGC deferGC(vm.heap);
    
    Structure* transition = create(vm, structure, deferred);

    PropertyTable* table = structure->copyPropertyTableForPinning(vm);
    transition->pin(holdLock(transition->m_lock), vm, table);
    transition->m_offset = structure->m_offset;
    transition->setDictionaryKind(kind);
    transition->setHasBeenDictionary(true);
    
    transition->checkOffsetConsistency();
    return transition;
}

Structure* Structure::toCacheableDictionaryTransition(VM& vm, Structure* structure, DeferredStructureTransitionWatchpointFire* deferred)
{
    return toDictionaryTransition(vm, structure, CachedDictionaryKind, deferred);
}

Structure* Structure::toUncacheableDictionaryTransition(VM& vm, Structure* structure)
{
    return toDictionaryTransition(vm, structure, UncachedDictionaryKind);
}

Structure* Structure::sealTransition(VM& vm, Structure* structure)
{
    return nonPropertyTransition(vm, structure, NonPropertyTransition::Seal);
}

Structure* Structure::freezeTransition(VM& vm, Structure* structure)
{
    return nonPropertyTransition(vm, structure, NonPropertyTransition::Freeze);
}

Structure* Structure::preventExtensionsTransition(VM& vm, Structure* structure)
{
    return nonPropertyTransition(vm, structure, NonPropertyTransition::PreventExtensions);
}

PropertyTable* Structure::takePropertyTableOrCloneIfPinned(VM& vm)
{
    // This must always return a property table. It can't return null.
    PropertyTable* result = propertyTableOrNull();
    if (result) {
        if (isPinnedPropertyTable())
            return result->copy(vm, result->size() + 1);
        ConcurrentJSLocker locker(m_lock);
        setPropertyTable(vm, nullptr);
        return result;
    }
    bool setPropertyTable = false;
    return materializePropertyTable(vm, setPropertyTable);
}

Structure* Structure::nonPropertyTransitionSlow(VM& vm, Structure* structure, NonPropertyTransition transitionKind)
{
    unsigned attributes = toAttributes(transitionKind);
    IndexingType indexingModeIncludingHistory = newIndexingType(structure->indexingModeIncludingHistory(), transitionKind);
    
    Structure* existingTransition;
    if (!structure->isDictionary() && (existingTransition = structure->m_transitionTable.get(0, attributes))) {
        ASSERT(existingTransition->attributesInPrevious() == attributes);
        ASSERT(existingTransition->indexingModeIncludingHistory() == indexingModeIncludingHistory);
        return existingTransition;
    }
    
    DeferGC deferGC(vm.heap);
    
    Structure* transition = create(vm, structure);
    transition->setAttributesInPrevious(attributes);
    transition->m_blob.setIndexingModeIncludingHistory(indexingModeIncludingHistory);
    
    if (preventsExtensions(transitionKind))
        transition->setDidPreventExtensions(true);
    
    if (setsDontDeleteOnAllProperties(transitionKind)
        || setsReadOnlyOnNonAccessorProperties(transitionKind)) {
        // We pin the property table on transitions that do wholesale editing of the property
        // table, since our logic for walking the property transition chain to rematerialize the
        // table doesn't know how to take into account such wholesale edits.

        PropertyTable* table = structure->copyPropertyTableForPinning(vm);
        transition->pinForCaching(holdLock(transition->m_lock), vm, table);
        transition->m_offset = structure->m_offset;
        
        table = transition->propertyTableOrNull();
        RELEASE_ASSERT(table);
        for (auto& entry : *table) {
            if (setsDontDeleteOnAllProperties(transitionKind))
                entry.attributes |= static_cast<unsigned>(PropertyAttribute::DontDelete);
            if (setsReadOnlyOnNonAccessorProperties(transitionKind) && !(entry.attributes & PropertyAttribute::Accessor))
                entry.attributes |= static_cast<unsigned>(PropertyAttribute::ReadOnly);
        }
    } else {
        transition->setPropertyTable(vm, structure->takePropertyTableOrCloneIfPinned(vm));
        transition->m_offset = structure->m_offset;
        checkOffset(transition->m_offset, transition->inlineCapacity());
    }
    
    if (setsReadOnlyOnNonAccessorProperties(transitionKind)
        && !transition->propertyTableOrNull()->isEmpty())
        transition->setHasReadOnlyOrGetterSetterPropertiesExcludingProto(true);
    
    if (structure->isDictionary()) {
        PropertyTable* table = transition->ensurePropertyTable(vm);
        transition->pin(holdLock(transition->m_lock), vm, table);
    } else {
        auto locker = holdLock(structure->m_lock);
        structure->m_transitionTable.add(vm, transition);
    }

    transition->checkOffsetConsistency();
    return transition;
}

// In future we may want to cache this property.
bool Structure::isSealed(VM& vm)
{
    if (isStructureExtensible())
        return false;

    PropertyTable* table = ensurePropertyTableIfNotEmpty(vm);
    if (!table)
        return true;
    
    PropertyTable::iterator end = table->end();
    for (PropertyTable::iterator iter = table->begin(); iter != end; ++iter) {
        if ((iter->attributes & PropertyAttribute::DontDelete) != static_cast<unsigned>(PropertyAttribute::DontDelete))
            return false;
    }
    return true;
}

// In future we may want to cache this property.
bool Structure::isFrozen(VM& vm)
{
    if (isStructureExtensible())
        return false;

    PropertyTable* table = ensurePropertyTableIfNotEmpty(vm);
    if (!table)
        return true;
    
    PropertyTable::iterator end = table->end();
    for (PropertyTable::iterator iter = table->begin(); iter != end; ++iter) {
        if (!(iter->attributes & PropertyAttribute::DontDelete))
            return false;
        if (!(iter->attributes & (PropertyAttribute::ReadOnly | PropertyAttribute::Accessor)))
            return false;
    }
    return true;
}

Structure* Structure::flattenDictionaryStructure(VM& vm, JSObject* object)
{
    checkOffsetConsistency();
    ASSERT(isDictionary());
    
    GCSafeConcurrentJSLocker locker(m_lock, vm.heap);
    
    object->setStructureIDDirectly(nuke(id()));
    WTF::storeStoreFence();

    size_t beforeOutOfLineCapacity = this->outOfLineCapacity();
    if (isUncacheableDictionary()) {
        PropertyTable* table = propertyTableOrNull();
        ASSERT(table);

        size_t propertyCount = table->size();

        // Holds our values compacted by insertion order.
        Vector<JSValue> values(propertyCount);

        // Copies out our values from their hashed locations, compacting property table offsets as we go.
        unsigned i = 0;
        PropertyTable::iterator end = table->end();
        m_offset = invalidOffset;
        for (PropertyTable::iterator iter = table->begin(); iter != end; ++iter, ++i) {
            values[i] = object->getDirect(iter->offset);
            m_offset = iter->offset = offsetForPropertyNumber(i, m_inlineCapacity);
        }
        
        // Copies in our values to their compacted locations.
        for (unsigned i = 0; i < propertyCount; i++)
            object->putDirect(vm, offsetForPropertyNumber(i, m_inlineCapacity), values[i]);

        table->clearDeletedOffsets();

        // We need to zero our unused property space; otherwise the GC might see a
        // stale pointer when we add properties in the future.
        memset(
            object->inlineStorageUnsafe() + inlineSize(),
            0,
            (inlineCapacity() - inlineSize()) * sizeof(EncodedJSValue));

        Butterfly* butterfly = object->butterfly();
        size_t preCapacity = butterfly->indexingHeader()->preCapacity(this);
        void* base = butterfly->base(preCapacity, beforeOutOfLineCapacity);
        void* startOfPropertyStorageSlots = reinterpret_cast<EncodedJSValue*>(base) + preCapacity;
        memset(startOfPropertyStorageSlots, 0, (beforeOutOfLineCapacity - outOfLineSize()) * sizeof(EncodedJSValue));
        checkOffsetConsistency();
    }

    setDictionaryKind(NoneDictionaryKind);
    setHasBeenFlattenedBefore(true);

    size_t afterOutOfLineCapacity = this->outOfLineCapacity();

    if (object->butterfly() && beforeOutOfLineCapacity != afterOutOfLineCapacity) {
        ASSERT(beforeOutOfLineCapacity > afterOutOfLineCapacity);
        // If the object had a Butterfly but after flattening/compacting we no longer have need of it,
        // we need to zero it out because the collector depends on the Structure to know the size for copying.
        if (!afterOutOfLineCapacity && !this->hasIndexingHeader(object))
            object->setButterfly(vm, nullptr);
        // If the object was down-sized to the point where the base of the Butterfly is no longer within the 
        // first CopiedBlock::blockSize bytes, we'll get the wrong answer if we try to mask the base back to 
        // the CopiedBlock header. To prevent this case we need to memmove the Butterfly down.
        else
            object->shiftButterflyAfterFlattening(locker, vm, this, afterOutOfLineCapacity);
    }
    
    WTF::storeStoreFence();
    object->setStructureIDDirectly(id());

    // We need to do a writebarrier here because the GC thread might be scanning the butterfly while
    // we are shuffling properties around. See: https://bugs.webkit.org/show_bug.cgi?id=166989
    vm.heap.writeBarrier(object);

    return this;
}

void Structure::pin(const AbstractLocker&, VM& vm, PropertyTable* table)
{
    setIsPinnedPropertyTable(true);
    setPropertyTable(vm, table);
    clearPreviousID();
    m_nameInPrevious = nullptr;
}

void Structure::pinForCaching(const AbstractLocker&, VM& vm, PropertyTable* table)
{
    setIsPinnedPropertyTable(true);
    setPropertyTable(vm, table);
    m_nameInPrevious = nullptr;
}

void Structure::allocateRareData(VM& vm)
{
    ASSERT(!hasRareData());
    StructureRareData* rareData = StructureRareData::create(vm, previousID());
    WTF::storeStoreFence();
    m_previousOrRareData.set(vm, this, rareData);
    ASSERT(hasRareData());
}

WatchpointSet* Structure::ensurePropertyReplacementWatchpointSet(VM& vm, PropertyOffset offset)
{
    ASSERT(!isUncacheableDictionary());

    // In some places it's convenient to call this with an invalid offset. So, we do the check here.
    if (!isValidOffset(offset))
        return nullptr;
    
    if (!hasRareData())
        allocateRareData(vm);
    ConcurrentJSLocker locker(m_lock);
    StructureRareData* rareData = this->rareData();
    if (!rareData->m_replacementWatchpointSets) {
        rareData->m_replacementWatchpointSets =
            std::make_unique<StructureRareData::PropertyWatchpointMap>();
        WTF::storeStoreFence();
    }
    auto result = rareData->m_replacementWatchpointSets->add(offset, nullptr);
    if (result.isNewEntry)
        result.iterator->value = adoptRef(new WatchpointSet(IsWatched));
    return result.iterator->value.get();
}

void Structure::startWatchingPropertyForReplacements(VM& vm, PropertyName propertyName)
{
    ASSERT(!isUncacheableDictionary());
    
    startWatchingPropertyForReplacements(vm, get(vm, propertyName));
}

void Structure::didCachePropertyReplacement(VM& vm, PropertyOffset offset)
{
    RELEASE_ASSERT(isValidOffset(offset));
    ensurePropertyReplacementWatchpointSet(vm, offset)->fireAll(vm, "Did cache property replacement");
}

void Structure::startWatchingInternalProperties(VM& vm)
{
    if (!isUncacheableDictionary()) {
        startWatchingPropertyForReplacements(vm, vm.propertyNames->toString);
        startWatchingPropertyForReplacements(vm, vm.propertyNames->valueOf);
    }
    setDidWatchInternalProperties(true);
}

#if DUMP_PROPERTYMAP_STATS

PropertyMapHashTableStats* propertyMapHashTableStats = 0;

struct PropertyMapStatisticsExitLogger {
    PropertyMapStatisticsExitLogger();
    ~PropertyMapStatisticsExitLogger();
};

DEFINE_GLOBAL_FOR_LOGGING(PropertyMapStatisticsExitLogger, logger, );

PropertyMapStatisticsExitLogger::PropertyMapStatisticsExitLogger()
{
    propertyMapHashTableStats = adoptPtr(new PropertyMapHashTableStats()).leakPtr();
}

PropertyMapStatisticsExitLogger::~PropertyMapStatisticsExitLogger()
{
    unsigned finds = propertyMapHashTableStats->numFinds;
    unsigned collisions = propertyMapHashTableStats->numCollisions;
    dataLogF("\nJSC::PropertyMap statistics for process %d\n\n", getCurrentProcessID());
    dataLogF("%d finds\n", finds);
    dataLogF("%d collisions (%.1f%%)\n", collisions, 100.0 * collisions / finds);
    dataLogF("%d lookups\n", propertyMapHashTableStats->numLookups.load());
    dataLogF("%d lookup probings\n", propertyMapHashTableStats->numLookupProbing.load());
    dataLogF("%d adds\n", propertyMapHashTableStats->numAdds.load());
    dataLogF("%d removes\n", propertyMapHashTableStats->numRemoves.load());
    dataLogF("%d rehashes\n", propertyMapHashTableStats->numRehashes.load());
    dataLogF("%d reinserts\n", propertyMapHashTableStats->numReinserts.load());
}

#endif

PropertyTable* Structure::copyPropertyTableForPinning(VM& vm)
{
    if (PropertyTable* table = propertyTableOrNull())
        return PropertyTable::clone(vm, *table);
    bool setPropertyTable = false;
    return materializePropertyTable(vm, setPropertyTable);
}

PropertyOffset Structure::getConcurrently(UniquedStringImpl* uid, unsigned& attributes)
{
    PropertyOffset result = invalidOffset;
    
    forEachPropertyConcurrently(
        [&] (const PropertyMapEntry& candidate) -> bool {
            if (candidate.key != uid)
                return true;
            
            result = candidate.offset;
            attributes = candidate.attributes;
            return false;
        });
    
    return result;
}

Vector<PropertyMapEntry> Structure::getPropertiesConcurrently()
{
    Vector<PropertyMapEntry> result;

    forEachPropertyConcurrently(
        [&] (const PropertyMapEntry& entry) -> bool {
            result.append(entry);
            return true;
        });
    
    return result;
}

PropertyOffset Structure::add(VM& vm, PropertyName propertyName, unsigned attributes)
{
    return add<ShouldPin::No>(
        vm, propertyName, attributes,
        [this] (const GCSafeConcurrentJSLocker&, PropertyOffset, PropertyOffset newLastOffset) {
            setLastOffset(newLastOffset);
        });
}

PropertyOffset Structure::remove(PropertyName propertyName)
{
    return remove(propertyName, [] (const ConcurrentJSLocker&, PropertyOffset) { });
}

void Structure::getPropertyNamesFromStructure(VM& vm, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    PropertyTable* table = ensurePropertyTableIfNotEmpty(vm);
    if (!table)
        return;
    
    bool knownUnique = propertyNames.canAddKnownUniqueForStructure();
    
    PropertyTable::iterator end = table->end();
    for (PropertyTable::iterator iter = table->begin(); iter != end; ++iter) {
        ASSERT(!isQuickPropertyAccessAllowedForEnumeration() || !(iter->attributes & PropertyAttribute::DontEnum));
        ASSERT(!isQuickPropertyAccessAllowedForEnumeration() || !iter->key->isSymbol());
        if (!(iter->attributes & PropertyAttribute::DontEnum) || mode.includeDontEnumProperties()) {
            if (iter->key->isSymbol() && !propertyNames.includeSymbolProperties())
                continue;
            if (knownUnique)
                propertyNames.addUnchecked(iter->key);
            else
                propertyNames.add(iter->key);
        }
    }
}

void StructureFireDetail::dump(PrintStream& out) const
{
    out.print("Structure transition from ", *m_structure);
}

DeferredStructureTransitionWatchpointFire::DeferredStructureTransitionWatchpointFire(VM& vm, Structure* structure)
    : DeferredWatchpointFire(vm)
    , m_structure(structure)
{
}

DeferredStructureTransitionWatchpointFire::~DeferredStructureTransitionWatchpointFire()
{
    fireAll();
}

void DeferredStructureTransitionWatchpointFire::dump(PrintStream& out) const
{
    out.print("Structure transition from ", *m_structure);
}

void Structure::didTransitionFromThisStructure(DeferredStructureTransitionWatchpointFire* deferred) const
{
    // If the structure is being watched, and this is the kind of structure that the DFG would
    // like to watch, then make sure to note for all future versions of this structure that it's
    // unwise to watch it.
    if (m_transitionWatchpointSet.isBeingWatched())
        const_cast<Structure*>(this)->setTransitionWatchpointIsLikelyToBeFired(true);

    if (deferred) {
        ASSERT(deferred->structure() == this);
        m_transitionWatchpointSet.fireAll(*vm(), deferred);
    } else
        m_transitionWatchpointSet.fireAll(*vm(), StructureFireDetail(this));
}

void Structure::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    Structure* thisObject = jsCast<Structure*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    Base::visitChildren(thisObject, visitor);
    
    ConcurrentJSLocker locker(thisObject->m_lock);
    
    visitor.append(thisObject->m_globalObject);
    if (!thisObject->isObject())
        thisObject->m_cachedPrototypeChain.clear();
    else {
        visitor.append(thisObject->m_prototype);
        visitor.append(thisObject->m_cachedPrototypeChain);
    }
    visitor.append(thisObject->m_previousOrRareData);

    if (thisObject->isPinnedPropertyTable() || thisObject->isAddingPropertyForTransition()) {
        // NOTE: This can interleave in pin(), in which case it may see a null property table.
        // That's fine, because then the barrier will fire and we will scan this again.
        visitor.append(thisObject->m_propertyTableUnsafe);
    } else if (visitor.isBuildingHeapSnapshot())
        visitor.append(thisObject->m_propertyTableUnsafe);
    else if (thisObject->m_propertyTableUnsafe)
        thisObject->m_propertyTableUnsafe.clear();
}

bool Structure::isCheapDuringGC()
{
    // FIXME: We could make this even safer by returning false if this structure's property table
    // has any large property names.
    // https://bugs.webkit.org/show_bug.cgi?id=157334
    
    return (!m_globalObject || Heap::isMarked(m_globalObject.get()))
        && (hasPolyProto() || !storedPrototypeObject() || Heap::isMarked(storedPrototypeObject()));
}

bool Structure::markIfCheap(SlotVisitor& visitor)
{
    if (!isCheapDuringGC())
        return Heap::isMarked(this);
    
    visitor.appendUnbarriered(this);
    return true;
}

Ref<StructureShape> Structure::toStructureShape(JSValue value, bool& sawPolyProtoStructure)
{
    Ref<StructureShape> baseShape = StructureShape::create();
    RefPtr<StructureShape> curShape = baseShape.ptr();
    Structure* curStructure = this;
    JSValue curValue = value;
    sawPolyProtoStructure = false;
    while (curStructure) {
        sawPolyProtoStructure |= curStructure->hasPolyProto();
        curStructure->forEachPropertyConcurrently(
            [&] (const PropertyMapEntry& entry) -> bool {
                if (!PropertyName(entry.key).isPrivateName())
                    curShape->addProperty(*entry.key);
                return true;
            });

        if (JSObject* curObject = curValue.getObject())
            curShape->setConstructorName(JSObject::calculatedClassName(curObject));
        else
            curShape->setConstructorName(curStructure->classInfo()->className);

        if (curStructure->isDictionary())
            curShape->enterDictionaryMode();

        curShape->markAsFinal();

        if (!curValue.isObject())
            break;

        JSObject* object = asObject(curValue);
        JSObject* prototypeObject = object->structure()->storedPrototypeObject(object);
        if (!prototypeObject)
            break;

        auto newShape = StructureShape::create();
        curShape->setProto(newShape.copyRef());
        curShape = WTFMove(newShape);
        curValue = prototypeObject;
        curStructure = prototypeObject->structure();
    }
    
    return baseShape;
}

void Structure::dump(PrintStream& out) const
{
    out.print(RawPointer(this), ":[", classInfo()->className, ", {");
    
    CommaPrinter comma;
    
    const_cast<Structure*>(this)->forEachPropertyConcurrently(
        [&] (const PropertyMapEntry& entry) -> bool {
            out.print(comma, entry.key, ":", static_cast<int>(entry.offset));
            return true;
        });
    
    out.print("}, ", IndexingTypeDump(indexingMode()));
    
    if (hasPolyProto())
        out.print(", PolyProto offset:", knownPolyProtoOffset);
    else if (m_prototype.get().isCell())
        out.print(", Proto:", RawPointer(m_prototype.get().asCell()));

    switch (dictionaryKind()) {
    case NoneDictionaryKind:
        if (hasBeenDictionary())
            out.print(", Has been dictionary");
        break;
    case CachedDictionaryKind:
        out.print(", Dictionary");
        break;
    case UncachedDictionaryKind:
        out.print(", UncacheableDictionary");
        break;
    }

    if (transitionWatchpointSetIsStillValid())
        out.print(", Leaf");
    else if (transitionWatchpointIsLikelyToBeFired())
        out.print(", Shady leaf");
    
    out.print("]");
}

void Structure::dumpInContext(PrintStream& out, DumpContext* context) const
{
    if (context)
        context->structures.dumpBrief(this, out);
    else
        dump(out);
}

void Structure::dumpBrief(PrintStream& out, const CString& string) const
{
    out.print("%", string, ":", classInfo()->className);
}

void Structure::dumpContextHeader(PrintStream& out)
{
    out.print("Structures:");
}

bool ClassInfo::hasStaticSetterOrReadonlyProperties() const
{
    for (const ClassInfo* ci = this; ci; ci = ci->parentClass) {
        if (const HashTable* table = ci->staticPropHashTable) {
            if (table->hasSetterOrReadonlyProperties)
                return true;
        }
    }
    return false;
}

void Structure::setCachedPropertyNameEnumerator(VM& vm, JSPropertyNameEnumerator* enumerator)
{
    ASSERT(!isDictionary());
    if (!hasRareData())
        allocateRareData(vm);
    rareData()->setCachedPropertyNameEnumerator(vm, enumerator);
}

JSPropertyNameEnumerator* Structure::cachedPropertyNameEnumerator() const
{
    if (!hasRareData())
        return nullptr;
    return rareData()->cachedPropertyNameEnumerator();
}

bool Structure::canCachePropertyNameEnumerator() const
{
    if (!this->canCacheOwnKeys())
        return false;

    StructureChain* structureChain = m_cachedPrototypeChain.get();
    ASSERT(structureChain);
    WriteBarrier<Structure>* structure = structureChain->head();
    while (true) {
        if (!structure->get())
            return true;
        if (!structure->get()->canCacheOwnKeys())
            return false;
        structure++;
    }

    ASSERT_NOT_REACHED();
    return true;
}
    
bool Structure::canAccessPropertiesQuicklyForEnumeration() const
{
    if (!isQuickPropertyAccessAllowedForEnumeration())
        return false;
    if (hasGetterSetterProperties())
        return false;
    if (isUncacheableDictionary())
        return false;
    return true;
}

} // namespace JSC
