#ifndef ANALYSISEVENT_H
#define ANALYSISEVENT_H

#include <vector>

#include "TVector3.h"

#include "EventCategory.h"

struct AnalysisEvent
{
    // **** Truth variables ****

    int mc_nu_pdg;

    float mc_nu_vx;
    float mc_nu_vy;
    float mc_nu_vz;

    float mc_nu_energy;

    int mc_nu_ccnc;
    int mc_nu_interaction_type;

    bool is_mc;
    bool mc_neutrino_is_numu;
    bool mc_vertex_in_FV;
    bool mc_muon_in_mom_range;

    bool mc_no_fs_hyperons;
    bool mc_is_signal; 

    EventCategory category;

    // **** Reconstructed variables **** 

    float topological_score;

    float nu_vx;
    float nu_vy;
    float nu_vz;

    int num_pf_particles;
    int num_tracks;
    int num_showers;

    // **** Recostruction selection  ****

    bool sel_nu_mu_cc;
    bool sel_reco_vertex_in_FV;
    bool sel_topo_cut_passed;

    bool sel_has_muon_candidate;
    bool sel_muon_contained;
    bool sel_muon_quality_ok;
    bool sel_muon_passed_mom_cuts;

    int muon_candidate_idx;
    int piplus_candidate_idx;
    int piminus_candidate_idx;
};

#endif // ANALYSISEVENT_H