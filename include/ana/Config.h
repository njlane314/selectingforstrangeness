#ifndef CONFIG_H
#define CONFIG_H

#include "ana/DataModel.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace strangeness {

class Config {
public:
  struct Sample {
    Source source = Source::MC;
    Slice slice = Slice::None;
    bool is_dirt = false;
    std::vector<std::string> files;
    double pot_nom = 0.0;
    double pot_eqv = 0.0;
    double trig_nom = 0.0;
    double trig_eqv = 0.0;
    std::unordered_map<std::string, std::vector<std::string>> detvars;
  };

  using PeriodSamples = std::unordered_map<std::string, std::vector<Sample>>;
  using BeamlinePeriods = std::unordered_map<std::string, PeriodSamples>;

  static const Config& instance();

  const BeamlinePeriods& beamlines() const { return beamlines_; }

private:
  Config();

  BeamlinePeriods beamlines_;
};

}

#endif  // CONFIG_H
