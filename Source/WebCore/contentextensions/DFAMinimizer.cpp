/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "DFAMinimizer.h"

#if ENABLE(CONTENT_EXTENSIONS)

#include "DFA.h"
#include "DFANode.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/StringHasher.h>
#include <wtf/Vector.h>

namespace WebCore {
namespace ContentExtensions {

namespace {

// simplifyTransitions() tries to collapse individual transitions into fallback transitions.
// After simplifyTransitions(), you can also make the assumption that a fallback transition target will never be
// also the target of an individual transition.
static void simplifyTransitions(DFA& dfa)
{
    for (DFANode& dfaNode : dfa.nodes) {
        bool addingFallback = false;
        uint32_t newFallbackDestination = std::numeric_limits<uint32_t>::max();
        if (!dfaNode.hasFallbackTransition()
            && ((dfaNode.transitionsLength() == 126 && !dfaNode.containsTransition(0, dfa))
                || (dfaNode.transitionsLength() == 127 && dfaNode.containsTransition(0, dfa)))) {
            unsigned bestTarget = std::numeric_limits<unsigned>::max();
            unsigned bestTargetScore = 0;
            HashMap<unsigned, unsigned, DefaultHash<unsigned>::Hash, WTF::UnsignedWithZeroKeyHashTraits<unsigned>> targetHistogram;
            for (const auto& transition : dfaNode.transitions(dfa)) {
                if (!transition.first)
                    continue;

                unsigned transitionTarget = transition.second;
                auto addResult = targetHistogram.add(transitionTarget, 1);
                if (!addResult.isNewEntry)
                    addResult.iterator->value++;

                if (addResult.iterator->value > bestTargetScore) {
                    bestTargetScore = addResult.iterator->value;
                    bestTarget = transitionTarget;
                }
            }
            ASSERT_WITH_MESSAGE(bestTargetScore, "There should be at least one valid target since having transitions is a precondition to enter this path.");

            newFallbackDestination = bestTarget;
            addingFallback = true;
        }
        
        // Use the same location to write new transitions possibly followed by unused memory.
        // We can do this because we are always decreasing the amount of memory used.
        // We will probably need something like setHasFallbackTransitionWithoutChangingDFA to do that.

        unsigned oldFallbackTransition = std::numeric_limits<uint32_t>::max();
        bool hadFallbackTransition = dfaNode.hasFallbackTransition();
        if (hadFallbackTransition)
            oldFallbackTransition = dfaNode.fallbackTransitionDestination(dfa);
        
        newFallbackDestination = (newFallbackDestination == std::numeric_limits<uint32_t>::max() ? oldFallbackTransition : newFallbackDestination);
        ASSERT(!addingFallback || (newFallbackDestination != std::numeric_limits<uint32_t>::max() && oldFallbackTransition == std::numeric_limits<uint32_t>::max()));
        bool willHaveFallback = newFallbackDestination != std::numeric_limits<uint32_t>::max();
        dfaNode.setHasFallbackTransitionWithoutChangingDFA(willHaveFallback);
        
        if (willHaveFallback) {
            Vector<std::pair<uint8_t, uint32_t>> transitions = dfaNode.transitions(dfa);
            unsigned availableSlotCount = transitions.size() + hadFallbackTransition;
            for (unsigned i = 0; i < transitions.size(); ++i) {
                if (transitions[i].second == newFallbackDestination)
                    transitions.remove(i--);
            }
        
            RELEASE_ASSERT(transitions.size() + willHaveFallback <= availableSlotCount);
        
            unsigned firstSlot = dfaNode.transitionsStart();
            dfaNode.resetTransitions(firstSlot, transitions.size());
            for (unsigned i = 0; i < transitions.size(); ++i)
                dfa.transitions[firstSlot + i] = transitions[i];
            for (unsigned i = transitions.size(); i < availableSlotCount; ++i) {
                // Invalidate now-unused memory to make finding bugs easier.
                dfa.transitions[firstSlot + i] = std::make_pair(std::numeric_limits<uint8_t>::max(), std::numeric_limits<uint32_t>::max());
            }
            if (willHaveFallback)
                dfa.transitions[firstSlot + transitions.size()] = std::make_pair(std::numeric_limits<uint8_t>::max(), newFallbackDestination);
        }
    }
}

class Partition {
public:
    void initialize(unsigned size)
    {
        if (!size)
            return;

        m_sets.reserveInitialCapacity(size);
        m_partitionedElements.resize(size);
        m_elementPositionInPartitionedNodes.resize(size);
        m_elementToSetMap.resize(size);

        for (unsigned i = 0; i < size; ++i) {
            m_partitionedElements[i] = i;
            m_elementPositionInPartitionedNodes[i] = i;
            m_elementToSetMap[i] = 0;
        }
        m_sets.append(SetDescriptor({ 0, size, 0 }));
    }

