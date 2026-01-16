#include "ana/rarexsec/proc/Selection.h"

namespace rarexsec::fiducial {

bool is_in_truth_volume(float, float, float)
{
    return true;
}

bool is_in_reco_volume(float, float, float)
{
    return true;
}

}  // namespace rarexsec::fiducial
