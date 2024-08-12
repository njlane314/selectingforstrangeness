#ifndef EVENTASSEMBLER_H
#define EVENTASSEMBLER_H

#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"

#include "AnalysisEvent.h"
#include "EventCategory.h"
#include "FiducialVolume.h"
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
        if(!file_ || !file_->IsOpen())
        {
            throw std::runtime_error("Could not open file " + input_name);
        }

        tree_ = dynamic_cast<TTree*>(file_->Get(""));

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

    AnalysisEvent get_event(int i)
    {
        tree_->GetEntry(i);

        AnalysisEvent e;
        e.mc_nu_pdg = mc_nu_pdg_;
        e.mc_nu_vx = mc_nu_vtx_x_;
        e.mc_nu_vy = mc_nu_vtx_y_;
        e.mc_nu_vz = mc_nu_vtx_z_;

        e.mc_nu_energy = mc_nu_energy_;
        e.mc_nu_ccnc = mc_nu_ccnc_;

        e.mc_nu_interaction_type = mc_nu_interaction_type_;

        e.category = categorise_event();

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

    int mc_nu_pdg_;
    
    float mc_nu_vtx_x_;
    float mc_nu_vtx_y_;
    float mc_nu_vtx_z_;

    float mc_nu_energy_;
    int mc_nu_ccnc_;

    int mc_nu_interaction_type_;

    EventCategory event_category_;

    void set_branch_addresses()
    {
        tree_->SetBranchAddress("mc_nu_pdg", &mc_nu_pdg_);

        tree_->SetBranchAddress("mc_nu_vtx_x", &mc_nu_vtx_x_);
        tree_->SetBranchAddress("mc_nu_vtx_y", &mc_nu_vtx_y_);
        tree_->SetBranchAddress("mc_nu_vtx_z", &mc_nu_vtx_z_);

        tree_->SetBranchAddress("mc_nu_energy", &mc_nu_energy_);
        tree_->SetBranchAddress("mc_nu_ccnc", &mc_nu_ccnc_);

        tree_->SetBranchAddress("interaction", &mc_nu_interaction_type_);
    }

    EventCategory categorise_event() const
    {
        int abs_mc_nu_pdg = std::abs(mc_nu_pdg_);
        bool is_mc = (abs_mc_nu_pdg == ELECTRON_NEUTRINO || abs_mc_nu_pdg == MUON_NEUTRINO || abs_mc_nu_pdg == TAU_NEUTRINO);
        if (!is_mc) 
            return kUnknown;

        bool mc_vertex_in_FV = point_inside_FV(mc_nu_vtx_x_, mc_nu_vtx_y_, mc_nu_vtx_z_);
        bool mc_neutrino_is_numu = (mc_nu_pdg_ == MUON_NEUTRINO);

        if (!mc_vertex_in_FV) 
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

        bool mc_is_signal = mc_vertex_in_FV && mc_neutrino_is_numu;
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