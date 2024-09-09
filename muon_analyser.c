#include "AnalysisEvent.h"
#include "EventAssembler.h"
#include "SliceAssembler.h"
#include "PlotFunctions.h"
#include "DisplayAssembler.h"

#include "TH2D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TBox.h"
#include "TLatex.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <random>

void muon_analyser() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/analysis_prod_strange_resample_fhc_run2_fhc_reco2_reco2.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const DisplayAssembler& display_assembler = DisplayAssembler::instance(input_file);

    int num_events = event_assembler.get_num_events();
    int total_muons = 0;
    int muons_in_region = 0;  // To count muons in the (purity > 0.8, completeness > 0.8) region
    
    // Create the TH2D histogram to store purity vs. completeness
    TH2D *h2_purity_completeness = new TH2D("h2_purity_completeness", "", 
                                            20, 0, 1, 20, 0, 1); // 10x10 bins, range 0-1 for both axes

    // Iterate over events
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_has_muon) continue;
        if (!event.mc_is_kshort_decay_pionic) continue;

        unsigned int muon_tid = event.mc_muon_tid;

        int max_hits = -1;
        size_t best_match_index = -1;

        // Find the backtracked particle with the most hits
        for (size_t j = 0; j < event.backtracked_tid->size(); ++j) {
            if (event.backtracked_tid->at(j) == muon_tid) {
                int num_hits = event.pfnhits->at(j);
                std::cout << event.backtracked_pdg->at(j) << std::endl;

                if (num_hits > max_hits) {
                    max_hits = num_hits;
                    best_match_index = j;
                }
            }
        }

        if (best_match_index != -1 && max_hits > 50) {
            float muon_purity = event.backtracked_purity->at(best_match_index);
            float muon_completeness = event.backtracked_completeness->at(best_match_index);

            h2_purity_completeness->Fill(muon_purity, muon_completeness);
            total_muons++;

            // Check if the muon is in the desired region
            if (muon_purity > 0.75 && muon_completeness > 0.9) {
                muons_in_region++;
            }
        }
        else
        {
            h2_purity_completeness->Fill(0.0, 0.0);
            total_muons++;
        }
    }

    // Calculate the fraction of muons in the region
    double fraction_in_region = (total_muons > 0) ? double(muons_in_region) / total_muons : 0.0;

    // Create a canvas for plotting the TH2D histogram
    TCanvas* c_purity_completeness = new TCanvas("c_purity_completeness", "Purity vs Completeness", 800, 600);
    
    gStyle->SetOptStat(0);

    // Draw the TH2D histogram
    h2_purity_completeness->GetXaxis()->SetTitle("True Muon Purity");
    h2_purity_completeness->GetYaxis()->SetTitle("True Muon Completeness");
    h2_purity_completeness->Draw("COLZ");

    // Add the red box around the region where completeness > 0.8 and purity > 0.8
    TBox *box = new TBox(0.75, 0.9, 1.0, 1.0);
    box->SetLineColor(kRed);
    box->SetLineWidth(3);
    box->SetFillStyle(0); // No fill, just the outline
    box->Draw();

    // Add text to display the fraction of muons in the region
    TLatex *latex = new TLatex();
    latex->SetNDC();
    latex->SetTextSize(0.04);
    latex->SetTextColor(kRed);
    latex->DrawLatex(0.6, 0.915, Form("Fraction in region: %.2f%%", fraction_in_region * 100));

    // Save the plot
    c_purity_completeness->SaveAs("./plots/muon_purity_completeness_with_box.pdf");

    // Clean up memory
    delete c_purity_completeness;
    delete h2_purity_completeness;
    delete box;
    delete latex;

    std::cout << "Fraction of muons in the (purity > 0.8, completeness > 0.8) region: " 
              << fraction_in_region * 100 << "%" << std::endl;
}
