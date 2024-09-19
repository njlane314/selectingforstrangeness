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

void reco_effic_analyser() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/prod_strange_resample_fhc_run2_fhc_reco2_reco2_signalfilter_100_analysis.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    int num_events = event_assembler.get_num_events();

    // Truth variable binning (energy bins)
    int n_bins = 20;
    double energy_min = 0.1;  // Log-scale x-axis, avoid zero
    double energy_max = 10.0; // adjust depending on your energy range

    // Create histograms for muon, pion-plus, and pion-minus
    TH1D* h_muon_total = new TH1D("h_muon_total", "Muon Total;True Energy [GeV];Count", n_bins, energy_min, energy_max);
    TH1D* h_muon_passed = new TH1D("h_muon_passed", "Muon Passed;True Energy [GeV];Count", n_bins, energy_min, energy_max);

    TH1D* h_piplus_total = new TH1D("h_piplus_total", "Pion-Plus Total;True Energy [GeV];Count", n_bins, energy_min, energy_max);
    TH1D* h_piplus_passed = new TH1D("h_piplus_passed", "Pion-Plus Passed;True Energy [GeV];Count", n_bins, energy_min, energy_max);

    TH1D* h_piminus_total = new TH1D("h_piminus_total", "Pion-Minus Total;True Energy [GeV];Count", n_bins, energy_min, energy_max);
    TH1D* h_piminus_passed = new TH1D("h_piminus_passed", "Pion-Minus Passed;True Energy [GeV];Count", n_bins, energy_min, energy_max);

    // Loop over events to fill histograms
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_has_muon) continue;
        if (!event.mc_is_kshort_decay_pionic) continue;

        // Muon reconstruction analysis
        if (event.mc_has_muon) {
            float muon_energy = event.mc_muon_energy;
            h_muon_total->Fill(muon_energy);

            if (event.pfp_muon_purity && event.pfp_muon_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_muon_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_muon_purity->at(pfp_idx);
                    float completeness = event.pfp_muon_completeness->at(pfp_idx);

                    if (purity >= 0.5 && completeness >= 0.1) {
                        h_muon_passed->Fill(muon_energy);
                        break;
                    }
                }
            }
        }

        // Pion-plus reconstruction analysis
        if (event.mc_kshrt_piplus_tid != 0) {
            float piplus_energy = event.mc_kshrt_piplus_energy;
            h_piplus_total->Fill(piplus_energy);

            if (event.pfp_piplus_purity && event.pfp_piplus_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_piplus_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_piplus_purity->at(pfp_idx);
                    float completeness = event.pfp_piplus_completeness->at(pfp_idx);

                    if (purity >= 0.5 && completeness >= 0.1) {
                        h_piplus_passed->Fill(piplus_energy);
                        break;
                    }
                }
            }
        }

        // Pion-minus reconstruction analysis
        if (event.mc_kshrt_piminus_tid != 0) {
            float piminus_energy = event.mc_kshrt_piminus_energy;
            h_piminus_total->Fill(piminus_energy);

            if (event.pfp_piminus_purity && event.pfp_piminus_completeness) {
                for (size_t pfp_idx = 0; pfp_idx < event.pfp_piminus_purity->size(); ++pfp_idx) {
                    float purity = event.pfp_piminus_purity->at(pfp_idx);
                    float completeness = event.pfp_piminus_completeness->at(pfp_idx);

                    if (purity >= 0.5 && completeness >= 0.1) {
                        h_piminus_passed->Fill(piminus_energy);
                        break;
                    }
                }
            }
        }
    }

    // Create efficiency objects for muon, pion-plus, and pion-minus
    TEfficiency* muon_efficiency = new TEfficiency(*h_muon_passed, *h_muon_total);
    TEfficiency* piplus_efficiency = new TEfficiency(*h_piplus_passed, *h_piplus_total);
    TEfficiency* piminus_efficiency = new TEfficiency(*h_piminus_passed, *h_piminus_total);

    // Create canvas
    TCanvas* c_efficiency = new TCanvas("c_efficiency", "Reconstruction Efficiency", 800, 600);

    TLegend *l = new TLegend(0.1,0.0,0.9,1.0);
    l->SetBorderSize(0);
    l->SetNColumns(2);

    c_efficiency->cd();

    // Set log scale for the x-axis
    c_efficiency->SetLogx();

    // Style the efficiency plots
    muon_efficiency->SetLineColor(kBlue);
    muon_efficiency->SetMarkerColor(kBlue);
    muon_efficiency->SetMarkerStyle(20);

    piplus_efficiency->SetLineColor(kRed);
    piplus_efficiency->SetMarkerColor(kRed);
    piplus_efficiency->SetMarkerStyle(21);

    piminus_efficiency->SetLineColor(kGreen + 2);
    piminus_efficiency->SetMarkerColor(kGreen + 2);
    piminus_efficiency->SetMarkerStyle(22);

    // Draw efficiency plots
    muon_efficiency->Draw("AP");
    piplus_efficiency->Draw("P same");
    piminus_efficiency->Draw("P same");

    // Add legend
    l->AddEntry(muon_efficiency, "Muon Efficiency", "P");
    l->AddEntry(piplus_efficiency, "Pion-Plus Efficiency", "P");
    l->AddEntry(piminus_efficiency, "Pion-Minus Efficiency", "P");
    l->Draw();



    // Save plot
    c_efficiency->SaveAs("EfficiencyPlot_full.pdf");

    // Cleanup
    delete h_muon_total;
    delete h_muon_passed;
    delete h_piplus_total;
    delete h_piplus_passed;
    delete h_piminus_total;
    delete h_piminus_passed;
    delete muon_efficiency;
    delete piplus_efficiency;
    delete piminus_efficiency;
    delete c_efficiency;
}
