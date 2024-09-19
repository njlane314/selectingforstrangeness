#pragma once

#include <vector>
#include "TVector3.h"

#include "TreeUtilities.h"
#include "Constants.h"

enum EventCategory
{
    kUnknown = 0,
    kSignalCCQE = 1,
    kSignalCCMEC = 2,
    kSignalCCRES = 3,
    kSignalOther = 4,
    kNuMuCCNhyp = 5,
    kNuMuCC0kshrt0hyp = 6,
    kNuMuCCOther = 7,
    kNC = 8,
    kOther = 9,
};

struct AnalysisEvent
{
    int event, run, subrun;

    // **** Simulation variables ****
    int mc_nu_pdg;

    float mc_nu_vtx_x;
    float mc_nu_vtx_y;
    float mc_nu_vtx_z;

    float mc_nu_energy;
    int mc_nu_ccnc;
    int mc_nu_interaction_type;

    float mc_nu_W;
    float mc_nu_X;
    float mc_nu_Y;
    float mc_nu_QSqr;

    tree_utils::ManagedPointer<std::vector<int>> mc_nu_daughter_pdg;
    tree_utils::ManagedPointer<std::vector<float>> mc_nu_daughter_energy;
    tree_utils::ManagedPointer<std::vector<float>> mc_nu_daughter_px;
    tree_utils::ManagedPointer<std::vector<float>> mc_nu_daughter_py;
    tree_utils::ManagedPointer<std::vector<float>> mc_nu_daughter_pz;

    bool mc_has_muon;

    int mc_muon_tid;
    int mc_muon_pdg;
    float mc_muon_energy;
    float mc_muon_px, mc_muon_py, mc_muon_pz;
    float mc_muon_startx, mc_muon_starty, mc_muon_startz;
    float mc_muon_endx, mc_muon_endy, mc_muon_endz;

    float mc_kshrt_total_energy;
    float mc_kshrt_endx, mc_kshrt_endy, mc_kshrt_endz;
    float mc_kshrt_end_sep;

    unsigned int mc_kshrt_piplus_tid; 
    unsigned int mc_kshrt_piminus_tid;

    float mc_kshrt_piplus_energy;
    float mc_kshrt_piplus_px, mc_kshrt_piplus_py, mc_kshrt_piplus_pz;

    float mc_kshrt_piminus_energy;
    float mc_kshrt_piminus_px, mc_kshrt_piminus_py, mc_kshrt_piminus_pz;

    float mc_kshrt_piplus_startx, mc_kshrt_piplus_starty, mc_kshrt_piplus_startz;
    float mc_kshrt_piplus_endx, mc_kshrt_piplus_endy, mc_kshrt_piplus_endz;

    float mc_kshrt_piminus_startx, mc_kshrt_piminus_starty, mc_kshrt_piminus_startz;
    float mc_kshrt_piminus_endx, mc_kshrt_piminus_endy, mc_kshrt_piminus_endz;

    unsigned int mc_kshrt_piplus_n_elas;
    unsigned int mc_kshrt_piplus_n_inelas;
    unsigned int mc_kshrt_piminus_n_elas;
    unsigned int mc_kshrt_piminus_n_inelas;

    tree_utils::ManagedPointer<std::string> mc_kshrt_piplus_endprocess;
    tree_utils::ManagedPointer<std::string> mc_kshrt_piminus_endprocess;

    bool mc_is_kshort_decay_pionic;

    bool mc_has_lambda;
    bool mc_has_sigma_minus;
    bool mc_has_sigma_plus;
    bool mc_has_sigma_zero;

    mutable EventCategory category;

    // **** Reconstruction variables ****
    float topological_score;
    
    float nu_vtx_x;
    float nu_vtx_y;
    float nu_vtx_z;

    int n_pf_particles;
    int n_trks;
    int n_shwrs;

