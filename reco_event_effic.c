#include "AnalysisEvent.h"
#include "EventAssembler.h"
#include "SliceAssembler.h"
#include "PlotFunctions.h"
#include "DisplayAssembler.h"

#include "TH1D.h"
#include "TCanvas.h"
#include "TEfficiency.h"
#include "TLegend.h"
#include "TGaxis.h"
#include <iostream>
#include <vector>
#include <set> 
#include <cmath> 

void plot_efficiency(TEfficiency* effic, std::string title, std::string name, double x_min, double x_max)
{
    const double Single_CanvasX = 800;
    const double Single_CanvasY = 600; 
    const double Single_PadSplit = 0.85; // Height at which to split the cavas into plot/legend

    const double Single_XaxisTitleSize = 0.05;
    const double Single_YaxisTitleSize = 0.05;
    const double Single_XaxisTitleOffset = 0.93;
    const double Single_YaxisTitleOffset = 1.06;
    const double Single_XaxisLabelSize = 0.045;
    const double Single_YaxisLabelSize = 0.045;

    const double Single_TextLabelSize = 0.09; // Label size if bin labels are used

    // Two panel axis settings etc.

    const double Dual_CanvasX = 800;
    const double Dual_CanvasY = 750; 
    const double Dual_PadSplitLow = 0.3; // Height at which to split the canvas between the ratio and the main plot
    const double Dual_PadSplitHigh = 0.9; // Height at which to split the canvas between the main plot and the legend

    const double Dual_MainXaxisTitleSize = 0;
    const double Dual_MainYaxisTitleSize = 0.065;
    const double Dual_MainXaxisTitleOffset = 0;
    const double Dual_MainYaxisTitleOffset = 0.8;
    const double Dual_MainXaxisLabelSize = 0;
    const double Dual_MainYaxisLabelSize = 0.05;

    const double Dual_RatioXaxisTitleSize = 0.12;
    const double Dual_RatioYaxisTitleSize = 0.12;
    const double Dual_RatioXaxisTitleOffset = 0.8;
    const double Dual_RatioYaxisTitleOffset = 0.43;
    const double Dual_RatioXaxisLabelSize = 0.1;
    const double Dual_RatioYaxisLabelSize = 0.1;

    const double Dual_TextLabelSize = 0.17; // Label size if bin labels are used

    TH1D* h_Before = (TH1D*)effic->GetTotalHistogram()->Clone("h_Before");
    TH1D* h_After = (TH1D*)effic->GetPassedHistogram()->Clone("h_After");

    TCanvas *c = new TCanvas("c1","c1",Single_CanvasX,Single_CanvasY);
    TPad *p_plot = new TPad("pad1","pad1",0,0,1,Single_PadSplit);
    TPad *p_legend = new TPad("pad2","pad2",0,Single_PadSplit,1,1);
    p_legend->SetBottomMargin(0);
    p_legend->SetTopMargin(0.1);
    p_plot->SetTopMargin(0.01);

    THStack *hs = new THStack("hs", title.c_str());

    h_Before->SetLineWidth(1);
    h_Before->SetLineColor(1);
    hs->Add(h_Before);
    h_After->SetLineWidth(1);
    h_After->SetLineColor(3);
    hs->Add(h_After);

    c->cd();
    p_plot->Draw();
    p_plot->cd();

    hs->Draw("nostack E0");
    hs->GetXaxis()->SetTitleSize(Single_XaxisTitleSize);
    hs->GetYaxis()->SetTitleSize(Single_YaxisTitleSize);
    hs->GetXaxis()->SetTitleOffset(Single_XaxisTitleOffset);
    hs->GetYaxis()->SetTitleOffset(Single_YaxisTitleOffset);
    hs->GetXaxis()->SetLabelSize(Single_XaxisLabelSize);
    hs->GetYaxis()->SetLabelSize(Single_YaxisLabelSize);
    hs->SetMaximum(1.25*hs->GetMaximum("nostack"));

    p_plot->Update(); 

    effic->SetConfidenceLevel(0.68);
    effic->SetStatisticOption(TEfficiency::kBUniform);
    effic->SetPosteriorMode();

    std::vector<double> Efficiency_X;
    std::vector<double> Efficiency_CV;
    std::vector<double> Efficiency_Low;
    std::vector<double> Efficiency_High;

    double effmax = 0.0;
    for(size_t i = 1; i < h_Before->GetNbinsX() + 1; i++)
            effmax = std::max(effmax, effic->GetEfficiency(i) + effic->GetEfficiencyErrorUp(i));

    int binmax = h_Before->GetMaximumBin();
    Float_t rightmax = 1.2 * effmax;
    double scale = p_plot->GetUymax() / rightmax;
    for(size_t i = 1; i <h_Before->GetNbinsX() + 1; i++){
        if(std::isnan(effic->GetEfficiency(i))) continue;
        Efficiency_X.push_back(h_Before->GetBinCenter(i));
        Efficiency_CV.push_back(effic->GetEfficiency(i) * scale);
        Efficiency_Low.push_back(effic->GetEfficiencyErrorLow(i) * scale);
        Efficiency_High.push_back(effic->GetEfficiencyErrorUp(i) * scale);
    }

    TGraphAsymmErrors *g_Efficiency = new  TGraphAsymmErrors(Efficiency_X.size(),&(Efficiency_X[0]),&(Efficiency_CV[0]),0,0,&(Efficiency_Low[0]),&(Efficiency_High[0]));
    g_Efficiency->SetLineColor(kRed);
    g_Efficiency->SetMarkerStyle(5);
    g_Efficiency->SetMarkerSize(1);
    g_Efficiency->SetMarkerColor(kRed);
    g_Efficiency->SetLineWidth(1);

    TGaxis *axis = new TGaxis(p_plot->GetUxmax(),p_plot->GetUymin(),p_plot->GetUxmax(),p_plot->GetUymax(),0,rightmax,510,"+L");
    axis->SetTitleColor(kRed);
    axis->SetLabelColor(kRed);
    axis->SetTitleSize(Single_YaxisTitleSize);
    axis->SetTitleOffset(0.9*Single_YaxisTitleOffset);
    axis->SetLabelSize(Single_YaxisLabelSize);
    axis->SetTitle("Pattern Recognition Efficiency");
    axis->Draw();

    g_Efficiency->Draw("P same");

    c->cd();
    p_legend->Draw();
    p_legend->cd();

    TLegend *l = new TLegend(0.1,0.0,0.9,1.0);
    l->SetBorderSize(0);
    l->SetNColumns(2);
    l->AddEntry(h_Before, "All Events", "L");
    l->AddEntry(g_Efficiency, "Efficiency", "P");
    l->AddEntry(h_After, "Selected", "L");
    l->Draw();

    p_plot->Update(); 

    c->Print(("plots/Efficiency_" + name + "_Ratio.pdf").c_str());

    c->Close();
}

