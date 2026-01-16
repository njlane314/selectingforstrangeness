#pragma once

#include "ana/DataModel.h"
#include "ana/Processor.h"

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RDF/RNode.hxx>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace strangeness {

class Hub {
public:
  explicit Hub(const std::string& path,
               std::shared_ptr<Processor> processor = std::make_shared<Processor>());

  Frame sample(const Entry& rec) const;
  ROOT::RDF::RNode apply_slice(ROOT::RDF::RNode node, const Entry& rec);

  std::vector<const Entry*> simulation_entries(
      const std::string& beamline,
      const std::vector<std::string>& periods) const;
  std::vector<const Entry*> data_entries(
      const std::string& beamline,
      const std::vector<std::string>& periods) const;

  const Processor& processor() const { return *processor_; }

private:
  std::shared_ptr<Processor> processor_;
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<Entry>>> db_;
};

}  // namespace strangeness
