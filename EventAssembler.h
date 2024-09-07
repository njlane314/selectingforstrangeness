#ifndef EVENTASSEMBLER_H
#define EVENTASSEMBLER_H

#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>

#include "TFile.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"

#include "TreeUtilities.h"
#include "Constants.h"
#include "AnalysisEvent.h"

class EventAssembler
{
public:

    EventAssembler( const EventAssembler& ) = delete;
    EventAssembler( EventAssembler&& ) = delete;
    EventAssembler& operator=( const EventAssembler& ) = delete;
    EventAssembler& operator=( EventAssembler&& ) = delete;

    inline static const EventAssembler& instance(const std::string& input_name)
    {
        static std::unique_ptr<EventAssembler> the_instance( new EventAssembler(input_name) );
        return *the_instance;
    }

    EventAssembler(const std::string& input_name)
    {
        file_ = TFile::Open(input_name.c_str(), "READ");
        tree_ = dynamic_cast<TTree*>(file_->Get("nuselection/StrangenessSelectionFilter"));

        num_events_ = tree_->GetEntries();

        set_branch_addresses(); 
    }

    ~EventAssembler()
    {
        if (file_)
        {
            file_->Close();
            delete file_; 
        }
    }

    const AnalysisEvent& get_event(int i) const
    {
        tree_->GetEntry(i);
        e_.category = categorise_event(e_);

        return e_;
    }

    int get_num_events() const
    {
        return num_events_; 
    }

    void print_event(const AnalysisEvent& event) const
    {
    }

private:

    TFile* file_;
    TTree* tree_;

    int num_events_;

    AnalysisEvent e_;  

