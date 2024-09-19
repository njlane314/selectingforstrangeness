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

void pion_analyser() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/analysis_prod_strange_resample_fhc_run2_fhc_reco2_reco2.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const DisplayAssembler& display_assembler = DisplayAssembler::instance(input_file);

    int num_events = event_assembler.get_num_events();
    int total_piplus = 0, total_piminus = 0, total_muons = 0;

    TH2D *h2_piplus_purity_completeness = new TH2D("h2_piplus_purity_completeness", "", 20, 0, 1, 20, 0, 1);
    TH2D *h2_piminus_purity_completeness = new TH2D("h2_piminus_purity_completeness", "", 20, 0, 1, 20, 0, 1);
    TH2D *h2_muon_purity_completeness = new TH2D("h2_muon_purity_completeness", "", 20, 0, 1, 20, 0, 1);

    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_has_muon) continue;
        if (!event.mc_is_kshort_decay_pionic) continue;

        for (size_t j = 0; j < event.pfp_piplus_purity->size(); ++j) {
            float piplus_purity = event.pfp_piplus_purity->at(j);
            float piplus_completeness = event.pfp_piplus_completeness->at(j);
            h2_piplus_purity_completeness->Fill(piplus_purity, piplus_completeness);
            total_piplus++;
        }

        for (size_t j = 0; j < event.pfp_piminus_purity->size(); ++j) {
            float piminus_purity = event.pfp_piminus_purity->at(j);
            float piminus_completeness = event.pfp_piminus_completeness->at(j);
            h2_piminus_purity_completeness->Fill(piminus_purity, piminus_completeness);
            total_piminus++;
        }

        for (size_t j = 0; j < event.pfp_muon_purity->size(); ++j) {
            float muon_purity = event.pfp_muon_purity->at(j);
            float muon_completeness = event.pfp_muon_completeness->at(j);
            h2_muon_purity_completeness->Fill(muon_purity, muon_completeness);
            total_muons++;
        }
    }

    TCanvas* c_piplus_purity_completeness = new TCanvas("c_piplus_purity_completeness", "Pi+ Purity vs Completeness", 800, 600);
    h2_piplus_purity_completeness->GetXaxis()->SetTitle("True Pi+ Purity");
    h2_piplus_purity_completeness->GetYaxis()->SetTitle("True Pi+ Completeness");
    h2_piplus_purity_completeness->Draw("COLZ");
    c_piplus_purity_completeness->SaveAs("./plots/piplus_purity_completeness.pdf");

    TCanvas* c_piminus_purity_completeness = new TCanvas("c_piminus_purity_completeness", "Pi- Purity vs Completeness", 800, 600);
    h2_piminus_purity_completeness->GetXaxis()->SetTitle("True Pi- Purity");
    h2_piminus_purity_completeness->GetYaxis()->SetTitle("True Pi- Completeness");
    h2_piminus_purity_completeness->Draw("COLZ");
    c_piminus_purity_completeness->SaveAs("./plots/piminus_purity_completeness.pdf");

    TCanvas* c_muon_purity_completeness = new TCanvas("c_muon_purity_completeness", "Muon Purity vs Completeness", 800, 600);
    h2_muon_purity_completeness->GetXaxis()->SetTitle("True Muon Purity");
    h2_muon_purity_completeness->GetYaxis()->SetTitle("True Muon Completeness");
    h2_muon_purity_completeness->Draw("COLZ");
    c_muon_purity_completeness->SaveAs("./plots/muon_purity_completeness.pdf");

    delete c_piplus_purity_completeness;
    delete h2_piplus_purity_completeness;
    delete c_piminus_purity_completeness;
    delete h2_piminus_purity_completeness;
    delete c_muon_purity_completeness;
    delete h2_muon_purity_completeness;

    std::cout << "Total number of pi+ analysed: " << total_piplus << std::endl;
    std::cout << "Total number of pi- analysed: " << total_piminus << std::endl;
    std::cout << "Total number of muons analysed: " << total_muons << std::endl;
}