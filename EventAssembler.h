#ifndef EVENTASSEMBLER_H
#define EVENTASSEMBLER_H

#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"

#include "AnalysisEvent.h"
#include "EventCategory.h"
#include "Constants.h"

class EventAssembler
{
public:

    EventAssembler( const EventAssembler& ) = delete;
    EventAssembler( EventAssembler&& ) = delete;
    EventAssembler& operator=( const EventAssembler& ) = delete;
    EventAssembler& operator=( EventAssembler&& ) = delete;

    inline static const EventAssembler& Instance(const std::string& input_name)
    {
        static std::unique_ptr<EventAssembler> the_instance( new EventAssembler(input_name) );

        return *the_instance;
    }

    EventAssembler(const std::string& input_name)
    {
        file_ = TFile::Open(input_name.c_str(), "READ");
        tree_ = dynamic_cast<TTree*>(file_->Get("nuselection/NeutrinoSelectionFilter"));

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

    AnalysisEvent get_event(int i) const
    {
        tree_->GetEntry(i);

        AnalysisEvent e;

        

        return e;
    }

    int get_num_events() const
    {
        return num_events_; 
    }

private:

    TFile* file_;
    TTree* tree_;

    int num_events_;

    int nu_pdg_;
    int ccnc_;
    int nu_parent_pdg_;
    int interaction_;
    float nu_e_;

    float true_nu_vtx_x_;
    float true_nu_vtx_y_;
    float true_nu_vtx_z_;
    
    float reco_nu_vtx_x_;
    float reco_nu_vtx_y_;
    float reco_nu_vtx_z_;

    std::vector<int> backtracked_pdg_;
    std::vector<float> backtracked_e_;
    std::vector<int> backtracked_tid_;

    std::vector<float> backtracked_px_;
    std::vector<float> backtracked_py_;
    std::vector<float> backtracked_pz_;

    void set_branch_addresses()
    {
        // Reconstruction variables
        tree_->SetBranchAddress("topological_score", &topological_score_);

        tree_->SetBranchAddress("nu_pdg", &nu_pdg_);
        tree->SetBranchAddress("ccnc", &ccnc_);
        tree->SetBranchAddress("nu_parent_pdg", &nu_parent_pdg_);
        tree->SetBranchAddress("interaction", &interaction_);
        tree->SetBranchAddress("nu_e", &nu_e_);

        tree_->SetBranchAddress("true_nu_vtx_x", &true_nu_vtx_x_);
        tree_->SetBranchAddress("true_nu_vtx_y", &true_nu_vtx_y_);
        tree_->SetBranchAddress("true_nu_vtx_z", &true_nu_vtx_z_);
        
        tree_->SetBranchAddress("reco_nu_vtx_sce_x", &reco_nu_vtx_x_);
        tree_->SetBranchAddress("reco_nu_vtx_sce_y", &reco_nu_vtx_y_);
        tree_->SetBranchAddress("reco_nu_vtx_sce_z", &reco_nu_vtx_z_);

        tree_->SetBranchAddress("backtracked_pdg", &backtracked_pdg_);
        tree_->SetBranchAddress("backtracked_e", &backtracked_e_);
        tree_->SetBranchAddress("backtracked_tid", &backtracked_tid_);

        tree_->SetBranchAddress("backtracked_px", &backtracked_px_);
        tree_->SetBranchAddress("backtracked_py", &backtracked_py_);
        tree_->SetBranchAddress("backtracked_pz", &backtracked_pz_);
    }

    EventCategory categorise_event() const
    {
        int abs_mc_nu_pdg = std::abs(mc_nu_pdg_);
        bool is_mc = (abs_mc_nu_pdg == ELECTRON_NEUTRINO || abs_mc_nu_pdg == MUON_NEUTRINO || abs_mc_nu_pdg == TAU_NEUTRINO);
        if (!is_mc) 
            return kUnknown;

        bool mc_nu_vtx_inside_fv = point_inside_fv(mc_nu_vtx_x_, mc_nu_vtx_y_, mc_nu_vtx_z_);
        bool mc_neutrino_is_numu = (mc_nu_pdg_ == MUON_NEUTRINO);

        if (!mc_nu_vtx_inside_fv) 
        {
            return kOOFV;
        }
        else if (mc_nu_ccnc_ == NEUTRAL_CURRENT) 
        {
            return kNC;
        }
        else if (!mc_neutrino_is_numu) 
        {
            return kOther;
        }

        bool mc_is_signal = mc_nu_vtx_inside_fv && mc_neutrino_is_numu;
        if (mc_is_signal) 
        {
            if (mc_nu_interaction_type_ == 0) 
                return kSignalCCQE;
            else if (mc_nu_interaction_type_ == 10) 
                return kSignalCCMEC;
            else if (mc_nu_interaction_type_ == 1) 
                return kSignalCCRES;
            else 
                return kSignalOther;
        }
        else 
        {
            return kOther;
        }
    }
};

#endif // EVENTASSEMBLER_H