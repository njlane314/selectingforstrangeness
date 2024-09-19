#ifndef SLICEASSEMBLER_H
#define SLICEASSEMBLER_H

#include "TFile.h"
#include "TTree.h"
#include <vector>
#include <map>
#include <string>

class SliceAssembler
{
public:
    inline static const SliceAssembler& instance(const std::string& input_name)
    {
        static std::unique_ptr<SliceAssembler> the_instance(new SliceAssembler(input_name));
        return *the_instance;
    }

    SliceAssembler(const std::string& input_name)
    {
        file_ = TFile::Open(input_name.c_str(), "READ");
        tree_ = dynamic_cast<TTree*>(file_->Get("emptyselectionfilter/SliceAnalysis"));

        num_events_ = tree_->GetEntries();

        set_branch_addresses();
    }

    ~SliceAssembler()
    {
        if (file_)
        {
            file_->Close();
            delete file_;
        }
    }

    std::vector<std::map<std::string, float>> get_flashes(int i) const
    {
        std::vector<std::map<std::string, float>> flashes;
        tree_->GetEntry(i); 

        for (size_t j = 0; j < _flash_time_v->size(); ++j)
        {
            std::map<std::string, float> flash_data = {
                {"time", _flash_time_v->at(j)},
                {"total_pe", _flash_total_pe_v->at(j)},
                {"center_y", _flash_center_y_v->at(j)},
                {"center_z", _flash_center_z_v->at(j)},
                {"width_y", _flash_width_y_v->at(j)},
                {"width_z", _flash_width_z_v->at(j)}
            };

            flashes.push_back(flash_data);
        }

        return flashes;
    }

    const std::map<int, std::map<std::string, float>>& get_slices(int i) const
    {
        slices_.clear();  

        tree_->GetEntry(i);
            
        for (size_t j = 0; j < _slice_ids_v->size(); ++j)
        {
            slices_[_slice_ids_v->at(j)] = {
                {"completeness", _slice_completeness_v->at(j)},
                {"purity", _slice_purity_v->at(j)},
                {"topological_score", _slice_topological_score_v->at(j)},
                {"pandora_score", _slice_pandora_score_v->at(j)},
                {"center_x", _slice_center_x_v->at(j)},
                {"center_y", _slice_center_y_v->at(j)},
                {"center_z", _slice_center_z_v->at(j)},
                {"charge", _slice_charge_v->at(j)},
                {"n_hits", _slice_n_hits_v->at(j)}
            };
        }

        return slices_;
    }

    std::tuple<std::map<std::string, float>, std::map<std::string, float>, std::map<std::string, float>> get_defined_slices(int i) const
    {
        slices_.clear();  

        tree_->GetEntry(i);
            
        for (size_t j = 0; j < _slice_ids_v->size(); ++j)
        {
            slices_[_slice_ids_v->at(j)] = {
                {"completeness", _slice_completeness_v->at(j)},
                {"purity", _slice_purity_v->at(j)},
                {"topological_score", _slice_topological_score_v->at(j)},
                {"pandora_score", _slice_pandora_score_v->at(j)},
                {"center_x", _slice_center_x_v->at(j)},
                {"center_y", _slice_center_y_v->at(j)},
                {"center_z", _slice_center_z_v->at(j)},
                {"charge", _slice_charge_v->at(j)},
                {"n_hits", _slice_n_hits_v->at(j)}
            };
        }

        std::map<std::string, float> true_slice_properties;
        std::map<std::string, float> pandora_slice_properties;
        std::map<std::string, float> flash_slice_properties;

        auto true_slice_it = slices_.find(_true_nu_slice_id);
        if (true_slice_it != slices_.end()) {
            true_slice_properties = true_slice_it->second;
        }

        auto pandora_slice_it = slices_.find(_pandora_nu_slice_id);
        if (pandora_slice_it != slices_.end()) {
            pandora_slice_properties = pandora_slice_it->second;
        }

        auto flash_slice_it = slices_.find(_flash_match_nu_slice_id);
        if (flash_slice_it != slices_.end()) {
            flash_slice_properties = flash_slice_it->second;
        }

        return std::make_tuple(true_slice_properties, pandora_slice_properties, flash_slice_properties);
    }

    TVector3 get_true_nu_vertex() const
    {
        return TVector3(_true_reco_nu_vtx_x, _true_reco_nu_vtx_y, _true_reco_nu_vtx_z);
    }

    TVector3 get_pandora_nu_vertex() const
    {
        return TVector3(_pandora_reco_nu_vtx_x, _pandora_reco_nu_vtx_y, _pandora_reco_nu_vtx_z);
    }

    TVector3 get_flash_nu_vertex() const
    {
        return TVector3(_flash_reco_nu_vtx_x, _flash_reco_nu_vtx_y, _flash_reco_nu_vtx_z);
    }

    int get_true_slice_id() const
    {
        return _true_nu_slice_id;
    }

    int get_pandora_slice_id() const
    {
        return _pandora_nu_slice_id;
    }

    int get_flash_slice_id() const
    {
        return _flash_match_nu_slice_id;
    }

private:
    TFile* file_;
    TTree* tree_;

    int num_events_;