    void markElementInCurrentGeneration(unsigned elementIndex)
    {
        // Swap the node with the first unmarked node.
        unsigned setIndex = m_elementToSetMap[elementIndex];
        SetDescriptor& setDescriptor = m_sets[setIndex];

        unsigned elementPositionInPartition = m_elementPositionInPartitionedNodes[elementIndex];
        ASSERT(elementPositionInPartition >= setDescriptor.start);
        ASSERT(elementPositionInPartition < setDescriptor.end());

        unsigned firstUnmarkedElementPositionInPartition = setDescriptor.indexAfterMarkedElements();
        ASSERT(firstUnmarkedElementPositionInPartition >= setDescriptor.start && firstUnmarkedElementPositionInPartition < setDescriptor.end());
        ASSERT(firstUnmarkedElementPositionInPartition >= firstUnmarkedElementPositionInPartition);

        // Swap the nodes in the set.
        unsigned firstUnmarkedElement = m_partitionedElements[firstUnmarkedElementPositionInPartition];
        m_partitionedElements[firstUnmarkedElementPositionInPartition] = elementIndex;
        m_partitionedElements[elementPositionInPartition] = firstUnmarkedElement;

        // Update their index.
        m_elementPositionInPartitionedNodes[elementIndex] = firstUnmarkedElementPositionInPartition;
        m_elementPositionInPartitionedNodes[firstUnmarkedElement] = elementPositionInPartition;

        if (!setDescriptor.markedCount) {
            ASSERT(!m_setsMarkedInCurrentGeneration.contains(setIndex));
            m_setsMarkedInCurrentGeneration.append(setIndex);
        }
        ++setDescriptor.markedCount;
    }

    // The function passed as argument MUST not modify the partition.
    template<typename Function>
    void refineGeneration(const Function& function)
    {
        for (unsigned setIndex : m_setsMarkedInCurrentGeneration) {
            SetDescriptor& setDescriptor = m_sets[setIndex];
            if (setDescriptor.markedCount == setDescriptor.size) {
                // Everything is marked, there is nothing to refine.
                setDescriptor.markedCount = 0;
                continue;
            }

            SetDescriptor newSet;
            bool newSetIsMarkedSet = setDescriptor.markedCount * 2 <= setDescriptor.size;
            if (newSetIsMarkedSet) {
                // Less than half of the nodes have been marked.
                newSet = { setDescriptor.start, setDescriptor.markedCount, 0 };
                setDescriptor.start = setDescriptor.start + setDescriptor.markedCount;
            } else
                newSet = { setDescriptor.start + setDescriptor.markedCount, setDescriptor.size - setDescriptor.markedCount, 0 };
            setDescriptor.size -= newSet.size;
            setDescriptor.markedCount = 0;

            unsigned newSetIndex = m_sets.size();
            m_sets.append(newSet);

            for (unsigned i = newSet.start; i < newSet.end(); ++i)
                m_elementToSetMap[m_partitionedElements[i]] = newSetIndex;

            function(newSetIndex);
        }
        m_setsMarkedInCurrentGeneration.clear();
    }

    // Call Function() on every node of a given subset.
    template<typename Function>
    void iterateSet(unsigned setIndex, const Function& function)
    {
        SetDescriptor& setDescriptor = m_sets[setIndex];
        for (unsigned i = setDescriptor.start; i < setDescriptor.end(); ++i)
            function(m_partitionedElements[i]);
    }

    // Index of the set containing the Node.
    unsigned setIndex(unsigned elementIndex) const
    {
        return m_elementToSetMap[elementIndex];
    }

    // NodeIndex of the first element in the set.
    unsigned firstElementInSet(unsigned setIndex) const
    {
        return m_partitionedElements[m_sets[setIndex].start];
    }

    unsigned size() const
    {
        return m_sets.size();
    }

private:
    struct SetDescriptor {
        unsigned start;
        unsigned size;
        unsigned markedCount;

        unsigned indexAfterMarkedElements() const { return start + markedCount; }
        unsigned end() const { return start + size; }
    };

    // List of sets.
    Vector<SetDescriptor> m_sets;

    // All the element indices such that two elements of the same set never have a element of a different set between them.
    Vector<unsigned> m_partitionedElements;

    // Map elementIndex->position in the partitionedElements.
    Vector<unsigned> m_elementPositionInPartitionedNodes;

