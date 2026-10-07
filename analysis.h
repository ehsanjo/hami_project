#pragma once

#include <QVector>

#include "pricedatabase.h"
#include "pricefetcher.h"

namespace Analysis {

struct PeriodStats {
    bool valid = false;       // false when there are fewer than 2 points
    int points = 0;
    double first = 0.0;
    double last = 0.0;
    double low = 0.0;
    double high = 0.0;
    double average = 0.0;
    double changePercent = 0.0; // from the first to the last point
};

// Statistics over the given points (oldest first).
PeriodStats computeStats(const QVector<PricePoint> &points);

struct GoldRatios {
    bool valid = false;            // false if a needed price is missing or stale
    double usdPerGram18 = 0.0;     // local 18k gram price / open-market dollar
    double usdPerGram24 = 0.0;     // local 24k gram price / open-market dollar
    double globalUsdPerGram24 = 0.0; // global ounce price / 31.1035
    double premiumPercent = 0.0;   // local 24k in USD vs global, in percent
    double gramsPer1000Usd = 0.0;  // grams of 18k gold that 1,000 USD buys
};

// Needs the items price_dollar_rl, geram18, geram24 and ons to be fresh.
GoldRatios computeGoldRatios(const QVector<PriceItem> &items);

} // namespace Analysis