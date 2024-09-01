#ifndef EVENTDISPLAY_H
#define EVENTDISPLAY_H

#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TROOT.h"
#include <map>
#include <algorithm>
#include <string>

class EventDisplay
{
public:
    EventDisplay(const std::string& input_name) 
    {
        file_ = TFile::Open(input_name.c_str(), "READ");
        tree_ = dynamic_cast<TTree*>(file_->Get("visualiseSlice/VisualisationTree"));

        set_branch_addresses();
    }

    ~EventDisplay() 
    {
        if (file_) 
        {
            file_->Close();
            delete file_;
        }
    }

    void plot_event(int i_event) 
    {
        tree_->GetEntry(i_event);

        display_event_hits();
        display_reconstructed_hits();
    }

private:
    TFile* file_;
    TTree* tree_;

    int event_, run_, subrun_;
    double true_nu_vtx_x_, true_nu_vtx_u_wire_, true_nu_vtx_v_wire_, true_nu_vtx_w_wire_;
    double reco_nu_vtx_x_, reco_nu_vtx_u_wire_, reco_nu_vtx_v_wire_, reco_nu_vtx_w_wire_;
    std::vector<double> *hits_u_wire_ = nullptr, *hits_u_drift_ = nullptr, *hits_u_owner_ = nullptr;
    std::vector<double> *hits_v_wire_ = nullptr, *hits_v_drift_ = nullptr, *hits_v_owner_ = nullptr;
    std::vector<double> *hits_w_wire_ = nullptr, *hits_w_drift_ = nullptr, *hits_w_owner_ = nullptr;
    std::vector<int> *pandora_pfp_code_ = nullptr;
    std::vector<std::vector<double>> *reco_hits_u_wire_ = nullptr, *reco_hits_u_drift_ = nullptr;
    std::vector<std::vector<double>> *reco_hits_v_wire_ = nullptr, *reco_hits_v_drift_ = nullptr;
    std::vector<std::vector<double>> *reco_hits_w_wire_ = nullptr, *reco_hits_w_drift_ = nullptr;

    void set_branch_addresses() 
    {
        tree_->SetBranchAddress("Event", &event_);
        tree_->SetBranchAddress("Run", &run_);
        tree_->SetBranchAddress("Subrun", &subrun_);
        tree_->SetBranchAddress("TrueNuVertexX", &true_nu_vtx_x_);
        tree_->SetBranchAddress("UTrueNuVertex_wire", &true_nu_vtx_u_wire_);
        tree_->SetBranchAddress("VTrueNuVertex_wire", &true_nu_vtx_v_wire_);
        tree_->SetBranchAddress("WTrueNuVertex_wire", &true_nu_vtx_w_wire_);
        tree_->SetBranchAddress("RecoNuVertexX", &reco_nu_vtx_x_);
        tree_->SetBranchAddress("URecoNuVertex_wire", &reco_nu_vtx_u_wire_);
        tree_->SetBranchAddress("VRecoNuVertex_wire", &reco_nu_vtx_v_wire_);
        tree_->SetBranchAddress("WRecoNuVertex_wire", &reco_nu_vtx_w_wire_);
        tree_->SetBranchAddress("AllUHits_wire", &hits_u_wire_);
        tree_->SetBranchAddress("AllUHits_drift", &hits_u_drift_);
        tree_->SetBranchAddress("AllUHits_owner", &hits_u_owner_);
        tree_->SetBranchAddress("AllVHits_wire", &hits_v_wire_);
        tree_->SetBranchAddress("AllVHits_drift", &hits_v_drift_);
        tree_->SetBranchAddress("AllVHits_owner", &hits_v_owner_);
        tree_->SetBranchAddress("AllWHits_wire", &hits_w_wire_);
        tree_->SetBranchAddress("AllWHits_drift", &hits_w_drift_);
        tree_->SetBranchAddress("AllWHits_owner", &hits_w_owner_);
        tree_->SetBranchAddress("PandoraPFPCode", &pandora_pfp_code_);
        tree_->SetBranchAddress("UHits_wire", &reco_hits_u_wire_);
        tree_->SetBranchAddress("UHits_drift", &reco_hits_u_drift_);
        tree_->SetBranchAddress("VHits_wire", &reco_hits_v_wire_);
        tree_->SetBranchAddress("VHits_drift", &reco_hits_v_drift_);
        tree_->SetBranchAddress("WHits_wire", &reco_hits_w_wire_);
        tree_->SetBranchAddress("WHits_drift", &reco_hits_w_drift_);
    }

