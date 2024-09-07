#ifndef SELECTOR_H
#define SELECTOR_H

#include "AnalysisEvent.h"

class Selector {
public:
    virtual ~Selector() = default;

    virtual bool pass_selection(const AnalysisEvent& event) const = 0;
};

#endif