#include "ana/Config.h"
#include "ana/SampleCatalog.h"

#include <ROOT/RDataFrame.hxx>
#include <TSystem.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void validate_sample_record(const strangeness::SampleRecord& rec, std::ostream& out, bool& ok)
{
    if (rec.files.empty()) {
        out << "[fail] " << rec.beamline << "/" << rec.period
            << " sample has no files listed\n";
        ok = false;
        return;
    }

    for (const auto& file : rec.files) {
        if (gSystem->AccessPathName(file.c_str()) == false)
            continue;
        out << "[warn] missing file on disk: " << file << "\n";
    }

    try {
        auto count = rec.rnode().Count();
        auto entries = count.GetValue();
        out << "[ok] " << rec.beamline << "/" << rec.period
            << " entries=" << entries << " file=" << rec.file << "\n";
    } catch (const std::exception& e) {
        out << "[fail] " << rec.beamline << "/" << rec.period
            << " could not load: " << e.what() << "\n";
        ok = false;
    }

    for (const auto& [tag, detvar] : rec.detvars) {
        try {
            auto count = detvar.rnode().Count();
            auto entries = count.GetValue();
            out << "[ok] detvar=" << tag << " entries=" << entries << "\n";
        } catch (const std::exception& e) {
            out << "[fail] detvar=" << tag << " could not load: " << e.what() << "\n";
            ok = false;
        }
    }
}

std::vector<std::string> collect_periods(const strangeness::Config::PeriodSamples& periods)
{
    std::vector<std::string> out;
    out.reserve(periods.size());
    for (const auto& [period, _] : periods)
        out.push_back(period);
    return out;
}

}

void validate_samples()
{
    using namespace strangeness;

    const auto& cfg = Config::instance().beamlines();
    SampleCatalog catalog;

    bool ok = true;
    std::ostringstream report;

    for (const auto& [beamline, periods] : cfg) {
        report << "[info] beamline=" << beamline << "\n";
        auto period_list = collect_periods(periods);

        auto data = catalog.data_entries(beamline, period_list);
        auto sim = catalog.simulation_entries(beamline, period_list);

        for (const auto* rec : data)
            validate_sample_record(*rec, report, ok);
        for (const auto* rec : sim)
            validate_sample_record(*rec, report, ok);
    }

    std::cout << report.str();

    if (!ok)
        throw std::runtime_error("sample validation failed; see log for details");
}
