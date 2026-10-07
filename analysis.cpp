#include "analysis.h"

namespace Analysis {

PeriodStats computeStats(const QVector<PricePoint> &points)
{
    PeriodStats s;
    s.points = points.size();
    if (points.size() < 2)
        return s;

    s.first = points.first().price;
    s.last = points.last().price;
    s.low = s.first;
    s.high = s.first;

    double sum = 0.0;
    for (const PricePoint &p : points) {
        sum += p.price;
        if (p.price < s.low)
            s.low = p.price;
        if (p.price > s.high)
            s.high = p.price;
    }
    s.average = sum / points.size();
    s.changePercent = s.first != 0.0 ? (s.last - s.first) / s.first * 100.0 : 0.0;
    s.valid = true;
    return s;
}

GoldRatios computeGoldRatios(const QVector<PriceItem> &items)
{
    GoldRatios r;

    auto find = [&items](const char *key) -> const PriceItem * {
        for (const PriceItem &it : items) {
            if (it.key == QLatin1String(key))
                return &it;
        }
        return nullptr;
    };
    auto usable = [](const PriceItem *p) {
        return p && p->found && !p->stale && p->price > 0.0;
    };

    const PriceItem *dollar = find("price_dollar_rl");
    const PriceItem *gram18 = find("geram18");
    const PriceItem *gram24 = find("geram24");
    const PriceItem *ounce = find("ons");
    if (!usable(dollar) || !usable(gram18) || !usable(gram24) || !usable(ounce))
        return r;

    constexpr double kGramsPerTroyOunce = 31.1034768;

    r.usdPerGram18 = gram18->price / dollar->price;
    r.usdPerGram24 = gram24->price / dollar->price;
    r.globalUsdPerGram24 = ounce->price / kGramsPerTroyOunce;
    r.premiumPercent = (r.usdPerGram24 / r.globalUsdPerGram24 - 1.0) * 100.0;
    r.gramsPer1000Usd = 1000.0 / r.usdPerGram18;
    r.valid = true;
    return r;
}

} // namespace Analysis