#pragma once

namespace ROOT::RDF {
class RNode;
}

namespace strangeness {

struct SampleRecord;

class FeatureDeriver {
public:
  virtual ~FeatureDeriver() = default;

  virtual ROOT::RDF::RNode run(ROOT::RDF::RNode node, const SampleRecord&) const;
};

const FeatureDeriver& feature_deriver();

}
