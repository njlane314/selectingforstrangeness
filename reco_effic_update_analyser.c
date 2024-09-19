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

// Function to plot efficiencies with dynamic x-axis range
void plot_efficiency(TEfficiency* mu_effic, TEfficiency* piplus_effic, TEfficiency* piminus_effic, 
                     const char* x_axis_label, const char* output_filename,
                     double x_min, double x_max) 
{
    TCanvas* c = new TCanvas("c", "", 800, 600);
    c->SetLogx();

    // Muon efficiency plot
    mu_effic->SetConfidenceLevel(0.68);
    mu_effic->SetStatisticOption(TEfficiency::kBUniform);
    mu_effic->SetPosteriorMode();
    mu_effic->SetLineColor(kBlue);
    mu_effic->SetMarkerColor(kBlue);
    mu_effic->SetMarkerStyle(5);
    mu_effic->SetMarkerSize(2);
    mu_effic->SetLineWidth(1);
    mu_effic->Draw("AP");

    // Pion-plus efficiency plot
    piplus_effic->SetConfidenceLevel(0.68);
    piplus_effic->SetStatisticOption(TEfficiency::kBUniform);
    piplus_effic->SetPosteriorMode();
    piplus_effic->SetLineColor(kRed);
    piplus_effic->SetMarkerColor(kRed);
    piplus_effic->SetMarkerStyle(5);
    piplus_effic->SetMarkerSize(2);
    piplus_effic->SetLineWidth(1);
    piplus_effic->Draw("P same");

    // Pion-minus efficiency plot
    piminus_effic->SetConfidenceLevel(0.68);
    piminus_effic->SetStatisticOption(TEfficiency::kBUniform);
    piminus_effic->SetPosteriorMode();
    piminus_effic->SetLineColor(kGreen + 2);
    piminus_effic->SetMarkerColor(kGreen + 2);
    piminus_effic->SetMarkerStyle(5);
    piminus_effic->SetMarkerSize(2);
    piminus_effic->SetLineWidth(1);
    piminus_effic->Draw("P same");

    // Update canvas and set axis ranges
    c->Update();
    mu_effic->GetPaintedGraph()->GetYaxis()->SetRangeUser(0, 1);
    mu_effic->GetPaintedGraph()->GetXaxis()->SetRangeUser(x_min, x_max); // Set x-axis range dynamically
    piplus_effic->GetPaintedGraph()->GetXaxis()->SetRangeUser(x_min, x_max);
    piminus_effic->GetPaintedGraph()->GetXaxis()->SetRangeUser(x_min, x_max);

    // Add legend with LaTeX symbols for muon and pion
    TLegend* l = new TLegend(0.1, 0.91, 0.9, 0.99);
    l->SetNColumns(3);
    l->SetBorderSize(0);
    l->AddEntry(mu_effic, "#mu Efficiency", "P");         // LaTeX symbol for Muon
    l->AddEntry(piplus_effic, "#pi^{+} Efficiency", "P");  // LaTeX symbol for Pion-Plus
    l->AddEntry(piminus_effic, "#pi^{-} Efficiency", "P"); // LaTeX symbol for Pion-Minus
    l->Draw();

    // Save the plot
    c->SaveAs(output_filename);

    // Cleanup
    delete c;
}