    std::vector<float>* _flash_time_v = nullptr;
    std::vector<float>* _flash_total_pe_v = nullptr;
    std::vector<float>* _flash_center_y_v = nullptr;
    std::vector<float>* _flash_center_z_v = nullptr;
    std::vector<float>* _flash_width_y_v = nullptr;
    std::vector<float>* _flash_width_z_v = nullptr;

    std::vector<float>* _slice_completeness_v = nullptr;
    std::vector<float>* _slice_purity_v = nullptr;
    std::vector<float>* _slice_topological_score_v = nullptr;
    std::vector<float>* _slice_pandora_score_v = nullptr;
    std::vector<int>* _slice_ids_v = nullptr;
    std::vector<int>* _slice_n_hits_v = nullptr;

    std::vector<float>* _slice_center_x_v = nullptr;
    std::vector<float>* _slice_center_y_v = nullptr;
    std::vector<float>* _slice_center_z_v = nullptr;
    std::vector<float>* _slice_charge_v = nullptr;

    int _true_nu_slice_id;
    int _pandora_nu_slice_id;
    int _flash_match_nu_slice_id;

    bool _flash_slice_found;
    bool _pandora_slice_found;

    float _true_slice_completeness;
    float _true_slice_purity;
    float _pandora_slice_completeness;
    float _pandora_slice_purity;
    float _flash_slice_completeness;
    float _flash_slice_purity;

    float _true_reco_nu_vtx_x;
    float _true_reco_nu_vtx_y;
    float _true_reco_nu_vtx_z;
    float _pandora_reco_nu_vtx_x;
    float _pandora_reco_nu_vtx_y;
    float _pandora_reco_nu_vtx_z;
    float _flash_reco_nu_vtx_x;
    float _flash_reco_nu_vtx_y;
    float _flash_reco_nu_vtx_z;

    mutable std::map<int, std::map<std::string, float>> slices_;

    void set_branch_addresses()
    {
        tree_->SetBranchAddress("flash_time", &_flash_time_v);
        tree_->SetBranchAddress("flash_total_pe", &_flash_total_pe_v);
        tree_->SetBranchAddress("flash_center_y", &_flash_center_y_v);
        tree_->SetBranchAddress("flash_center_z", &_flash_center_z_v);
        tree_->SetBranchAddress("flash_width_y", &_flash_width_y_v);
        tree_->SetBranchAddress("flash_width_z", &_flash_width_z_v);

        tree_->SetBranchAddress("slice_completeness", &_slice_completeness_v);
        tree_->SetBranchAddress("slice_purity", &_slice_purity_v);
        tree_->SetBranchAddress("slice_topological_score", &_slice_topological_score_v);
        tree_->SetBranchAddress("slice_pandora_score", &_slice_pandora_score_v);
        tree_->SetBranchAddress("slice_ids", &_slice_ids_v);
        tree_->SetBranchAddress("slice_n_hits", &_slice_n_hits_v);

        tree_->SetBranchAddress("slice_center_x", &_slice_center_x_v);
        tree_->SetBranchAddress("slice_center_y", &_slice_center_y_v);
        tree_->SetBranchAddress("slice_center_z", &_slice_center_z_v);
        tree_->SetBranchAddress("slice_charge", &_slice_charge_v);

        tree_->SetBranchAddress("true_nu_slice_id", &_true_nu_slice_id);
        tree_->SetBranchAddress("pandora_nu_slice_id", &_pandora_nu_slice_id);
        tree_->SetBranchAddress("flash_match_nu_slice_id", &_flash_match_nu_slice_id);

        tree_->SetBranchAddress("true_slice_completeness", &_true_slice_completeness);
        tree_->SetBranchAddress("true_slice_purity", &_true_slice_purity);
        tree_->SetBranchAddress("pandora_slice_completeness", &_pandora_slice_completeness);
        tree_->SetBranchAddress("pandora_slice_purity", &_pandora_slice_purity);
        tree_->SetBranchAddress("flash_slice_completeness", &_flash_slice_completeness);
        tree_->SetBranchAddress("flash_slice_purity", &_flash_slice_purity);

        tree_->SetBranchAddress("true_reco_nu_vtx_x", &_true_reco_nu_vtx_x);
        tree_->SetBranchAddress("true_reco_nu_vtx_y", &_true_reco_nu_vtx_y);
        tree_->SetBranchAddress("true_reco_nu_vtx_z", &_true_reco_nu_vtx_z);
        tree_->SetBranchAddress("pandora_reco_nu_vtx_x", &_pandora_reco_nu_vtx_x);
        tree_->SetBranchAddress("pandora_reco_nu_vtx_y", &_pandora_reco_nu_vtx_y);
        tree_->SetBranchAddress("pandora_reco_nu_vtx_z", &_pandora_reco_nu_vtx_z);
        tree_->SetBranchAddress("flash_reco_nu_vtx_x", &_flash_reco_nu_vtx_x);
        tree_->SetBranchAddress("flash_reco_nu_vtx_y", &_flash_reco_nu_vtx_y);
        tree_->SetBranchAddress("flash_reco_nu_vtx_z", &_flash_reco_nu_vtx_z);
    }
};

#endif // SLICEASSEMBLER_H
