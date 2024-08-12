#pragma once

#include "TH1.h"

enum EventCategory
{
    kUnknown = 0,

    kSignalCCQE = 1,
    kSignalCCMEC = 2,
    kSignalCCRES = 3,
    kSignalOther = 4,

    kNuMuCCNhyp = 5,
    kNuMuCC0kshrt0hyp = 6,
    kNuMuCCOther = 7,
    kNC = 8,
    kOOFV = 9,
    kOther = 10,
};

class EventCategoryInterpreter
{
public:
    EventCategoryInterpreter( const EventCategoryInterpreter& ) = delete;
    EventCategoryInterpreter( EventCategoryInterpreter&& ) = delete;
    EventCategoryInterpreter& operator=( const EventCategoryInterpreter& ) = delete;
    EventCategoryInterpreter& operator=( EventCategoryInterpreter&& ) = delete;

    inline static const EventCategoryInterpreter& Instance()
    {
        static std::unique_ptr<EventCategoryInterpreter> the_instance( new EventCategoryInterpreter() );
        return *the_instance;
    }

    inline const std::map<EventCategory, std::string>& label_map() const
    {
        return event_category_to_label_map_;
    }

    inline std::string label(EventCategory ec) const
    {
        return event_category_to_label_map_.at(ec); 
    }

    inline int colour_code(EventCategory ec) const 
    {
        return event_category_to_colour_map_.at(ec);
    }

private:
    EventCategoryInterpreter() {} 
    
    std::map<EventCategory, std::string> event_category_to_label_map_ = {
        { kUnknown, "Unknown" },
        { kSignalCCQE, "Signal (CCQE)" },
        { kSignalCCMEC, "Signal (CCMEC)" },
        { kSignalCCRES, "Signal (CCRES)" },
        { kSignalOther, "Signal (Other)" },
        { kNuMuCCNhyp, "#nu_{#mu} CCNhyp" },
        { kNuMuCC0kshrt0hyp, "#nu_{#mu} CC0kshrt0hyp" },
        { kNuMuCCOther, "Other #nu_{#mu} CC" },
        { kNC, "NC" },
        { kOOFV, "Out FV" },
        { kOther, "Other" }
    };

    std::map<EventCategory, int> event_category_to_colour_map_ = {
        { kUnknown, kGray },
        { kSignalCCQE, kGreen },
        { kSignalCCMEC, kGreen + 1 },
        { kSignalCCRES, kGreen + 2 },
        { kSignalOther, kGreen + 3 },
        { kNuMuCCNhyp, kAzure - 2 },
        { kNuMuCC0kshrt0hyp, kAzure - 1 },
        { kNuMuCCOther, kAzure },
        { kNC, kOrange },
        { kOOFV, kRed + 3 },
        { kOther, kRed + 1 }
    };
};