    // Map elementIndex->SetIndex.
    Vector<unsigned> m_elementToSetMap;

    // List of sets with any marked node. Each set can appear at most once.
    // FIXME: find a good inline size for this.
    Vector<unsigned, 128> m_setsMarkedInCurrentGeneration;
};

class FullGraphPartition {
public:
    FullGraphPartition(const DFA& dfa)
    {
        m_nodePartition.initialize(dfa.nodes.size());

        m_flattenedTransitionsStartOffsetPerNode.resize(dfa.nodes.size());
        for (unsigned& counter : m_flattenedTransitionsStartOffsetPerNode)
            counter = 0;

        m_flattenedFallbackTransitionsStartOffsetPerNode.resize(dfa.nodes.size());
        for (unsigned& counter : m_flattenedFallbackTransitionsStartOffsetPerNode)
            counter = 0;

        // Count the number of incoming transitions per node.
        for (const DFANode& dfaNode : dfa.nodes) {
            for (const auto& transition : dfaNode.transitions(dfa))
                ++m_flattenedTransitionsStartOffsetPerNode[transition.second];
            if (dfaNode.hasFallbackTransition())
                ++m_flattenedFallbackTransitionsStartOffsetPerNode[dfaNode.fallbackTransitionDestination(dfa)];
        }

        // Accumulate the offsets.
        unsigned transitionAccumulator = 0;
        for (unsigned i = 0; i < m_flattenedTransitionsStartOffsetPerNode.size(); ++i) {
            unsigned transitionsCountForNode = m_flattenedTransitionsStartOffsetPerNode[i];
            m_flattenedTransitionsStartOffsetPerNode[i] = transitionAccumulator;
            transitionAccumulator += transitionsCountForNode;
        }
        unsigned flattenedTransitionsSize = transitionAccumulator;

        unsigned fallbackTransitionAccumulator = 0;
        for (unsigned i = 0; i < m_flattenedFallbackTransitionsStartOffsetPerNode.size(); ++i) {
            unsigned fallbackTransitionsCountForNode = m_flattenedFallbackTransitionsStartOffsetPerNode[i];
            m_flattenedFallbackTransitionsStartOffsetPerNode[i] = fallbackTransitionAccumulator;
            fallbackTransitionAccumulator += fallbackTransitionsCountForNode;
        }
        unsigned flattenedFallbackTransitionsSize = fallbackTransitionAccumulator;

        // Next, let's fill the transition table and set up the size of each group at the same time.
        m_flattenedTransitionsSizePerNode.resize(dfa.nodes.size());
        for (unsigned& counter : m_flattenedTransitionsSizePerNode)
            counter = 0;

        m_flattenedFallbackTransitionsSizePerNode.resize(dfa.nodes.size());
        for (unsigned& counter : m_flattenedFallbackTransitionsSizePerNode)
            counter = 0;

        m_flattenedTransitions.resize(flattenedTransitionsSize);

        m_flattenedFallbackTransitions.resize(flattenedFallbackTransitionsSize);

        for (unsigned i = 0; i < dfa.nodes.size(); ++i) {
            const DFANode& dfaNode = dfa.nodes[i];
            for (const auto& transition : dfaNode.transitions(dfa)) {
                unsigned targetNodeIndex = transition.second;

                unsigned start = m_flattenedTransitionsStartOffsetPerNode[targetNodeIndex];
                unsigned offset = m_flattenedTransitionsSizePerNode[targetNodeIndex];

                m_flattenedTransitions[start + offset] = Transition({ i, targetNodeIndex, transition.first });

                ++m_flattenedTransitionsSizePerNode[targetNodeIndex];
            }
            if (dfaNode.hasFallbackTransition()) {
                unsigned targetNodeIndex = dfaNode.fallbackTransitionDestination(dfa);

                unsigned start = m_flattenedFallbackTransitionsStartOffsetPerNode[targetNodeIndex];
                unsigned offset = m_flattenedFallbackTransitionsSizePerNode[targetNodeIndex];

                m_flattenedFallbackTransitions[start + offset] = i;
                ++m_flattenedFallbackTransitionsSizePerNode[targetNodeIndex];
            }
        }

        // Create the initial partition of transition. Each character differentiating a transiton
        // from an other gets its own set in the partition.
        m_transitionPartition.initialize(m_flattenedTransitions.size());
        for (uint16_t i = 0; i < 128; ++i) {
            for (unsigned transitionIndex = 0; transitionIndex < m_flattenedTransitions.size(); ++transitionIndex) {
                const Transition& transition = m_flattenedTransitions[transitionIndex];
                if (transition.character == i)
                    m_transitionPartition.markElementInCurrentGeneration(transitionIndex);
            }
            m_transitionPartition.refineGeneration([](unsigned) { });
        }

        // Fallback partitions are considered as a special type of differentiator, we don't split them initially.
        m_fallbackTransitionPartition.initialize(m_flattenedFallbackTransitions.size());
    }

