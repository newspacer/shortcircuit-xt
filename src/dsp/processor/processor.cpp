/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "processor.h"
#include "processor_defs.h"

#include <functional>
#include <new>
#include <cassert>

namespace scxt::dsp::processor
{

/**
 * This code blob uses constexpr expansion to allow us to convert a runtime integer to
 * a template argument without a big case statement.
 */
namespace detail
{

/**
 * Booleans
 */

using boolOp_t = bool (*)();
using constCharOp_t = const char *(*)();

template <size_t I> bool implCanInPlace()
{
    if constexpr (I == ProcessorType::proct_none)
        return true;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return false;
    else
        return sizeof(typename ProcessorImplementor<(ProcessorType)I>::T) <
               processorMemoryBufferSize;
}

template <size_t... Is> auto canInPlace(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implCanInPlace<Is>...};
    return fnc[ft]();
}

template <size_t I> bool implIsProcessorImplemented()
{
    if constexpr (I == ProcessorType::proct_none)
        return true;

    return !std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value;
}

template <size_t... Is> auto isProcessorImplemented(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsProcessorImplemented<Is>...};
    return fnc[ft]();
}

template <size_t I> bool implIsZoneProcessor()
{
    if constexpr (I == ProcessorType::proct_none)
        return true;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return false;
    else
        return ProcessorImplementor<(ProcessorType)I>::T::isZoneProcessor;
}

template <size_t... Is> auto isZoneProcessor(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsZoneProcessor<Is>...};
    return fnc[ft]();
}

template <size_t I> bool implIsPartProcessor()
{
    if constexpr (I == ProcessorType::proct_none)
        return true;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return false;
    else
        return ProcessorImplementor<(ProcessorType)I>::T::isPartProcessor;
}

template <size_t... Is> auto isPartProcessor(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsPartProcessor<Is>...};
    return fnc[ft]();
}

template <size_t I> bool implIsFXProcessor()
{
    if constexpr (I == ProcessorType::proct_none)
        return true;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return false;
    else
        return ProcessorImplementor<(ProcessorType)I>::T::isFXProcessor;
}

template <size_t... Is> auto isFXProcessor(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsFXProcessor<Is>...};
    return fnc[ft]();
}

template <size_t I> const char *implGetProcessorName()
{
    if constexpr (I == ProcessorType::proct_none)
        return "Off";

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return "error";
    else
        return ProcessorImplementor<(ProcessorType)I>::T::processorName;
}

template <size_t... Is> auto getProcessorName(size_t ft, std::index_sequence<Is...>)
{
    constexpr constCharOp_t fnc[] = {detail::implGetProcessorName<Is>...};
    return fnc[ft]();
}

template <size_t I> const char *implGetProcessorStreamingName()
{
    if constexpr (I == ProcessorType::proct_none)
        return "none";

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return "error";
    else
        return ProcessorImplementor<(ProcessorType)I>::T::processorStreamingName;
}

template <size_t... Is> auto getProcessorStreamingName(size_t ft, std::index_sequence<Is...>)
{
    constexpr constCharOp_t fnc[] = {detail::implGetProcessorStreamingName<Is>...};
    return fnc[ft]();
}

template <size_t I> Processor *returnSpawnOnto(uint8_t *m, float *fp, int *ip, bool st)
{
    if constexpr (I == ProcessorType::proct_none)
        return nullptr;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
    {
        return nullptr;
    }
    else
    {
        auto mem = new (m) typename ProcessorImplementor<(ProcessorType)I>::T(fp, ip, st);
        return mem;
    }
}

template <size_t... Is>
auto spawnOnto(size_t ft, uint8_t *m, float *fp, int *ip, bool st, std::index_sequence<Is...>)
{
    using FuncType = Processor *(*)(uint8_t *, float *, int *, bool);
    constexpr FuncType arFuncs[] = {detail::returnSpawnOnto<Is>...};
    return arFuncs[ft](m, fp, ip, st);
}
} // namespace detail