    tree_utils::ManagedPointer<std::vector<int>> backtracked_tid;
    tree_utils::ManagedPointer<std::vector<int>> backtracked_pdg;
    tree_utils::ManagedPointer<std::vector<float>> backtracked_purity;
    tree_utils::ManagedPointer<std::vector<float>> backtracked_completeness;
    tree_utils::ManagedPointer<std::vector<float>> backtracked_overlay_purity;

    tree_utils::ManagedPointer<std::vector<int>> pfnhits;

    tree_utils::ManagedPointer<std::vector<float>> pfp_muon_purity;
    tree_utils::ManagedPointer<std::vector<float>> pfp_muon_completeness;
    tree_utils::ManagedPointer<std::vector<float>> pfp_piplus_purity;
    tree_utils::ManagedPointer<std::vector<float>> pfp_piplus_completeness;
    tree_utils::ManagedPointer<std::vector<float>> pfp_piminus_purity;
    tree_utils::ManagedPointer<std::vector<float>> pfp_piminus_completeness;

    tree_utils::ManagedPointer<std::vector<int>> bt_pdg; 
    tree_utils::ManagedPointer<std::vector<std::vector<unsigned int>>> bt_tids;
    tree_utils::ManagedPointer<std::vector<float>> bt_energy;
};

static const std::map<EventCategory, std::string> event_category_to_label_map = {
    { kUnknown, "Unknown" },
    { kSignalCCQE, "Signal (CCQE)" },
    { kSignalCCMEC, "Signal (CCMEC)" },
    { kSignalCCRES, "Signal (CCRES)" },
    { kSignalOther, "Signal (Other)" },
    { kNuMuCCNhyp, "#nu_{#mu} CCNhyp" },
    { kNuMuCC0kshrt0hyp, "#nu_{#mu} CC0kshrt0hyp" },
    { kNuMuCCOther, "Other #nu_{#mu} CC" },
    { kNC, "NC" },
    { kOther, "Other" }
};

static const std::map<EventCategory, int> event_category_to_colour_map = {
    { kUnknown, kGray },
    { kSignalCCQE, kGreen },
    { kSignalCCMEC, kGreen + 1 },
    { kSignalCCRES, kGreen + 2 },
    { kSignalOther, kGreen + 3 },
    { kNuMuCCNhyp, kAzure - 2 },
    { kNuMuCC0kshrt0hyp, kAzure - 1 },
    { kNuMuCCOther, kAzure },
    { kNC, kOrange },
    { kOther, kRed + 3 }
};

inline std::string get_event_category_label(EventCategory ec) 
{
    auto it = event_category_to_label_map.find(ec);
    if (it != event_category_to_label_map.end()) {
        return it->second;
    }
    return "Unknown";
}

inline int get_event_category_colour(EventCategory ec) 
{
    auto it = event_category_to_colour_map.find(ec);
    if (it != event_category_to_colour_map.end()) {
        return it->second;
    }
    return kGray;
}

inline EventCategory categorise_event(const AnalysisEvent& e) 
{
    int abs_mc_nu_pdg = std::abs(e.mc_nu_pdg);
    bool is_mc = (abs_mc_nu_pdg == ELECTRON_NEUTRINO || abs_mc_nu_pdg == MUON_NEUTRINO || abs_mc_nu_pdg == TAU_NEUTRINO);
    if (!is_mc) 
        return kUnknown;

    bool mc_neutrino_is_numu = (e.mc_nu_pdg == MUON_NEUTRINO);

    if (e.mc_nu_ccnc == NEUTRAL_CURRENT) 
        return kNC;
    else if (!mc_neutrino_is_numu) 
        return kOther;

    bool mc_is_signal = mc_neutrino_is_numu;
    if (mc_is_signal) 
    {
        if (e.mc_nu_interaction_type == 0) 
            return kSignalCCQE;
        else if (e.mc_nu_interaction_type == 10) 
            return kSignalCCMEC;
        else if (e.mc_nu_interaction_type == 1) 
            return kSignalCCRES;
        else 
            return kSignalOther;
    }
    else 
        return kOther;
}