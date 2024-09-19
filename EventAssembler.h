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
        tree_ = dynamic_cast<TTree*>(file_->Get("emptyselectionfilter/StrangenessSelectionFilter"));

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

    void print_event(int i) const
    {
        tree_->GetEntry(i);

        std::cout << "Run: " << e_.run << ", Subrun: " << e_.subrun << ", Event: " << e_.event << std::endl;
    }

private:

    TFile* file_;
    TTree* tree_;

    int num_events_;

    AnalysisEvent e_;  

    void set_branch_addresses()
    {
        tree_->SetBranchAddress("evt", &e_.event);
        tree_->SetBranchAddress("run", &e_.run);
        tree_->SetBranchAddress("sub", &e_.subrun);

        tree_->SetBranchAddress("nu_pdg", &e_.mc_nu_pdg);
        tree_->SetBranchAddress("true_nu_vtx_x", &e_.mc_nu_vtx_x);
        tree_->SetBranchAddress("true_nu_vtx_y", &e_.mc_nu_vtx_y);
        tree_->SetBranchAddress("true_nu_vtx_z", &e_.mc_nu_vtx_z);
        tree_->SetBranchAddress("nu_e", &e_.mc_nu_energy);
        tree_->SetBranchAddress("ccnc", &e_.mc_nu_ccnc);
        tree_->SetBranchAddress("interaction", &e_.mc_nu_interaction_type);

        tree_->SetBranchAddress("W", &e_.mc_nu_W);
        tree_->SetBranchAddress("X", &e_.mc_nu_X);
        tree_->SetBranchAddress("Y", &e_.mc_nu_Y);
        tree_->SetBranchAddress("QSqr", &e_.mc_nu_QSqr);

        set_object_input_branch_address(*tree_, "mc_pdg", e_.mc_nu_daughter_pdg);
        set_object_input_branch_address(*tree_, "mc_E", e_.mc_nu_daughter_energy);
        set_object_input_branch_address(*tree_, "mc_px", e_.mc_nu_daughter_px);
        set_object_input_branch_address(*tree_, "mc_py", e_.mc_nu_daughter_py);
        set_object_input_branch_address(*tree_, "mc_pz", e_.mc_nu_daughter_pz);

        tree_->SetBranchAddress("mc_has_muon", &e_.mc_has_muon);
        tree_->SetBranchAddress("mc_is_kshort_decay_pionic", &e_.mc_is_kshort_decay_pionic);
        tree_->SetBranchAddress("mc_has_lambda", &e_.mc_has_lambda);
        tree_->SetBranchAddress("mc_has_sigma_plus", &e_.mc_has_sigma_plus);
        tree_->SetBranchAddress("mc_has_sigma_minus", &e_.mc_has_sigma_minus);
        tree_->SetBranchAddress("mc_has_sigma_zero", &e_.mc_has_sigma_zero);

        tree_->SetBranchAddress("mc_muon_tid", &e_.mc_muon_tid); 
        tree_->SetBranchAddress("mc_muon_pdg", &e_.mc_muon_pdg);
        tree_->SetBranchAddress("mc_muon_energy", &e_.mc_muon_energy);
        tree_->SetBranchAddress("mc_muon_px", &e_.mc_muon_px);
        tree_->SetBranchAddress("mc_muon_py", &e_.mc_muon_py);
        tree_->SetBranchAddress("mc_muon_pz", &e_.mc_muon_pz);
        tree_->SetBranchAddress("mc_muon_startx", &e_.mc_muon_startx);
        tree_->SetBranchAddress("mc_muon_starty", &e_.mc_muon_starty);
        tree_->SetBranchAddress("mc_muon_startz", &e_.mc_muon_startz);
        tree_->SetBranchAddress("mc_muon_endx", &e_.mc_muon_endx);
        tree_->SetBranchAddress("mc_muon_endy", &e_.mc_muon_endy);
        tree_->SetBranchAddress("mc_muon_endz", &e_.mc_muon_endz);

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
        tree_->SetBranchAddress("mc_kshrt_piplus_startx", &e_.mc_kshrt_piplus_startx);
        tree_->SetBranchAddress("mc_kshrt_piplus_starty", &e_.mc_kshrt_piplus_starty);
        tree_->SetBranchAddress("mc_kshrt_piplus_startz", &e_.mc_kshrt_piplus_startz);
        tree_->SetBranchAddress("mc_kshrt_piplus_endx", &e_.mc_kshrt_piplus_endx);
        tree_->SetBranchAddress("mc_kshrt_piplus_endy", &e_.mc_kshrt_piplus_endy);
        tree_->SetBranchAddress("mc_kshrt_piplus_endz", &e_.mc_kshrt_piplus_endz);

        tree_->SetBranchAddress("mc_kshrt_piminus_energy", &e_.mc_kshrt_piminus_energy);
        tree_->SetBranchAddress("mc_kshrt_piminus_px", &e_.mc_kshrt_piminus_px);
        tree_->SetBranchAddress("mc_kshrt_piminus_py", &e_.mc_kshrt_piminus_py);
        tree_->SetBranchAddress("mc_kshrt_piminus_pz", &e_.mc_kshrt_piminus_pz);
        tree_->SetBranchAddress("mc_kshrt_piminus_startx", &e_.mc_kshrt_piminus_startx);
        tree_->SetBranchAddress("mc_kshrt_piminus_starty", &e_.mc_kshrt_piminus_starty);
        tree_->SetBranchAddress("mc_kshrt_piminus_startz", &e_.mc_kshrt_piminus_startz);
        tree_->SetBranchAddress("mc_kshrt_piminus_endx", &e_.mc_kshrt_piminus_endx);
        tree_->SetBranchAddress("mc_kshrt_piminus_endy", &e_.mc_kshrt_piminus_endy);
        tree_->SetBranchAddress("mc_kshrt_piminus_endz", &e_.mc_kshrt_piminus_endz);

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

        set_object_input_branch_address(*tree_, "backtracked_tid", e_.backtracked_tid);
        set_object_input_branch_address(*tree_, "backtracked_pdg", e_.backtracked_pdg);
        set_object_input_branch_address(*tree_, "backtracked_purity", e_.backtracked_purity);
        set_object_input_branch_address(*tree_, "backtracked_completeness", e_.backtracked_completeness); 
        set_object_input_branch_address(*tree_, "backtracked_overlay_purity", e_.backtracked_overlay_purity);

        set_object_input_branch_address(*tree_, "pfnhits", e_.pfnhits);

        set_object_input_branch_address(*tree_, "pfp_muon_purity", e_.pfp_muon_purity);
        set_object_input_branch_address(*tree_, "pfp_muon_completeness", e_.pfp_muon_completeness);
        set_object_input_branch_address(*tree_, "pfp_piplus_purity", e_.pfp_piplus_purity);
        set_object_input_branch_address(*tree_, "pfp_piplus_completeness", e_.pfp_piplus_completeness);
        set_object_input_branch_address(*tree_, "pfp_piminus_purity", e_.pfp_piminus_purity);
        set_object_input_branch_address(*tree_, "pfp_piminus_completeness", e_.pfp_piminus_completeness);

        set_object_input_branch_address(*tree_, "bt_pdg", e_.bt_pdg);
        set_object_input_branch_address(*tree_, "bt_tids", e_.bt_tids);
        set_object_input_branch_address(*tree_, "bt_energy", e_.bt_energy);
    }
};

#endif // EVENTASSEMBLER_H