    void markNode(unsigned nodeIndex)
    {
        m_nodePartition.markElementInCurrentGeneration(nodeIndex);
    }

    void refinePartitions()
    {
        m_nodePartition.refineGeneration([&](unsigned smallestSetIndex) {
            m_nodePartition.iterateSet(smallestSetIndex, [&](unsigned nodeIndex) {
                unsigned incomingTransitionsStartForNode = m_flattenedTransitionsStartOffsetPerNode[nodeIndex];
                unsigned incomingTransitionsSizeForNode = m_flattenedTransitionsSizePerNode[nodeIndex];

                for (unsigned i = 0; i < incomingTransitionsSizeForNode; ++i)
                    m_transitionPartition.markElementInCurrentGeneration(incomingTransitionsStartForNode + i);

                unsigned incomingFallbackTransitionsStartForNode = m_flattenedFallbackTransitionsStartOffsetPerNode[nodeIndex];
                unsigned incomingFallbackTransitionsSizeForNode = m_flattenedFallbackTransitionsSizePerNode[nodeIndex];

                for (unsigned i = 0; i < incomingFallbackTransitionsSizeForNode; ++i)
                    m_fallbackTransitionPartition.markElementInCurrentGeneration(incomingFallbackTransitionsStartForNode + i);
            });

            // We only need to split the transitions, we handle the new sets through the main loop.
            m_transitionPartition.refineGeneration([](unsigned) { });
            m_fallbackTransitionPartition.refineGeneration([](unsigned) { });
        });
    }

    void splitByUniqueTransitions()
    {
        for (; m_nextTransitionSetToProcess < m_transitionPartition.size(); ++m_nextTransitionSetToProcess) {
            // We use the known splitters to refine the set.
            m_transitionPartition.iterateSet(m_nextTransitionSetToProcess, [&](unsigned transitionIndex) {
                unsigned sourceNodeIndex = m_flattenedTransitions[transitionIndex].source;
                m_nodePartition.markElementInCurrentGeneration(sourceNodeIndex);
            });

            refinePartitions();
        }
    }

    void splitByFallbackTransitions()
    {
        ASSERT_WITH_MESSAGE(m_nextTransitionSetToProcess || !m_transitionPartition.size(), "We can only distinguish nodes by fallback transition *after* all other transitions are covered. Doing otherwise would be incorrect since the unique transitions from 2 nodes could cover all possible transitions.");

        for (unsigned fallbackTransitionSetIndex = 0; fallbackTransitionSetIndex < m_fallbackTransitionPartition.size(); ++fallbackTransitionSetIndex) {

            m_fallbackTransitionPartition.iterateSet(fallbackTransitionSetIndex, [&](unsigned transitionIndex) {
                unsigned sourceNodeIndex = m_flattenedFallbackTransitions[transitionIndex];
                m_nodePartition.markElementInCurrentGeneration(sourceNodeIndex);
            });
            refinePartitions();

            // Any new split need to spread to all the unique transition that can reach the two new sets.
            splitByUniqueTransitions();
        }
    }

    unsigned nodeReplacement(unsigned nodeIndex)
    {
        unsigned setIndex = m_nodePartition.setIndex(nodeIndex);
        return m_nodePartition.firstElementInSet(setIndex);
    }

private:
    struct Transition {
        unsigned source;
        unsigned target;
        uint16_t character;
    };

    Vector<unsigned> m_flattenedTransitionsStartOffsetPerNode;
    Vector<unsigned> m_flattenedTransitionsSizePerNode;
    Vector<unsigned> m_flattenedFallbackTransitionsStartOffsetPerNode;
    Vector<unsigned> m_flattenedFallbackTransitionsSizePerNode;

    Vector<Transition> m_flattenedTransitions;
    Vector<unsigned> m_flattenedFallbackTransitions;

    Partition m_nodePartition;
    Partition m_transitionPartition;
    Partition m_fallbackTransitionPartition;

    unsigned m_nextTransitionSetToProcess { 0 };
};

struct ActionKey {
    enum DeletedValueTag { DeletedValue };
    explicit ActionKey(DeletedValueTag) { state = Deleted; }

    enum EmptyValueTag { EmptyValue };
    explicit ActionKey(EmptyValueTag) { state = Empty; }

