#include "ana/Config.h"

#include <utility>

namespace strangeness {

Config::Config()
{
    {
        std::vector<Sample> run1;

        {
            Sample s;
            s.source = Source::Data;
            s.slice = Slice::None;
            s.files = {"/path/to/data_run1.root"};
            run1.push_back(std::move(s));
        }

        {
            Sample s;
            s.source = Source::Ext;
            s.slice = Slice::None;
            s.files = {"/path/to/ext_run1.root"};
            s.trig_nom = 1.0;
            s.trig_eqv = 1.0;
            run1.push_back(std::move(s));
        }

        {
            Sample s;
            s.source = Source::MC;
            s.slice = Slice::None;
            s.files = {"/path/to/mc_run1.root"};
            s.pot_nom = 1.0;
            s.pot_eqv = 1.0;
            s.detvars.emplace("SCE", std::vector<std::string>{"/path/to/mc_run1_sce.root"});
            run1.push_back(std::move(s));
        }

        {
            Sample s;
            s.source = Source::MC;
            s.slice = Slice::BeamInclusive;
            s.files = {"/path/to/mc_run1_beamonly.root"};
            s.pot_nom = 1.0;
            s.pot_eqv = 1.0;
            run1.push_back(std::move(s));
        }

        {
            Sample s;
            s.source = Source::MC;
            s.slice = Slice::StrangenessInclusive;
            s.files = {"/path/to/mc_run1_strangeonly.root"};
            s.pot_nom = 1.0;
            s.pot_eqv = 1.0;
            run1.push_back(std::move(s));
        }

        {
            Sample s;
            s.source = Source::MC;
            s.slice = Slice::None;
            s.is_dirt = true;
            s.files = {"/path/to/dirt_run1.root"};
            s.pot_nom = 1.0;
            s.pot_eqv = 1.0;
            run1.push_back(std::move(s));
        }

        beamlines_["BNB"]["Run1"] = std::move(run1);
    }
}

const Config& Config::instance()
{
    static const Config cfg{};
    return cfg;
}

}