void reco_effic_update_analyser() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/prod_strange_resample_fhc_run2_fhc_reco2_reco2_signalfilter_1000_analysis.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    int num_events = event_assembler.get_num_events();

    int n_bins = 8;
    double x_min = 0.1;  
    double x_max = 3.14;

    // Efficiency objects for true energy and momentum
    TEfficiency* mu_effic = new TEfficiency("mu_effic", "Muon Efficiency;True Particle Energy [GeV]; Recognition Efficiency", n_bins, x_min, x_max);
    TEfficiency* piplus_effic = new TEfficiency("piplus_effic", "Pion-Plus Efficiency;True Particle Energy [GeV]; Recognition Efficiency", n_bins, x_min, x_max);
    TEfficiency* piminus_effic = new TEfficiency("piminus_effic", "Pion-Minus Efficiency;True Particle Energy [GeV]; Recognition Efficiency", n_bins, x_min, x_max);

    TEfficiency* mu_mom_effic = new TEfficiency("mu_mom_effic", "Muon Efficiency;True Particle Momentum [GeV/c]; Recognition Efficiency", n_bins, x_min, x_max);
    TEfficiency* piplus_mom_effic = new TEfficiency("piplus_mom_effic", "Pion-Plus Efficiency;True Particle Momentum [GeV/c]; Recognition Efficiency", n_bins, x_min, x_max);
    TEfficiency* piminus_mom_effic = new TEfficiency("piminus_mom_effic", "Pion-Minus Efficiency;True Particle Momentum [GeV/c]; Recognition Efficiency", n_bins, x_min, x_max);

    TEfficiency* mu_mom_kshrt_energy = new TEfficiency("mu_mom_effic", "Muon Efficiency;True Kaon-Short Energy [GeV]; Recognition Efficiency", n_bins, x_min, x_max);
    TEfficiency* piplus_mom_kshrt_energy = new TEfficiency("piplus_mom_effic", "Pion-Plus Efficiency;True Kaon-Short Energy [GeV]; Recognition Efficiency", n_bins, x_min, x_max);
    TEfficiency* piminus_mom_kshrt_energy = new TEfficiency("piminus_mom_effic", "Pion-Minus Efficiency;True Kaon-Short Energy [GeV]; Recognition Efficiency", n_bins, x_min, x_max);

    int n_bins_sep = 100;
    double x_min_sep = 0.1;
    double x_max_sep = 100;

    TEfficiency* mu_mom_kshrt_sep = new TEfficiency("mu_mom_effic", "Muon Efficiency;True Kaon-Short Decay Distance []; Recognition Efficiency", n_bins_sep, x_min_sep, x_max_sep);
    TEfficiency* piplus_mom_kshrt_sep = new TEfficiency("piplus_mom_effic", "Pion-Plus Efficiency;True Kaon-Short Decay Distance []; Recognition Efficiency", n_bins_sep, x_min_sep, x_max_sep);
    TEfficiency* piminus_mom_kshrt_sep = new TEfficiency("piminus_mom_effic", "Pion-Minus Efficiency;True Kaon-Short Decay Distance []; Recognition Efficiency", n_bins_sep, x_min_sep, x_max_sep);

    int n_bins_ang = 8;
    double x_min_ang = 0; 
    double x_max_ang = 3.14;

    TEfficiency* mu_mom_open_angle = new TEfficiency("mu_mom_effic", "Muon Efficiency;True Decay Opening Angle [rad]; Recognition Efficiency", n_bins_ang, x_min_ang, x_max_ang);
    TEfficiency* piplus_mom_open_angle = new TEfficiency("piplus_mom_effic", "Pion-Plus Efficiency;True Decay Opening Angle [rad]; Recognition Efficiency", n_bins_ang, x_min_ang, x_max_ang);
    TEfficiency* piminus_mom_open_angle = new TEfficiency("piminus_mom_effic", "Pion-Minus Efficiency;True Decay Opening Angle [rad]; Recognition Efficiency", n_bins_ang, x_min_ang, x_max_ang);

    /*int n_bins_w = 10;
    double x_min_w = 0; 
    double x_max_w = -1;

    TEfficiency* mu_w = new TEfficiency("", ";True Event Invariant Mass [GeV]; Recognition Efficiency", n_bins_w, x_min_w, x_max_w);
    TEfficiency* piplus_w = new TEfficiency("", ";True Event Invariant Mass [GeV]; Recognition Efficiency", n_bins_w, x_min_w, x_max_w);
    TEfficiency* piminus_w = new TEfficiency("", ";True Event Invariant Mass [GeV]; Recognition Efficiency", n_bins_w, x_min_w, x_max_w);*/

    int mu_filled = 0, piplus_filled = 0, piminus_filled = 0, kshort_filled = 0;

    // Loop over events and fill efficiencies
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_has_muon) continue;
        if (!event.mc_is_kshort_decay_pionic) continue;

        float kshrt_energy = event.mc_kshrt_total_energy;
        float kshrt_sep = event.mc_kshrt_end_sep;

        TVector3 piplus_vec(event.mc_kshrt_piplus_px, event.mc_kshrt_piplus_py, event.mc_kshrt_piplus_pz);
        TVector3 piminus_vec(event.mc_kshrt_piminus_px, event.mc_kshrt_piminus_py, event.mc_kshrt_piminus_pz);
        float angle_deg = piplus_vec.Angle(piminus_vec); 

        // Muon reconstruction analysis
        if (event.mc_muon_tid) {
            float energy = event.mc_muon_energy;
            float momentum = std::sqrt(event.mc_muon_px * event.mc_muon_px +
                                       event.mc_muon_py * event.mc_muon_py +
                                       event.mc_muon_pz * event.mc_muon_pz);
            bool found_candidate = false;
            if (event.pfp_muon_purity && event.pfp_muon_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_muon_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_muon_purity->at(pfp_idx);
                    float completeness = event.pfp_muon_completeness->at(pfp_idx);

                    if (purity >= 0.5 && completeness >= 0.1) {
                        mu_effic->Fill(true, energy);
                        mu_mom_effic->Fill(true, momentum);
                        mu_mom_kshrt_energy->Fill(true, kshrt_energy);
                        mu_mom_kshrt_sep->Fill(true, kshrt_sep);
                        mu_mom_open_angle->Fill(true, angle_deg);
                        mu_filled++;
                        found_candidate = true;
                        break;
                    }
                }
            }
            if (!found_candidate) {
                mu_effic->Fill(false, energy);
                mu_mom_effic->Fill(false, momentum);
                mu_mom_kshrt_energy->Fill(false, kshrt_energy);
                mu_mom_kshrt_sep->Fill(false, kshrt_sep);
                mu_mom_open_angle->Fill(false, angle_deg);
            }
        }

        // Pion-plus reconstruction analysis
        if (event.mc_kshrt_piplus_tid) {
            float energy = event.mc_kshrt_piplus_energy;
            float momentum = std::sqrt(event.mc_kshrt_piplus_px * event.mc_kshrt_piplus_px +
                                       event.mc_kshrt_piplus_py * event.mc_kshrt_piplus_py +
                                       event.mc_kshrt_piplus_pz * event.mc_kshrt_piplus_pz);
            bool found_candidate = false;

            if (event.pfp_piplus_purity && event.pfp_piplus_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_piplus_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_piplus_purity->at(pfp_idx);
                    float completeness = event.pfp_piplus_completeness->at(pfp_idx);

                    if (purity >= 0.5 && completeness >= 0.1) {
                        piplus_effic->Fill(true, energy);
                        piplus_mom_effic->Fill(true, momentum);
                        piplus_mom_kshrt_energy->Fill(true, kshrt_energy);
                        piplus_mom_kshrt_sep->Fill(true, kshrt_sep);
                        piplus_mom_open_angle->Fill(true, angle_deg);
                        piplus_filled++;
                        found_candidate = true;
                        break;
                    }
                }
            }
            if (!found_candidate) {
                piplus_effic->Fill(false, energy);
                piplus_mom_effic->Fill(false, momentum);
                piplus_mom_kshrt_energy->Fill(false, kshrt_energy);
                piplus_mom_kshrt_sep->Fill(false, kshrt_sep);
                piplus_mom_open_angle->Fill(false, angle_deg);
            }
        }

        // Pion-minus reconstruction analysis
        if (event.mc_kshrt_piminus_tid) {
            float energy = event.mc_kshrt_piminus_energy;
            float momentum = std::sqrt(event.mc_kshrt_piplus_px * event.mc_kshrt_piplus_px +
                                       event.mc_kshrt_piplus_py * event.mc_kshrt_piplus_py +
                                       event.mc_kshrt_piplus_pz * event.mc_kshrt_piplus_pz);
            bool found_candidate = false;
            if (event.pfp_piminus_purity && event.pfp_piminus_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_piminus_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_piminus_purity->at(pfp_idx);
                    float completeness = event.pfp_piminus_completeness->at(pfp_idx);

                    if (purity >= 0.5 && completeness >= 0.1) {
                        piminus_effic->Fill(true, energy);
                        piminus_mom_effic->Fill(true, momentum);
                        piminus_mom_kshrt_energy->Fill(true, kshrt_energy);
                        piminus_mom_kshrt_sep->Fill(true, kshrt_sep);
                        piminus_mom_open_angle->Fill(true, angle_deg);
                        piminus_filled++;
                        found_candidate = true;
                        break;
                    }
                }
            }
            if (!found_candidate) {
                piminus_effic->Fill(false, energy);
                piminus_mom_effic->Fill(false, momentum);
                piminus_mom_kshrt_energy->Fill(false, kshrt_energy);
                piminus_mom_kshrt_sep->Fill(false, kshrt_sep);
                piminus_mom_open_angle->Fill(false, angle_deg);
            }
        }
    }

    // Debugging: Print out how many entries were filled
    std::cout << "Muon entries filled: " << mu_filled << std::endl;
    std::cout << "Pion-plus entries filled: " << piplus_filled << std::endl;
    std::cout << "Pion-minus entries filled: " << piminus_filled << std::endl;
    std::cout << "Kaon-short entries filled: " << kshort_filled << std::endl;

    // Call the plotting function with appropriate axis label, output file name, and dynamic x-axis range
    plot_efficiency(mu_effic, piplus_effic, piminus_effic, "True Energy [GeV]", "EfficiencyPlot_full.pdf", 0.1, 3.0);
    plot_efficiency(mu_mom_effic, piplus_mom_effic, piminus_mom_effic, "True Momentum [GeV/c]", "MomentumEfficiencyPlot.pdf", 0.1, 3.0);

    plot_efficiency(mu_mom_kshrt_energy, piplus_mom_kshrt_energy, piminus_mom_kshrt_energy, "True Kaon-Short Energy [GeV]", "KaonShortEnergyEfficiencyPlot.pdf", 0.1, 3.0);
    plot_efficiency(mu_mom_kshrt_sep, piplus_mom_kshrt_sep, piminus_mom_kshrt_sep, "True Kaon-Short Decay Distance []", "KaonShortDecayDistanceEfficiencyPlot.pdf", 0.01, 20.0);
    plot_efficiency(mu_mom_open_angle, piplus_mom_open_angle, piminus_mom_open_angle, "True Decay Opening Angle []", "KaonShortOpeningAngleEfficiencyPlot.pdf", 0., 3.14);

    // Cleanup
    delete mu_effic;
    delete piplus_effic;
    delete piminus_effic;
    delete mu_mom_effic;
    delete piplus_mom_effic;
    delete piminus_mom_effic;
}