bool canInPlaceNew(ProcessorType id)
{
    return detail::canInPlace(id,
                              std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

bool isProcessorImplemented(ProcessorType id)
{
    return detail::isProcessorImplemented(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

bool isZoneProcessor(ProcessorType id)
{
    return detail::isZoneProcessor(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

bool isPartProcessor(ProcessorType id)
{
    return detail::isPartProcessor(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

bool isFXProcessor(ProcessorType id)
{
    return detail::isFXProcessor(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

const char *getProcessorName(ProcessorType id)
{
    return detail::getProcessorName(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

const char *getProcessorStreamingName(ProcessorType id)
{
    return detail::getProcessorStreamingName(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

std::optional<ProcessorType> fromProcessorStreamingName(const std::string &s)
{
    // A bit gross but hey
    for (auto i = 0; i < proct_num_types; ++i)
    {
        if (getProcessorStreamingName((ProcessorType)i) == s)
            return (ProcessorType)i;
    }
    return {};
}

processorList_t getAllProcessorDescriptions()
{
    processorList_t res;
    for (auto i = 0U; i < (size_t)ProcessorType::proct_num_types; ++i)
    {
        auto pt = (ProcessorType)i;
        if (isProcessorImplemented(pt))
        {
            res.push_back({pt, getProcessorStreamingName(pt), getProcessorName(pt),
                           isZoneProcessor(pt), isPartProcessor(pt), isFXProcessor(pt)});
        }
    }
    return res;
}

/**
 * Spawn with in-place new onto a pre-allocated block. The memory must
 * be a 16byte aligned block of at least size processorMemoryBufferSize.
 */
Processor *spawnProcessorInPlace(ProcessorType id, uint8_t *memory, size_t memorySize, float *fp,
                                 int *ip, bool stereo)
{
    assert(memorySize >= processorMemoryBufferSize);
    return detail::spawnOnto(id, memory, fp, ip, stereo,
                             std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

/**
 * Spawn a Processor, potentially allocating memory. Call this if canInPlaceNew
 * returns false.
 *
 * TODO: Make it so we can remove this
 */
Processor *spawnProcessorAllocating(int id, float *fp, int *ip, bool stereo)
{
    assert(false);
    return 0;
}

/**
 * Whetner a Processor is spawned in place or onto fresh memory, release it here.
 */
void unspawnProcessor(Processor *f)
{
    if (!f)
        return;

    if (canInPlaceNew(f->getType()))
    {
        f->~Processor();
    }
    else
    {
        delete f;
    }
}

ProcessorControlDescription Processor::getControlDescription()
{
    ProcessorControlDescription res;
    res.type = getType();
    res.typeDisplayName = getName();
    res.numFloatParams = getFloatParameterCount();
    for (int i = 0; i < res.numFloatParams; ++i)
    {
        res.floatControlNames[i] = ctrllabel[i];
        res.floatControlDescriptions[i] = ctrlmode_desc[i];
    }
    res.numIntParams = getIntParameterCount();
    for (int i = 0; i < res.numIntParams; ++i)
    {
        res.intControlNames[i] = getIntParameterLabel(i);
        auto cd = datamodel::ControlDescription{datamodel::ControlDescription::INT,
                                                datamodel::ControlDescription::LINEAR,
                                                0,
                                                1,
                                                (float)getIntParameterChoicesCount(i),
                                                0,
                                                "",
                                                1,
                                                0};
        for (int j = 0;
             j < getIntParameterChoicesCount(i) && j < datamodel::ControlDescription::maxIntChoices; ++j)
        {
            strncpy(cd.choices[j], getIntParameterChoicesLabel(i, j), 32);
            cd.choices[j][31] = 0;
        }
        res.intControlDescriptions[i] = cd;
    }
    return res;
}

} // namespace scxt::dsp::processor