    void get_limits(const std::vector<double>& wire_coord_vec, const std::vector<double>& drift_coord_vec,
                   double& global_wire_min, double& global_wire_max, double& global_drift_min, double& global_drift_max)
    {
        if (!wire_coord_vec.empty() && !drift_coord_vec.empty()) {
            double local_wire_min = *std::min_element(wire_coord_vec.begin(), wire_coord_vec.end());
            double local_wire_max = *std::max_element(wire_coord_vec.begin(), wire_coord_vec.end());
            double local_drift_min = *std::min_element(drift_coord_vec.begin(), drift_coord_vec.end());
            double local_drift_max = *std::max_element(drift_coord_vec.begin(), drift_coord_vec.end());

            global_wire_min = std::min(global_wire_min, local_wire_min);
            global_wire_max = std::max(global_wire_max, local_wire_max);
            global_drift_min = std::min(global_drift_min, local_drift_min);
            global_drift_max = std::max(global_drift_max, local_drift_max);
        }
    }

    void display_event_hits() {
        double global_true_drift_min = 1e10, global_true_drift_max = -1e10;
        double wire_min_u_truth = 1e10, wire_max_u_truth = -1e10;
        double wire_min_v_truth = 1e10, wire_max_v_truth = -1e10;
        double wire_min_w_truth = 1e10, wire_max_w_truth = -1e10;
        double buffer = 10.0;

        get_limits(*hits_u_wire_, *hits_u_drift_, wire_min_u_truth, wire_max_u_truth, global_true_drift_min, global_true_drift_max);
        get_limits(*hits_v_wire_, *hits_v_drift_, wire_min_v_truth, wire_max_v_truth, global_true_drift_min, global_true_drift_max);
        get_limits(*hits_w_wire_, *hits_w_drift_, wire_min_w_truth, wire_max_w_truth, global_true_drift_min, global_true_drift_max);

        // Create TMultiGraphs and TGraphs for each view (U, V, W)
        TCanvas* c4 = new TCanvas("c4", "", 1200, 1200);
        c4->Divide(1, 3, 0, 0);

        TMultiGraph* mg_u = new TMultiGraph();
        TMultiGraph* mg_v = new TMultiGraph();
        TMultiGraph* mg_w = new TMultiGraph();

        mg_u->SetTitle(";Drift Coordinate;U Wire");
        mg_v->SetTitle(";Drift Coordinate;V Wire");
        mg_w->SetTitle(";Drift Coordinate;W Wire");

        // TGraphs for true vertex positions
        TGraph* true_vertex_u = new TGraph();
        true_vertex_u->SetPoint(0, true_nu_vtx_x_, true_nu_vtx_u_wire_);
        true_vertex_u->SetMarkerStyle(4);
        true_vertex_u->SetMarkerColor(kBlack);
        true_vertex_u->SetMarkerSize(1.0);

        TGraph* true_vertex_v = new TGraph();
        true_vertex_v->SetPoint(0, true_nu_vtx_x_, true_nu_vtx_v_wire_);
        true_vertex_v->SetMarkerStyle(4);
        true_vertex_v->SetMarkerColor(kBlack);
        true_vertex_v->SetMarkerSize(1.0);

        TGraph* true_vertex_w = new TGraph();
        true_vertex_w->SetPoint(0, true_nu_vtx_x_, true_nu_vtx_w_wire_);
        true_vertex_w->SetMarkerStyle(4);
        true_vertex_w->SetMarkerColor(kBlack);
        true_vertex_w->SetMarkerSize(1.0);

        // Create TGraphs for each PDG
        std::map<int, TGraph*> pdgGraphs;
        for (size_t i = 0; i < hits_u_wire_->size(); ++i) {
            int pdg = std::abs(hits_u_owner_->at(i));

            if (pdgGraphs.find(pdg) == pdgGraphs.end()) {
                pdgGraphs[pdg] = new TGraph();
                pdgGraphs[pdg]->SetMarkerStyle(20);
                pdgGraphs[pdg]->SetMarkerSize(0.25);
                pdgGraphs[pdg]->SetMarkerColor(kGray); // Default color

                if (pdg == 13) pdgGraphs[pdg]->SetMarkerColor(kBlue); // Muon
                else if (pdg == 11) pdgGraphs[pdg]->SetMarkerColor(kRed); // Electron
                else if (pdg == 2212) pdgGraphs[pdg]->SetMarkerColor(kGreen); // Proton
                else if (pdg == 211) pdgGraphs[pdg]->SetMarkerColor(kPink + 9); // Pion
                else if (pdg == 22) pdgGraphs[pdg]->SetMarkerColor(kOrange); // Photon
            }

            pdgGraphs[pdg]->SetPoint(pdgGraphs[pdg]->GetN(), hits_u_drift_->at(i), hits_u_wire_->at(i));
        }

        for (auto& entry : pdgGraphs) {
            mg_u->Add(entry.second);
        }
        mg_u->Add(true_vertex_u);

        c4->cd(1);
        mg_u->Draw("AP");
        mg_u->GetXaxis()->SetLimits(global_true_drift_min - buffer, global_true_drift_max + buffer);

        pdgGraphs.clear(); 

        // Repeat for V view
        for (size_t i = 0; i < hits_v_wire_->size(); ++i) {
            int pdg = std::abs(hits_v_owner_->at(i));

            if (pdgGraphs.find(pdg) == pdgGraphs.end()) {
                pdgGraphs[pdg] = new TGraph();
                pdgGraphs[pdg]->SetMarkerStyle(20);
                pdgGraphs[pdg]->SetMarkerSize(0.25);
                pdgGraphs[pdg]->SetMarkerColor(kGray); // Default color

                if (pdg == 13) pdgGraphs[pdg]->SetMarkerColor(kBlue); // Muon
                else if (pdg == 11) pdgGraphs[pdg]->SetMarkerColor(kRed); // Electron
                else if (pdg == 2212) pdgGraphs[pdg]->SetMarkerColor(kGreen); // Proton
                else if (pdg == 211) pdgGraphs[pdg]->SetMarkerColor(kPink + 9); // Pion
                else if (pdg == 22) pdgGraphs[pdg]->SetMarkerColor(kOrange); // Photon
            }

            pdgGraphs[pdg]->SetPoint(pdgGraphs[pdg]->GetN(), hits_v_drift_->at(i), hits_v_wire_->at(i));
        }

        for (auto& entry : pdgGraphs) {
            mg_v->Add(entry.second);
        }
        mg_v->Add(true_vertex_v);

        c4->cd(2);
        mg_v->Draw("AP");
        mg_v->GetXaxis()->SetLimits(global_true_drift_min - buffer, global_true_drift_max + buffer);

        pdgGraphs.clear(); 

        // Repeat for W view
        for (size_t i = 0; i < hits_w_wire_->size(); ++i) {
            int pdg = std::abs(hits_w_owner_->at(i));

            if (pdgGraphs.find(pdg) == pdgGraphs.end()) {
                pdgGraphs[pdg] = new TGraph();
                pdgGraphs[pdg]->SetMarkerStyle(20);
                pdgGraphs[pdg]->SetMarkerSize(0.25);
                pdgGraphs[pdg]->SetMarkerColor(kGray); // Default color

                if (pdg == 13) pdgGraphs[pdg]->SetMarkerColor(kBlue); // Muon
                else if (pdg == 11) pdgGraphs[pdg]->SetMarkerColor(kRed); // Electron
                else if (pdg == 2212) pdgGraphs[pdg]->SetMarkerColor(kGreen); // Proton
                else if (pdg == 211) pdgGraphs[pdg]->SetMarkerColor(kPink + 9); // Pion
                else if (pdg == 22) pdgGraphs[pdg]->SetMarkerColor(kOrange); // Photon
            }

            pdgGraphs[pdg]->SetPoint(pdgGraphs[pdg]->GetN(), hits_w_drift_->at(i), hits_w_wire_->at(i));
        }

        for (auto& entry : pdgGraphs) {
            mg_w->Add(entry.second);
        }
        mg_w->Add(true_vertex_w);

        c4->cd(3);
        mg_w->Draw("AP");
        mg_w->GetXaxis()->SetLimits(global_true_drift_min - buffer, global_true_drift_max + buffer);

        std::string filename = "true_interaction_hits_" + std::to_string(run_) + "_" + std::to_string(subrun_) + "_" + std::to_string(event_);
        c4->SaveAs((filename + ".png").c_str());

        delete mg_u;
        delete mg_v;
        delete mg_w;
        delete true_vertex_u;
        delete true_vertex_v;
        delete true_vertex_w;
        delete c4;
    }

