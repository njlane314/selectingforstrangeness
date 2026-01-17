#ifndef SAMPLE_CATALOG_H
#define SAMPLE_CATALOG_H

#include "ana/DataModel.h"
#include "ana/FeatureDeriver.h"

#include <ROOT/RDataFrame.hxx>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace strangeness {

class SampleCatalog {
public:
  explicit SampleCatalog(std::shared_ptr<FeatureDeriver> feature_deriver = std::make_shared<FeatureDeriver>());

  explicit SampleCatalog(const std::string& /*path*/,
               std::shared_ptr<FeatureDeriver> feature_deriver = std::make_shared<FeatureDeriver>());

  SampleView sample(const SampleRecord& rec) const;
  ROOT::RDF::RNode apply_slice(ROOT::RDF::RNode node, const SampleRecord& rec) const;

  std::vector<const SampleRecord*> simulation_entries(
      const std::string& beamline,
      const std::vector<std::string>& periods) const;

  std::vector<const SampleRecord*> data_entries(
      const std::string& beamline,
      const std::vector<std::string>& periods) const;

  const FeatureDeriver& feature_deriver() const { return *feature_deriver_; }

private:
  void load_from_config();

  std::shared_ptr<FeatureDeriver> feature_deriver_;
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<SampleRecord>>> samples_by_beamline_period_;
};

}

#endif  // SAMPLE_CATALOG_H
