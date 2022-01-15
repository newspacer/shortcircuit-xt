//
// Created by Paul Walker on 1/15/22.
//

#ifndef SHORTCIRCUIT_DATAAPPLICATORS_H
#define SHORTCIRCUIT_DATAAPPLICATORS_H

#include "sampler_wrapper_actiondata.h"
#include "data/ParameterProxy.h"

namespace scxt
{
namespace data
{

template <typename T> inline bool applyActionData(const actiondata &ad, ParameterProxy<T> &proxy)
{
    if (!std::holds_alternative<VAction>(ad.actiontype))
        return false;

    proxy.id = ad.id;
    proxy.subid = ad.subid;

    auto at = std::get<VAction>(ad.actiontype);
    switch (at)
    {
    case vga_floatval:
        proxy.setFrom(ad.data.f[0]);
        return true;
        break;
    case vga_intval:
        proxy.setFrom(ad.data.i[0]);
        return true;
        break;
    case vga_text:
        proxy.setFrom(std::string(ad.data.str));
        return true;
        break;
    case vga_disable_state:
        proxy.disabled = ad.data.i[0];
        return true;
        break;
    case vga_hide:
        proxy.hidden = ad.data.i[0];
        return true;
        break;
    case vga_label:
        proxy.label = (char *)(&ad.data.str[0]); // fixme - check for overruns
        return true;
        break;
    case vga_datamode:
        proxy.parseDatamode(ad.data.str);
        return true;
        break;
    case vga_set_range_and_units:
    {
        proxy.paramRangesSet = true;
        if (ad.data.i[0] == 'f')
        {
            proxy.min = ad.data.f[1];
            proxy.max = ad.data.f[2];
            proxy.def = ad.data.f[3];
            proxy.step = ad.data.f[4];
        }
        else
        {
            proxy.min = ad.data.i[1];
            proxy.max = ad.data.i[2];
            proxy.def = ad.data.i[3];
            proxy.step = ad.data.i[4];
        }
        return true;
    }
    break;
    default:
        break;
    }
    return false;
}

template <typename T, size_t N>
inline bool applyActionData(const actiondata &ad, int base, ParameterProxy<T> (&proxy)[N])
{
    auto idx = ad.id - base;

    auto res = applyActionData(ad, proxy[idx]);

    return res;
}

template <typename T>
inline bool applyActionDataIf(const actiondata &ad, const InteractionId &id,
                              ParameterProxy<T> &proxy)
{
    if (ad.id == id && applyActionData(ad, proxy))
        return true;
    return false;
}
template <typename T, size_t N>
inline bool applyActionDataSubIdIf(const actiondata &ad, const InteractionId &id,
                                   ParameterProxy<T> (&proxy)[N])
{
    if (ad.id == id && applyActionData(ad, proxy[ad.subid]))
        return true;
    return false;
}

template <typename T, size_t N, typename F>
inline bool applyToOneOrAll(const actiondata &ad, T (&indexedElement)[N], F getFunction)
{
    bool res = false;
    if (ad.subid < 0)
        for (auto &q : indexedElement)
        {
            auto &r = getFunction(q);
            res = applyActionData(ad, r) || res;
        }
    else
        res = applyActionData(ad, (getFunction(indexedElement[ad.subid])));
    return res;
}

} // namespace data
} // namespace scxt
#endif // SHORTCIRCUIT_DATAAPPLICATORS_H