    explicit ActionKey(const DFA* dfa, uint32_t actionsStart, uint16_t actionsLength)
        : dfa(dfa)
        , actionsStart(actionsStart)
        , actionsLength(actionsLength)
    {
        StringHasher hasher;
        hasher.addCharactersAssumingAligned(reinterpret_cast<const UChar*>(&dfa->actions[actionsStart]), actionsLength * sizeof(uint64_t) / sizeof(UChar));
        hash = hasher.hash();
    }

    bool isEmptyValue() const { return state == Empty; }
    bool isDeletedValue() const { return state == Deleted; }

    unsigned hash;
    
    const DFA* dfa;
    uint32_t actionsStart;
    uint16_t actionsLength;
    
    enum {
        Valid,
        Empty,
        Deleted
    } state;
};

struct ActionKeyHash {
    static unsigned hash(const ActionKey& actionKey)
    {
        return actionKey.hash;
    }

    // FIXME: Release builds on Mavericks fail with this inlined.
    __attribute__((noinline)) static bool equal(const ActionKey& a, const ActionKey& b)
    {
        if (a.state != b.state
            || a.dfa != b.dfa
            || a.actionsLength != b.actionsLength)
            return false;
        for (uint16_t i = 0; i < a.actionsLength; ++i) {
            if (a.dfa->actions[a.actionsStart + i] != a.dfa->actions[b.actionsStart + i])
                return false;
        }
        return true;
    }
    static const bool safeToCompareToEmptyOrDeleted = false;
};

struct ActionKeyHashTraits : public WTF::CustomHashTraits<ActionKey> {
    static const bool emptyValueIsZero = true;
};

} // anonymous namespace.

void DFAMinimizer::minimize(DFA& dfa)
{
    simplifyTransitions(dfa);

    FullGraphPartition fullGraphPartition(dfa);

    // Unlike traditional minimization final states can be differentiated by their action.
    // Instead of creating a single set for the final state, we partition by actions from
    // the start.
    HashMap<ActionKey, Vector<unsigned>, ActionKeyHash, ActionKeyHashTraits> finalStates;
    for (unsigned i = 0; i < dfa.nodes.size(); ++i) {
        const DFANode& node = dfa.nodes[i];
        if (node.hasActions()) {
            // FIXME: Sort the actions in the dfa to make nodes that have the same actions in different order equal.
            auto addResult = finalStates.add(ActionKey(&dfa, node.actionsStart(), node.actionsLength()), Vector<unsigned>());
            addResult.iterator->value.append(i);
        }
    }

    for (const auto& slot : finalStates) {
        for (unsigned finalStateIndex : slot.value)
            fullGraphPartition.markNode(finalStateIndex);
        fullGraphPartition.refinePartitions();
    }

    // Use every splitter to refine the node partitions.
    fullGraphPartition.splitByUniqueTransitions();

    // At this stage, we know that no pair of state can be distinguished by the individual transitions. They can still
    // be distinguished by their fallback transitions.
    //
    // For example, two states [1, 2] can both have a transition on 'x' to a state [3], but each have fallback transition
    // to different states [4] and [5].
    //
    // Here, we distinguish such cases and at each stage refine the new discovered sets by their individual transitions.
    fullGraphPartition.splitByFallbackTransitions();

    Vector<unsigned> relocationVector;
    relocationVector.reserveInitialCapacity(dfa.nodes.size());
    for (unsigned i = 0; i < dfa.nodes.size(); ++i)
        relocationVector.uncheckedAppend(i);

    // Kill the useless nodes and keep track of the new node transitions should point to.
    for (unsigned i = 0; i < dfa.nodes.size(); ++i) {
        unsigned replacement = fullGraphPartition.nodeReplacement(i);
        if (i != replacement) {
            relocationVector[i] = replacement;
            dfa.nodes[i].kill(dfa);
        }
    }

    for (DFANode& node : dfa.nodes) {
        auto nodeTransitions = node.transitions(dfa);
        for (unsigned i = 0; i < node.transitionsLength(); ++i)
            dfa.transitions[node.transitionsStart() + i].second = relocationVector[nodeTransitions[i].second];
        if (node.hasFallbackTransition())
            node.changeFallbackTransition(dfa, relocationVector[node.fallbackTransitionDestination(dfa)]);
    }

    // After minimizing, there is no guarantee individual transition are still poiting to different states.
    // The state pointed by one individual transition and the fallback states may have been merged.
    simplifyTransitions(dfa);

    dfa.root = relocationVector[dfa.root];
}

} // namespace ContentExtensions
} // namespace WebCore

#endif // ENABLE(CONTENT_EXTENSIONS)