    void display_reconstructed_hits() {
        double global_reco_drift_min = 1e10, global_reco_drift_max = -1e10;
        double wire_min_u_slice = 1e10, wire_max_u_slice = -1e10;
        double wire_min_v_slice = 1e10, wire_max_v_slice = -1e10;
        double wire_min_w_slice = 1e10, wire_max_w_slice = -1e10;
        double buffer = 10.0;

        // Calculate global min and max for reconstructed hits
        for (size_t i = 0; i < reco_hits_u_wire_->size(); ++i) {
            get_limits(reco_hits_u_wire_->at(i), reco_hits_u_drift_->at(i), wire_min_u_slice, wire_max_u_slice, global_reco_drift_min, global_reco_drift_max);
        }
        for (size_t i = 0; i < reco_hits_v_wire_->size(); ++i) {
            get_limits(reco_hits_v_wire_->at(i), reco_hits_v_drift_->at(i), wire_min_v_slice, wire_max_v_slice, global_reco_drift_min, global_reco_drift_max);
        }
        for (size_t i = 0; i < reco_hits_w_wire_->size(); ++i) {
            get_limits(reco_hits_w_wire_->at(i), reco_hits_w_drift_->at(i), wire_min_w_slice, wire_max_w_slice, global_reco_drift_min, global_reco_drift_max);
        }

        // Create TMultiGraphs and TGraphs for each view (U, V, W)
        TCanvas* c5 = new TCanvas("c5", "", 1200, 1200);
        c5->Divide(1, 3, 0, 0);

        TMultiGraph* reco_mg_u = new TMultiGraph();
        TMultiGraph* reco_mg_v = new TMultiGraph();
        TMultiGraph* reco_mg_w = new TMultiGraph();

        reco_mg_u->SetTitle(";Drift Coordinate;U Wire");
        reco_mg_v->SetTitle(";Drift Coordinate;V Wire");
        reco_mg_w->SetTitle(";Drift Coordinate;W Wire");

        // TGraphs for reconstructed vertex positions
        TGraph* reco_vertex_u = new TGraph();
        reco_vertex_u->SetPoint(0, reco_nu_vtx_x_, reco_nu_vtx_u_wire_);
        reco_vertex_u->SetMarkerStyle(4);
        reco_vertex_u->SetMarkerColor(kBlack);
        reco_vertex_u->SetMarkerSize(1.0);

        TGraph* reco_vertex_v = new TGraph();
        reco_vertex_v->SetPoint(0, reco_nu_vtx_x_, reco_nu_vtx_v_wire_);
        reco_vertex_v->SetMarkerStyle(4);
        reco_vertex_v->SetMarkerColor(kBlack);
        reco_vertex_v->SetMarkerSize(1.0);

        TGraph* reco_vertex_w = new TGraph();
        reco_vertex_w->SetPoint(0, reco_nu_vtx_x_, reco_nu_vtx_w_wire_);
        reco_vertex_w->SetMarkerStyle(4);
        reco_vertex_w->SetMarkerColor(kBlack);
        reco_vertex_w->SetMarkerSize(1.0);

        // Color map for different reconstructed particles
        std::vector<int> color_map = {
            kRed, kBlue, kGreen, kMagenta, kCyan, kYellow, kOrange, kPink, kViolet,
            kSpring, kTeal, kAzure, kRose, kGray, kBlack, kOrange + 7, kBlue - 9, kGreen + 3
        };

        int colorMapIndex = 0;
        for (size_t i = 0; i < pandora_pfp_code_->size(); ++i) {
            int particle_color = color_map[colorMapIndex % color_map.size()];
            colorMapIndex++;

            TGraph* pfp_graph_u = new TGraph();
            pfp_graph_u->SetMarkerStyle(20);
            pfp_graph_u->SetMarkerSize(0.25);
            pfp_graph_u->SetMarkerColor(particle_color);

            for (size_t hit = 0; hit < reco_hits_u_drift_->at(i).size(); ++hit) {
                pfp_graph_u->SetPoint(pfp_graph_u->GetN(), reco_hits_u_drift_->at(i).at(hit), reco_hits_u_wire_->at(i).at(hit));
            }

            reco_mg_u->Add(pfp_graph_u);

            TGraph* pfp_graph_v = new TGraph();
            pfp_graph_v->SetMarkerStyle(20);
            pfp_graph_v->SetMarkerSize(0.25);
            pfp_graph_v->SetMarkerColor(particle_color);

            for (size_t hit = 0; hit < reco_hits_v_drift_->at(i).size(); ++hit) {
                pfp_graph_v->SetPoint(pfp_graph_v->GetN(), reco_hits_v_drift_->at(i).at(hit), reco_hits_v_wire_->at(i).at(hit));
            }

            reco_mg_v->Add(pfp_graph_v);

            TGraph* pfp_graph_w = new TGraph();
            pfp_graph_w->SetMarkerStyle(20);
            pfp_graph_w->SetMarkerSize(0.25);
            pfp_graph_w->SetMarkerColor(particle_color);

            for (size_t hit = 0; hit < reco_hits_w_drift_->at(i).size(); ++hit) {
                pfp_graph_w->SetPoint(pfp_graph_w->GetN(), reco_hits_w_drift_->at(i).at(hit), reco_hits_w_wire_->at(i).at(hit));
            }

            reco_mg_w->Add(pfp_graph_w);
        }

        reco_mg_u->Add(reco_vertex_u);
        reco_mg_v->Add(reco_vertex_v);
        reco_mg_w->Add(reco_vertex_w);

        c5->cd(1);
        reco_mg_u->Draw("AP");
        reco_mg_u->GetXaxis()->SetLimits(global_reco_drift_min - buffer, global_reco_drift_max + buffer);

        c5->cd(2);
        reco_mg_v->Draw("AP");
        reco_mg_v->GetXaxis()->SetLimits(global_reco_drift_min - buffer, global_reco_drift_max + buffer);

        c5->cd(3);
        reco_mg_w->Draw("AP");
        reco_mg_w->GetXaxis()->SetLimits(global_reco_drift_min - buffer, global_reco_drift_max + buffer);

        std::string filename = "reco_hits_" + std::to_string(run_) + "_" + std::to_string(subrun_) + "_" + std::to_string(event_);
        c5->SaveAs((filename + ".png").c_str());

        delete reco_mg_u;
        delete reco_mg_v;
        delete reco_mg_w;
        delete reco_vertex_u;
        delete reco_vertex_v;
        delete reco_vertex_w;
        delete c5;
    }
};

#endif // EVENTDISPLAY_H
