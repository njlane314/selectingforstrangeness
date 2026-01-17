#ifndef FEATURE_DERIVER_H
#define FEATURE_DERIVER_H

#include <ROOT/RDataFrame.hxx>

namespace strangeness {

struct SampleRecord;

class FeatureDeriver {
public:
  virtual ~FeatureDeriver() = default;

  virtual ROOT::RDF::RNode run(ROOT::RDF::RNode node, const SampleRecord&) const;
};

const FeatureDeriver& feature_deriver();

}

#endif  // FEATURE_DERIVER_H
