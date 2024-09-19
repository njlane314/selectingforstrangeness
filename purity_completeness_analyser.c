#include "AnalysisEvent.h"
#include "EventAssembler.h"
#include "SliceAssembler.h"
#include "PlotFunctions.h"
#include "DisplayAssembler.h"

#include "TH2D.h"
#include "TGraph2D.h"

#include "TGraphAsymmErrors.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include <iostream>
#include <vector>
#include <cmath>

#include <random>

void purity_completeness_analyser() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/prod_strange_resample_fhc_run2_fhc_reco2_reco2_signalfilter_1000_analysis.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const DisplayAssembler& display_assembler = DisplayAssembler::instance(input_file);

    int num_events = event_assembler.get_num_events();

    // Define histograms
    TH2D *h2_muon_purity_completeness = new TH2D("h2_muon_purity_completeness", "", 
                                            20, 0, 1, 20, 0, 1);
    TH2D *h2_piplus_purity_completeness = new TH2D("h2_piplus_purity_completeness", "", 
                                            20, 0, 1, 20, 0, 1);                                       
    TH2D *h2_piminus_purity_completeness = new TH2D("h2_piminus_purity_completeness", "", 
                                            20, 0, 1, 20, 0, 1);                                       

    int total_muons = 0, total_piplus = 0, total_piminus = 0;
    int muons_in_region = 0, piplus_in_region = 0, piminus_in_region = 0; 

    // Loop over events
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);

        if (!event.mc_has_muon) continue;
        if (!event.mc_is_kshort_decay_pionic) continue;

        // Variables to store best purity and completeness (greatest Euclidean distance from the origin)
        float best_muon_purity = 0, best_muon_completeness = 0;
        float best_piplus_purity = 0, best_piplus_completeness = 0;
        float best_piminus_purity = 0, best_piminus_completeness = 0;

        float max_muon_distance = 0;
        float max_piplus_distance = 0;
        float max_piminus_distance = 0;

        // Loop over PFP candidates
        for (size_t pfp_idx = 0; pfp_idx < event.pfp_muon_purity->size(); ++pfp_idx) {
            // Muon purity and completeness
            float muon_purity = event.pfp_muon_purity->at(pfp_idx);
            float muon_completeness = event.pfp_muon_completeness->at(pfp_idx);

            float distance = std::sqrt(muon_purity * muon_purity + muon_completeness * muon_completeness);
            if (distance > max_muon_distance) {
                max_muon_distance = distance;
                best_muon_purity = muon_purity;
                best_muon_completeness = muon_completeness;
            }

            // Piplus purity and completeness
            float piplus_purity = event.pfp_piplus_purity->at(pfp_idx);
            float piplus_completeness = event.pfp_piplus_completeness->at(pfp_idx);

            distance = std::sqrt(piplus_purity * piplus_purity + piplus_completeness * piplus_completeness);
            if (distance > max_piplus_distance) {
                max_piplus_distance = distance;
                best_piplus_purity = piplus_purity;
                best_piplus_completeness = piplus_completeness;
            }

            // Piminus purity and completeness
            float piminus_purity = event.pfp_piminus_purity->at(pfp_idx);
            float piminus_completeness = event.pfp_piminus_completeness->at(pfp_idx);

            distance = std::sqrt(piminus_purity * piminus_purity + piminus_completeness * piminus_completeness);
            if (distance > max_piminus_distance) {
                max_piminus_distance = distance;
                best_piminus_purity = piminus_purity;
                best_piminus_completeness = piminus_completeness;
            }
        }

        // Fill histograms and count events in the region
        if (best_muon_completeness > 0 && best_muon_purity > 0)
            h2_muon_purity_completeness->Fill(best_muon_purity, best_muon_completeness);

        if (best_piplus_completeness > 0 && best_piplus_purity > 0)
            h2_piplus_purity_completeness->Fill(best_piplus_purity, best_piplus_completeness);

        if (best_piminus_completeness > 0 && best_piminus_purity > 0)
            h2_piminus_purity_completeness->Fill(best_piminus_purity, best_piminus_completeness);

        total_muons++;
        if (best_muon_purity > 0.5 && best_muon_completeness > 0.1) muons_in_region++;

        total_piplus++;
        if (best_piplus_purity > 0.5 && best_piplus_completeness > 0.1) piplus_in_region++;

        total_piminus++;
        if (best_piminus_purity > 0.5 && best_piminus_completeness > 0.1) piminus_in_region++;
    }

    // Calculate the fraction of events in the region
    double muon_fraction_in_region = (total_muons > 0) ? double(muons_in_region) / total_muons : 0.0;
    double piplus_fraction_in_region = (total_piplus > 0) ? double(piplus_in_region) / total_piplus : 0.0;
    double piminus_fraction_in_region = (total_piminus > 0) ? double(piminus_in_region) / total_piminus : 0.0;

    // Plotting and adding boxes and text for each particle

    // Muon plot
    TCanvas* c_muon = new TCanvas("c_muon", "", 600, 800);
    gStyle->SetOptStat(0);
    h2_muon_purity_completeness->GetXaxis()->SetTitle("True #mu Purity");
    h2_muon_purity_completeness->GetYaxis()->SetTitle("True #mu Completeness");
    h2_muon_purity_completeness->Draw("COLZ");

    // Add the red box and text
    TBox *box_muon = new TBox(0.45, 0.1, 1.0, 1.0);
    box_muon->SetLineColor(kRed);
    box_muon->SetLineWidth(3);
    box_muon->SetFillStyle(0); // No fill, just the outline
    box_muon->Draw();

    TLatex *latex_muon = new TLatex();
    latex_muon->SetNDC();
    latex_muon->SetTextSize(0.04);
    latex_muon->SetTextColor(kRed);
    latex_muon->DrawLatex(0.45, 0.915, Form("Fraction in Region: %.2f%%", muon_fraction_in_region * 100));

    c_muon->SaveAs("./plots/muon_purity_completeness_2D.pdf");

    // Piplus plot
    TCanvas* c_piplus = new TCanvas("c_piplus", "", 600, 800);
    h2_piplus_purity_completeness->GetXaxis()->SetTitle("True #pi^{+} Purity");
    h2_piplus_purity_completeness->GetYaxis()->SetTitle("True #pi^{+} Completeness");
    h2_piplus_purity_completeness->Draw("COLZ");

    TBox *box_piplus = new TBox(0.45, 0.1, 1.0, 1.0);
    box_piplus->SetLineColor(kRed);
    box_piplus->SetLineWidth(3);
    box_piplus->SetFillStyle(0); 
    box_piplus->Draw();

    TLatex *latex_piplus = new TLatex();
    latex_piplus->SetNDC();
    latex_piplus->SetTextSize(0.04);
    latex_piplus->SetTextColor(kRed);
    latex_piplus->DrawLatex(0.45, 0.915, Form("Fraction in Region: %.2f%%", piplus_fraction_in_region * 100));

    c_piplus->SaveAs("./plots/piplus_purity_completeness_2D.pdf");

    // Piminus plot
    TCanvas* c_piminus = new TCanvas("c_piminus", "", 600, 800);
    h2_piminus_purity_completeness->GetXaxis()->SetTitle("True #pi^{-} Purity");
    h2_piminus_purity_completeness->GetYaxis()->SetTitle("True #pi^{-} Completeness");
    h2_piminus_purity_completeness->Draw("COLZ");

    TBox *box_piminus = new TBox(0.45, 0.1, 1.0, 1.0);
    box_piminus->SetLineColor(kRed);
    box_piminus->SetLineWidth(3);
    box_piminus->SetFillStyle(0); 
    box_piminus->Draw();

    TLatex *latex_piminus = new TLatex();
    latex_piminus->SetNDC();
    latex_piminus->SetTextSize(0.04);
    latex_piminus->SetTextColor(kRed);
    latex_piminus->DrawLatex(0.45, 0.915, Form("Fraction in Region: %.2f%%", piminus_fraction_in_region * 100));

    c_piminus->SaveAs("./plots/piminus_purity_completeness_2D.pdf");

    // Cleanup
    delete c_muon;
    delete c_piplus;
    delete c_piminus;
    delete h2_muon_purity_completeness;
    delete h2_piplus_purity_completeness;
    delete h2_piminus_purity_completeness;
    delete box_muon;
    delete box_piplus;
    delete box_piminus;
    delete latex_muon;
    delete latex_piplus;
    delete latex_piminus;
}