void reco_event_effic()
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/prod_strange_resample_fhc_run2_fhc_reco2_reco2_signalfilter_1000_analysis.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const DisplayAssembler& display_assembler = DisplayAssembler::instance(input_file);
    int num_events = event_assembler.get_num_events();

    int n_bins = 15;
    double x_min = 0.1;
    double x_max = 3.0;

    // Efficiency histograms for energy, separation, and opening angle
    TEfficiency* event_effic_energy = new TEfficiency("event_effic_energy", ";True Kaon-Short Energy [GeV];Reconstruction Efficiency", n_bins, x_min, x_max);

    double x_min_sep = 0;
    double x_max_sep = 24;
    int n_bins_sep = 24;
    TEfficiency* event_effic_sep = new TEfficiency("event_effic_sep", ";True K_{S}^{0} Decay Distance [cm];Event Recognition Efficiency", n_bins_sep, x_min_sep, x_max_sep);

    double x_min_ang = 0.0;
    double x_max_ang = 3.14;
    int n_bins_ang = 15;
    TEfficiency* event_effic_angle = new TEfficiency("event_effic_angle", ";True Decay Opening Angle [rad];Reconstruction Efficiency", n_bins_ang, x_min_ang, x_max_ang);

    // Momentum efficiency histograms for muon, pion-plus, and pion-minus
    double x_min_momentum = 0.0;
    double x_max_momentum = 3.0; // GeV/c (adjust this based on the range of momenta in your events)
    TEfficiency* event_effic_muon_momentum = new TEfficiency("event_effic_muon_momentum", ";True Muon Momentum [GeV/c];Reconstruction Efficiency", n_bins, x_min_momentum, x_max_momentum);
    TEfficiency* event_effic_piplus_momentum = new TEfficiency("event_effic_piplus_momentum", ";True Pion-Plus Momentum [GeV/c];Reconstruction Efficiency", n_bins, x_min_momentum, x_max_momentum);
    TEfficiency* event_effic_piminus_momentum = new TEfficiency("event_effic_piminus_momentum", ";True Pion-Minus Momentum [GeV/c];Reconstruction Efficiency", n_bins, x_min_momentum, x_max_momentum);

    TEfficiency* event_effic_W = new TEfficiency("", ";True W;Reconstruction Efficiency", 100, 0, 100);
    TEfficiency* event_effic_X = new TEfficiency("", ";True X;Reconstruction Efficiency", 100, 0, 100);
    TEfficiency* event_effic_Y = new TEfficiency("", ";True Y;Reconstruction Efficiency", 100, 0, 100);
    TEfficiency* event_effic_QSqr = new TEfficiency("", ";True QSqr;Reconstruction Efficiency", 100, 0, 100);

    TEfficiency* event_effic_nu_energy = new TEfficiency("", ";True Neutrino Energy;Reconstruction Efficiency", 24, 0, -1);

    int well_reconstructed_filled = 0;
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_is_kshort_decay_pionic) continue;
        if (!event.mc_has_muon) continue;

        float kshort_energy = event.mc_kshrt_total_energy;
        float kshort_sep = event.mc_kshrt_end_sep;

        // Calculate opening angle between pion-plus and pion-minus
        TVector3 piplus_vec(event.mc_kshrt_piplus_px, event.mc_kshrt_piplus_py, event.mc_kshrt_piplus_pz);
        TVector3 piminus_vec(event.mc_kshrt_piminus_px, event.mc_kshrt_piminus_py, event.mc_kshrt_piminus_pz);
        float angle_rad = piplus_vec.Angle(piminus_vec);

        // Calculate momenta for muon, pion-plus, and pion-minus
        float muon_momentum = std::sqrt(event.mc_muon_px * event.mc_muon_px +
                                        event.mc_muon_py * event.mc_muon_py +
                                        event.mc_muon_pz * event.mc_muon_pz);
        float piplus_momentum = piplus_vec.Mag();  // Magnitude of the pion-plus momentum vector
        float piminus_momentum = piminus_vec.Mag(); // Magnitude of the pion-minus momentum vector

        size_t muon_pf_index = -1;
        size_t piplus_pf_index = -1;
        size_t piminus_pf_index = -1;

        bool muon_reco = false;
        if (event.mc_muon_tid) {
            if (event.pfp_muon_purity && event.pfp_muon_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_muon_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_muon_purity->at(pfp_idx);
                    float completeness = event.pfp_muon_completeness->at(pfp_idx);
                    if (purity >= 0.5 && completeness >= 0.1) {
                        muon_reco = true;
                        muon_pf_index = pfp_idx; 
                        break;
                    }
                }
            }
        }

        bool piplus_reco = false;
        if (event.mc_kshrt_piplus_tid) {
            if (event.pfp_piplus_purity && event.pfp_piplus_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_piplus_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_piplus_purity->at(pfp_idx);
                    float completeness = event.pfp_piplus_completeness->at(pfp_idx);
                    if (purity >= 0.5 && completeness >= 0.1) {
                        piplus_reco = true;
                        piplus_pf_index = pfp_idx; 
                        break;
                    }
                }
            }
        }

        bool piminus_reco = false;
        if (event.mc_kshrt_piminus_tid) {
            if (event.pfp_piminus_purity && event.pfp_piminus_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_piminus_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_piminus_purity->at(pfp_idx);
                    float completeness = event.pfp_piminus_completeness->at(pfp_idx);
                    if (purity >= 0.5 && completeness >= 0.1) {
                        piminus_reco = true;
                        piminus_pf_index = pfp_idx;  
                        break;
                    }
                }
            }
        }

        // Check if muon, pion-plus, and pion-minus are well reconstructed and independent
        if (muon_reco && piplus_reco && piminus_reco &&
            muon_pf_index != piplus_pf_index &&
            muon_pf_index != piminus_pf_index &&
            piplus_pf_index != piminus_pf_index) 
        {
            event_effic_energy->Fill(true, kshort_energy);
            event_effic_sep->Fill(true, kshort_sep);
            event_effic_angle->Fill(true, angle_rad);

            // Fill momentum efficiencies
            event_effic_muon_momentum->Fill(true, muon_momentum);
            event_effic_piplus_momentum->Fill(true, piplus_momentum);
            event_effic_piminus_momentum->Fill(true, piminus_momentum);

            event_effic_W->Fill(true, event.mc_nu_W);
            event_effic_X->Fill(true, event.mc_nu_X);
            event_effic_Y->Fill(true, event.mc_nu_Y);
            event_effic_QSqr->Fill(true, event.mc_nu_QSqr);

            event_effic_nu_energy->Fill(true, event.mc_nu_energy);

            well_reconstructed_filled++;
        } else {
            event_effic_energy->Fill(false, kshort_energy);
            event_effic_sep->Fill(false, kshort_sep);
            event_effic_angle->Fill(false, angle_rad);

            // Fill momentum efficiencies for false
            event_effic_muon_momentum->Fill(false, muon_momentum);
            event_effic_piplus_momentum->Fill(false, piplus_momentum);
            event_effic_piminus_momentum->Fill(false, piminus_momentum);

            event_effic_W->Fill(false, event.mc_nu_W);
            event_effic_X->Fill(false, event.mc_nu_X);
            event_effic_Y->Fill(false, event.mc_nu_Y);
            event_effic_QSqr->Fill(false, event.mc_nu_QSqr);

            event_effic_nu_energy->Fill(false, event.mc_nu_energy);

            if (well_reconstructed_filled < 8)
                display_assembler.plot_event(i);
        }
    }

    std::cout << "Well-reconstructed events filled: " << well_reconstructed_filled << std::endl;

    // Plot the efficiency as a function of energy, separation, angle, and momentum
    plot_efficiency(event_effic_energy, ";True Kaon-Short Energy [GeV]; Event/bin", "kshort_energy", 0.1, 3.0);
    plot_efficiency(event_effic_sep, ";True K_{S}^{0} Decay-Distance [cm]; Events/bin", "kshort_sep", 0.1, 100.0);
    plot_efficiency(event_effic_angle, ";True Decay Opening Angle [rad]; Event/bin", "opening_angle", 0.0, 3.14);

    plot_efficiency(event_effic_muon_momentum, ";True Muon Momentum [GeV/c]; Event/bin", "muon_momentum", 0.0, 3.0);
    plot_efficiency(event_effic_piplus_momentum, ";True Pion-Plus Momentum [GeV/c]; Event/bin", "piplus_momentum", 0.0, 3.0);
    plot_efficiency(event_effic_piminus_momentum, ";True Pion-Minus Momentum [GeV/c]; Event/bin", "piminus_momentum", 0.0, 3.0);

    plot_efficiency(event_effic_W, ";True W; Events/bin", "W", 0, 100);
    plot_efficiency(event_effic_X, ";True X; Events/bin", "X", 0, 100);
    plot_efficiency(event_effic_Y, ";True Y; Events/bin", "Y", 0, 100);
    plot_efficiency(event_effic_QSqr, ";True QSqr; Events/bin", "Qsqr", 0, 100);

    plot_efficiency(event_effic_nu_energy, ";True Neutrino Energy [GeV]; Enrties/bin", "energy", 0, -1);

    // Cleanup
    delete event_effic_energy;
    delete event_effic_sep;
    delete event_effic_angle;
    delete event_effic_muon_momentum;
    delete event_effic_piplus_momentum;
    delete event_effic_piminus_momentum;
}