    void set_branch_addresses()
    {
        tree_->SetBranchAddress("nu_pdg", &e_.mc_nu_pdg);
        tree_->SetBranchAddress("true_nu_vtx_x", &e_.mc_nu_vtx_x);
        tree_->SetBranchAddress("true_nu_vtx_y", &e_.mc_nu_vtx_y);
        tree_->SetBranchAddress("true_nu_vtx_z", &e_.mc_nu_vtx_z);
        tree_->SetBranchAddress("nu_e", &e_.mc_nu_energy);
        tree_->SetBranchAddress("ccnc", &e_.mc_nu_ccnc);
        tree_->SetBranchAddress("interaction", &e_.mc_nu_interaction_type);

        set_object_input_branch_address(*tree_, "mc_pdg", e_.mc_nu_daughter_pdg);
        set_object_input_branch_address(*tree_, "mc_E", e_.mc_nu_daughter_energy);
        set_object_input_branch_address(*tree_, "mc_px", e_.mc_nu_daughter_px);
        set_object_input_branch_address(*tree_, "mc_py", e_.mc_nu_daughter_py);
        set_object_input_branch_address(*tree_, "mc_pz", e_.mc_nu_daughter_pz);

        tree_->SetBranchAddress("mc_is_kshort_decay_pionic", &e_.mc_is_kshort_decay_pionic);
        tree_->SetBranchAddress("mc_has_lambda", &e_.mc_has_lambda);
        tree_->SetBranchAddress("mc_has_sigma_plus", &e_.mc_has_sigma_plus);
        tree_->SetBranchAddress("mc_has_sigma_minus", &e_.mc_has_sigma_minus);
        tree_->SetBranchAddress("mc_has_sigma_zero", &e_.mc_has_sigma_zero);

        tree_->SetBranchAddress("mc_kshrt_total_energy", &e_.mc_kshrt_total_energy);
        tree_->SetBranchAddress("mc_kaon_decay_x", &e_.mc_kshrt_endx);
        tree_->SetBranchAddress("mc_kaon_decay_y", &e_.mc_kshrt_endy);
        tree_->SetBranchAddress("mc_kaon_decay_z", &e_.mc_kshrt_endz);
        tree_->SetBranchAddress("mc_kaon_decay_distance", &e_.mc_kshrt_end_sep);
        tree_->SetBranchAddress("mc_piplus_tid", &e_.mc_kshrt_piplus_tid);
        tree_->SetBranchAddress("mc_piminus_tid", &e_.mc_kshrt_piminus_tid);

        tree_->SetBranchAddress("mc_kshrt_piplus_energy", &e_.mc_kshrt_piplus_energy);
        tree_->SetBranchAddress("mc_kshrt_piplus_px", &e_.mc_kshrt_piplus_px);
        tree_->SetBranchAddress("mc_kshrt_piplus_py", &e_.mc_kshrt_piplus_py);
        tree_->SetBranchAddress("mc_kshrt_piplus_pz", &e_.mc_kshrt_piplus_pz);

        tree_->SetBranchAddress("mc_kshrt_piminus_energy", &e_.mc_kshrt_piminus_energy);
        tree_->SetBranchAddress("mc_kshrt_piminus_px", &e_.mc_kshrt_piminus_px);
        tree_->SetBranchAddress("mc_kshrt_piminus_py", &e_.mc_kshrt_piminus_py);
        tree_->SetBranchAddress("mc_kshrt_piminus_pz", &e_.mc_kshrt_piminus_pz);

        tree_->SetBranchAddress("mc_piplus_n_elas", &e_.mc_kshrt_piplus_n_elas);
        tree_->SetBranchAddress("mc_piminus_n_elas", &e_.mc_kshrt_piminus_n_elas);

        tree_->SetBranchAddress("mc_piplus_n_inelas", &e_.mc_kshrt_piplus_n_inelas);
        tree_->SetBranchAddress("mc_piminus_n_inelas", &e_.mc_kshrt_piminus_n_inelas);

        set_object_input_branch_address(*tree_, "mc_piplus_endprocess", e_.mc_kshrt_piplus_endprocess);
        set_object_input_branch_address(*tree_, "mc_piminus_endprocess", e_.mc_kshrt_piminus_endprocess);

        tree_->SetBranchAddress("topological_score", &e_.topological_score);
        tree_->SetBranchAddress("reco_nu_vtx_sce_x", &e_.nu_vtx_x);
        tree_->SetBranchAddress("reco_nu_vtx_sce_y", &e_.nu_vtx_y);
        tree_->SetBranchAddress("reco_nu_vtx_sce_z", &e_.nu_vtx_z);

        tree_->SetBranchAddress("n_pfps", &e_.n_pf_particles);
        tree_->SetBranchAddress("n_tracks", &e_.n_trks);
        tree_->SetBranchAddress("n_showers", &e_.n_shwrs);

        /*tree_->SetBranchAddress("pfp_generation_v", &e_.pfp_generation);
        tree_->SetBranchAddress("pfp_trk_daughters_v", &e_.pfp_trk_daughters_count);
        tree_->SetBranchAddress("pfp_shr_daughters_v", &e_.pfp_shr_daughters_count);

        tree_->SetBranchAddress("trk_score_v", &e_.pfp_track_score);
        tree_->SetBranchAddress("pfpdg", &e_.pfp_reco_pdg);
        tree_->SetBranchAddress("pfnhits", &e_.pfp_hits);
        tree_->SetBranchAddress("pfnplanehits_U", &e_.pfp_hits_uplane);
        tree_->SetBranchAddress("pfnplanehits_V", &e_.pfp_hits_vplane);
        tree_->SetBranchAddress("pfnplanehits_Y", &e_.pfp_hits_yplane);

        tree_->SetBranchAddress("backtracked_pdg", &e_.pfp_true_pdg);
        tree_->SetBranchAddress("backtracked_e", &e_.pfp_true_energy);
        tree_->SetBranchAddress("backtracked_px", &e_.pfp_true_px);
        tree_->SetBranchAddress("backtracked_py", &e_.pfp_true_py);
        tree_->SetBranchAddress("backtracked_pz", &e_.pfp_true_pz);*/
    }
};

#endif // EVENTASSEMBLER_H