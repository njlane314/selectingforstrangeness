#include "ana/SampleCatalog.h"

#include "ana/Config.h"

#include <stdexcept>
#include <utility>

namespace strangeness {

SampleCatalog::SampleCatalog(std::shared_ptr<FeatureDeriver> feature_deriver)
    : feature_deriver_(std::move(feature_deriver))
{
    load_from_config();
}

SampleCatalog::SampleCatalog(const std::string& /*path*/, std::shared_ptr<FeatureDeriver> feature_deriver)
    : feature_deriver_(std::move(feature_deriver))
{
    load_from_config();
}

void SampleCatalog::load_from_config()
{
    const auto& cfg = Config::instance().beamlines();

    for (const auto& [beamline, periods] : cfg) {
        for (const auto& [period, samples] : periods) {
            auto& bucket = samples_by_beamline_period_[beamline][period];
            bucket.reserve(samples.size());

            for (const auto& s : samples) {
                SampleRecord rec;
                rec.beamline = beamline;
                rec.period = period;

                rec.source = s.source;
                rec.slice = s.slice;

                rec.kind = s.is_dirt ? sample::origin::dirt
                                     : sample::from_source_slice(rec.source, rec.slice);

                rec.files = s.files;
                if (rec.files.empty()) {
                    throw std::runtime_error("empty 'files' for sample in " + beamline + "/" + period);
                }
                rec.file = rec.files.front();

                if (rec.source == Source::Ext) {
                    rec.trig_nom = s.trig_nom;
                    rec.trig_eqv = s.trig_eqv;
                } else if (rec.source == Source::MC) {
                    rec.pot_nom = s.pot_nom;
                    rec.pot_eqv = s.pot_eqv;
                }

                rec.nominal = sample(rec);

                for (const auto& [tag, dv_files] : s.detvars) {
                    if (dv_files.empty())
                        continue;

                    SampleRecord dv = rec;
                    dv.files = dv_files;
                    dv.file = dv.files.front();

                    dv.nominal = SampleView{};
                    dv.detvars.clear();

                    rec.detvars.emplace(tag, sample(dv));
                }

                bucket.push_back(std::move(rec));
            }
        }
    }
}

SampleView SampleCatalog::sample(const SampleRecord& rec) const
{
    static const std::string tree = "nuselection/EventSelectionFilter";

    auto df_ptr = std::make_shared<ROOT::RDataFrame>(tree, rec.files);
    ROOT::RDF::RNode node = *df_ptr;

    node = feature_deriver().run(node, rec);
    node = apply_slice(node, rec);

    return SampleView{df_ptr, std::move(node)};
}

ROOT::RDF::RNode SampleCatalog::apply_slice(ROOT::RDF::RNode node, const SampleRecord& rec) const
{
    using strangeness::Slice;
    using strangeness::Source;

    if (rec.source == Source::MC) {
        if (rec.slice == Slice::StrangenessInclusive)
            return node.Filter([](bool s) { return s; }, {"is_strange"});
        if (rec.slice == Slice::BeamInclusive)
            return node.Filter([](bool s) { return !s; }, {"is_strange"});
        return node;
    }

    if (rec.slice != Slice::None) {
        throw std::runtime_error(
            "Slice requested for non-MC sample at " + rec.beamline + "/" + rec.period);
    }
    return node;
}

std::vector<const SampleRecord*>
SampleCatalog::simulation_entries(const std::string& beamline,
                        const std::vector<std::string>& periods) const
{
    std::vector<const SampleRecord*> out;

    auto it_bl = samples_by_beamline_period_.find(beamline);
    if (it_bl == samples_by_beamline_period_.end())
        return out;

    for (const auto& per : periods) {
        auto it_p = it_bl->second.find(per);
        if (it_p == it_bl->second.end())
            continue;

        for (const auto& rec : it_p->second) {
            if (rec.source != Source::Data)
                out.push_back(&rec);
        }
    }
    return out;
}

std::vector<const SampleRecord*>
SampleCatalog::data_entries(const std::string& beamline,
                  const std::vector<std::string>& periods) const
{
    std::vector<const SampleRecord*> out;

    auto it_bl = samples_by_beamline_period_.find(beamline);
    if (it_bl == samples_by_beamline_period_.end())
        return out;

    for (const auto& per : periods) {
        auto it_p = it_bl->second.find(per);
        if (it_p == it_bl->second.end())
            continue;

        for (const auto& rec : it_p->second) {
            if (rec.source == Source::Data)
                out.push_back(&rec);
        }
    }
    return out;
}

}
