#ifndef SELECTION_H
#define SELECTION_H

#include "ana/DataModel.h"

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>
#include <cstddef>
#include <vector>

namespace strangeness {
namespace selection {

inline constexpr const char* nominal_weight_column = "w_nominal";
inline constexpr const char* analysis_channel_column = "analysis_channels";

inline constexpr float optical_filter_beam_pe_min = 0.f;
inline constexpr float optical_filter_veto_pe_max = 20.f;

inline constexpr int required_slice_count = 1;
inline constexpr float slice_topological_score_min = 0.06f;

inline constexpr float contained_fraction_min = 0.0f;
inline constexpr float slice_cluster_fraction_min = 0.5f;

inline constexpr float muon_track_score_min = 0.5f;
inline constexpr float muon_track_length_min = 10.0f;
inline constexpr float muon_track_vertex_distance_max = 4.0f;
inline constexpr unsigned muon_pfp_generation_required = 2u;

enum class SelectionPreset {
    None,
    OpticalTriggerGate,
    SliceQuality,
    RecoFiducial,
    TopologyQuality,
    MuonCandidate,
    InclusiveMuonCC
};

inline ROOT::RDF::RNode apply_preset(ROOT::RDF::RNode node,
                                    SelectionPreset preset,
                                    const SampleRecord& record)
{
    switch (preset) {
    case SelectionPreset::None:
        return node;
    case SelectionPreset::OpticalTriggerGate:
        return node.Filter(
            [source = record.source](float beam_pe, float veto_pe, int software_trigger) {
                if (source != Source::MC) {
                    return true;
                }
                return (beam_pe > optical_filter_beam_pe_min) &&
                       (veto_pe < optical_filter_veto_pe_max) &&
                       (software_trigger > 0);
            },
            {"optical_filter_pe_beam", "optical_filter_pe_veto", "software_trigger"});

    case SelectionPreset::SliceQuality:
        return node.Filter(
            [](int slice_count, float topological_score) {
                return slice_count == required_slice_count &&
                       topological_score > slice_topological_score_min;
            },
            {"num_slices", "topological_score"});

    case SelectionPreset::RecoFiducial:
        return node.Filter([](bool in_reco_fiducial) { return in_reco_fiducial; },
                           {"in_reco_fiducial"});

    case SelectionPreset::TopologyQuality:
        return node.Filter(
            [](float contained_fraction, float slice_cluster_fraction) {
                return contained_fraction >= contained_fraction_min &&
                       slice_cluster_fraction >= slice_cluster_fraction_min;
            },
            {"contained_fraction", "slice_cluster_fraction"});

    case SelectionPreset::MuonCandidate:
        return node.Filter(
            [](const ROOT::RVec<float>& track_shower_scores,
               const ROOT::RVec<float>& track_lengths,
               const ROOT::RVec<float>& track_vertex_distances,
               const ROOT::RVec<unsigned>& pfp_generations) {
                const auto track_count = track_shower_scores.size();
                for (std::size_t i = 0; i < track_count; ++i) {
                    const bool is_muon_candidate =
                        (track_shower_scores[i] > muon_track_score_min) &&
                        (track_lengths[i] > muon_track_length_min) &&
                        (track_vertex_distances[i] < muon_track_vertex_distance_max) &&
                        (pfp_generations[i] == muon_pfp_generation_required);
                    if (is_muon_candidate) {
                        return true;
                    }
                }
                return false;
            },
            {"track_shower_scores",
             "track_length",
             "track_distance_to_vertex",
             "pfp_generations"});

    case SelectionPreset::InclusiveMuonCC:
    default: {
        auto filtered = apply_preset(node, SelectionPreset::OpticalTriggerGate, record);
        filtered = apply_preset(filtered, SelectionPreset::SliceQuality, record);
        filtered = apply_preset(filtered, SelectionPreset::RecoFiducial, record);
        filtered = apply_preset(filtered, SelectionPreset::TopologyQuality, record);
        return apply_preset(filtered, SelectionPreset::MuonCandidate, record);
    }
    }
}

struct SelectionMetrics {
    double truth_signal_weight = 0.0;

    double selected_weight = 0.0;

    double selected_truth_signal_weight = 0.0;

    double efficiency() const {
        return truth_signal_weight > 0.0 ? (selected_truth_signal_weight / truth_signal_weight) : 0.0;
    }
    double purity() const {
        return selected_weight > 0.0 ? (selected_truth_signal_weight / selected_weight) : 0.0;
    }
};

template <class SignalPredicate>
inline SelectionMetrics evaluate_selection_metrics(const std::vector<const SampleRecord*>& mc_records,
                                                  const SignalPredicate& is_truth_signal_channel,
                                                  SelectionPreset selection_preset)
{
    auto sum_nominal_weight = [](ROOT::RDF::RNode node_in) {
        auto sum = node_in.Sum<float>(nominal_weight_column);
        return static_cast<double>(sum.GetValue());
    };

    SelectionMetrics totals;

    for (const SampleRecord* record : mc_records) {
        ROOT::RDF::RNode unselected = record->nominal.rnode();

        auto truth_signal = unselected.Filter(
            [&](int channel) { return is_truth_signal_channel(channel); },
            {analysis_channel_column});
        totals.truth_signal_weight += sum_nominal_weight(truth_signal);

        auto selected = apply_preset(unselected, selection_preset, *record);
        totals.selected_weight += sum_nominal_weight(selected);

        auto selected_truth_signal = selected.Filter(
            [&](int channel) { return is_truth_signal_channel(channel); },
            {analysis_channel_column});
        totals.selected_truth_signal_weight += sum_nominal_weight(selected_truth_signal);
    }
    return totals;
}

}  // namespace selection
}  // namespace strangeness

#endif  // SELECTION_H
