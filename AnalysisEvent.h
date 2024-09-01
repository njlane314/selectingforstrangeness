#ifndef ANALYSISEVENT_H
#define ANALYSISEVENT_H

#include <vector>
#include "TVector3.h"
#include "EventCategory.h"

struct AnalysisEvent
{
    // **** Reconstruction variables ****
    float topological_score;

    int nu_pdg_;
    int nslices_;
    
    float nu_vx_;
    float nu_vy_;
    float nu_vz_;

    int num_pf_particles_;
    int num_tracks_;
    int num_showers_;

    std::vector<unsigned int> pfp_generation_;
    std::vector<unsigned int> pfp_trk_daughters_count_;
    std::vector<unsigned int> pfp_shr_daughters_count_;

    std::vector<float> pfp_track_score_;
    std::vector<int> pfp_reco_pdg_;

    std::vector<int> pfp_hits_;

    std::vector<int> pfp_hitsU_;
    std::vector<int> pfp_hitsV_;
    std::vector<int> pfp_hitsY_;

    std::vector<int> pfp_true_pdg_;

    std::vector<float> pfp_true_E_;
    std::vector<float> pfp_true_px_;
    std::vector<float> pfp_true_py_;
    std::vector<float> pfp_true_pz_;

    std::vector<unsigned long> shower_pfp_id_;
    std::vector<float> shower_startx_;
    std::vector<float> shower_starty_;
    std::vector<float> shower_startz_;
    std::vector<float> shower_start_distance_;

    std::vector<unsigned long> track_pfp_id_;
    std::vector<float> track_length_;
    std::vector<float> track_startx_;
    std::vector<float> track_starty_;
    std::vector<float> track_startz_;
    std::vector<float> track_start_distance_;
    std::vector<float> track_endx_;
    std::vector<float> track_endy_;
    std::vector<float> track_endz_;
    std::vector<float> track_dirx_;
    std::vector<float> track_diry_;
    std::vector<float> track_dirz_;

    // **** Simulation variables ****
    int mc_nu_pdg_;

    float mc_nu_vx_;
    float mc_nu_vy_;
    float mc_nu_vz_;

    float mc_nu_energy_;
    float mc_nu_ccnc_;

    int mc_nu_interaction_type_;

    std::vector<int> mc_nu_daughter_pdg_;
    std::vector<float> mc_nu_daughter_energy_;
    std::vector<float> mc_nu_daughter_px_;
    std::vector<float> mc_nu_daughter_py_;
    std::vector<float> mc_nu_daughter_pz_;

    EventCategory category;
};

#endif // ANALYSISEVENT_H