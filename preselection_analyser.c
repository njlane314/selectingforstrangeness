#include "AnalysisEvent.h"
#include "EventAssembler.h"
#include "SliceAssembler.h"
#include "PlotFunctions.h"

#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TLegend.h"
#include <iostream>
#include <cassert>

void preselection_analyser()
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/analysis_prod_strange_resample_fhc_run2_fhc_reco2_reco2.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const SliceAssembler& slice_assembler = SliceAssembler::instance(input_file);

    assert(event_assembler.get_num_events() == slice_assembler.get_num_events());

    int num_events = event_assembler.get_num_events();

    // Create histograms for completeness, purity, and other properties
    TH1D* h_true_completeness = new TH1D("h_true_completeness", ";Completeness;Entries", 10, 0, 1);
    TH1D* h_pandora_completeness = new TH1D("h_pandora_completeness", ";Completeness;Entries", 10, 0, 1);
    TH1D* h_flash_completeness = new TH1D("h_flash_completeness", ";Completeness;Entries", 10, 0, 1);
    TH1D* h_cosmic_completeness = new TH1D("h_cosmic_completeness", ";Completeness;Entries", 10, 0, 1);

    TH1D* h_true_purity = new TH1D("h_true_purity", ";Purity;Entries", 10, 0, 1);
    TH1D* h_pandora_purity = new TH1D("h_pandora_purity", ";Purity;Entries", 10, 0, 1);
    TH1D* h_flash_purity = new TH1D("h_flash_purity", ";Purity;Entries", 10, 0, 1);
    TH1D* h_cosmic_purity = new TH1D("h_cosmic_purity", ";Purity;Entries", 10, 0, 1);

    TH1D* h_true_topo = new TH1D("h_true_topo", ";Topological Score;Entries", 10, 0, 1);
    TH1D* h_pandora_topo = new TH1D("h_pandora_topo", ";Topological Score;Entries", 10, 0, 1);
    TH1D* h_flash_topo = new TH1D("h_flash_topo", ";Topological Score;Entries", 10, 0, 1);
    TH1D* h_cosmic_topo = new TH1D("h_cosmic_topo", ";Topological Score;Entries", 10, 0, 1);

    TH1D* h_true_charge = new TH1D("h_true_charge", ";Charge;Entries", 30, 0, 200000);
    TH1D* h_pandora_charge = new TH1D("h_pandora_charge", ";Charge;Entries", 30, 0, 200000);
    TH1D* h_flash_charge = new TH1D("h_flash_charge", ";Charge;Entries", 30, 0, 200000);
    TH1D* h_cosmic_charge = new TH1D("h_cosmic_charge", ";Charge;Entries", 30, 0, 200000);

    TH1D* h_true_center_x = new TH1D("h_true_center_x", ";Center X;Entries", 40, -400, 400);
    TH1D* h_pandora_center_x = new TH1D("h_pandora_center_x", ";Center X;Entries", 40, -400, 400);
    TH1D* h_flash_center_x = new TH1D("h_flash_center_x", ";Center X;Entries", 40, -400, 400);
    TH1D* h_cosmic_center_x = new TH1D("h_cosmic_center_x", ";Center X;Entries", 40, -400, 400);

    TH1D* h_true_center_y = new TH1D("h_true_center_y", ";Center Y;Entries", 40, -200, 200);
    TH1D* h_pandora_center_y = new TH1D("h_pandora_center_y", ";Center Y;Entries", 40, -200, 200);
    TH1D* h_flash_center_y = new TH1D("h_flash_center_y", ";Center Y;Entries", 40, -200, 200);
    TH1D* h_cosmic_center_y = new TH1D("h_cosmic_center_y", ";Center Y;Entries", 40, -200, 200);

    TH1D* h_true_center_z = new TH1D("h_true_center_z", ";Center Z;Entries", 50, -1500, 1500);
    TH1D* h_pandora_center_z = new TH1D("h_pandora_center_z", ";Center Z;Entries", 50, -1500, 1500);
    TH1D* h_flash_center_z = new TH1D("h_flash_center_z", ";Center Z;Entries", 50, -1500, 1500);
    TH1D* h_cosmic_center_z = new TH1D("h_cosmic_center_z", ";Center Z;Entries", 50, -1500, 1500);

    TH1D* h_true_n_hits = new TH1D("h_true_n_hits", ";Num Hits;Entries", 30, 0, 6000);
    TH1D* h_pandora_n_hits = new TH1D("h_pandora_n_hits", ";Num Hits;Entries", 30, 0, 6000);
    TH1D* h_flash_n_hits = new TH1D("h_flash_n_hits", ";Num Hits;Entries", 30, 0, 6000);
    TH1D* h_cosmic_n_hits = new TH1D("h_cosmic_n_hits", ";Num Hits;Entries", 30, 0, 6000);

    TH2D* h_separation_vs_completeness = new TH2D("h_separation_vs_completeness", ";Spatial Separation;Completeness", 50, 0, 500, 50, 0, 1);
    TH2D* h_separation_vs_purity = new TH2D("h_separation_vs_purity", ";Spatial Separation;Purity", 50, 0, 500, 50, 0, 1);

    int sig_count = 0;
    for (int i = 0; i < num_events; ++i)
    {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_is_kshort_decay_pionic) continue;
        sig_count += 1;

        auto [true_slice, pandora_slice, flash_slice] = slice_assembler.get_defined_slices(i);
        const auto& slices = slice_assembler.get_slices(i);

        // Fill histograms for completeness, purity, topological score, charge, and other properties
        if (!true_slice.empty()) {
            if (true_slice["center_x"] == 0.0 && true_slice["center_y"] == 0.0 && true_slice["center_z"] == 0.0) {
                continue;  // Skip if all center properties are zero
            }
            h_true_completeness->Fill(true_slice["completeness"]);
            h_true_purity->Fill(true_slice["purity"]);
            h_true_topo->Fill(true_slice["topological_score"]);
            h_true_charge->Fill(true_slice["charge"]);
            h_true_center_x->Fill(true_slice["center_x"]);
            h_true_center_y->Fill(true_slice["center_y"]);
            h_true_center_z->Fill(true_slice["center_z"]);
            h_true_n_hits->Fill(true_slice["n_hits"]);
        }
        if (!pandora_slice.empty()) {
            if (pandora_slice["center_x"] == 0.0 && pandora_slice["center_y"] == 0.0 && pandora_slice["center_z"] == 0.0) {
                continue;  // Skip if all center properties are zero
            }
            h_pandora_completeness->Fill(pandora_slice["completeness"]);
            h_pandora_purity->Fill(pandora_slice["purity"]);
            h_pandora_topo->Fill(pandora_slice["topological_score"]);
            h_pandora_charge->Fill(pandora_slice["charge"]);
            h_pandora_center_x->Fill(pandora_slice["center_x"]);
            h_pandora_center_y->Fill(pandora_slice["center_y"]);
            h_pandora_center_z->Fill(pandora_slice["center_z"]);
            h_pandora_n_hits->Fill(pandora_slice["n_hits"]);
        }
        if (!flash_slice.empty()) {
            if (flash_slice["center_x"] == 0.0 && flash_slice["center_y"] == 0.0 && flash_slice["center_z"] == 0.0) {
                continue;  // Skip if all center properties are zero
            }
            h_flash_completeness->Fill(flash_slice["completeness"]);
            h_flash_purity->Fill(flash_slice["purity"]);
            h_flash_topo->Fill(flash_slice["topological_score"]);
            h_flash_charge->Fill(flash_slice["charge"]);
            h_flash_center_x->Fill(flash_slice["center_x"]);
            h_flash_center_y->Fill(flash_slice["center_y"]);
            h_flash_center_z->Fill(flash_slice["center_z"]);
            h_flash_n_hits->Fill(flash_slice["n_hits"]);
        }

        // Fill cosmic slices (slices that are not true, pandora, or flash)
        for (const auto& [slice_id, slice_properties] : slices) {
            if (slice_id != slice_assembler.get_true_slice_id() &&
                slice_id != slice_assembler.get_pandora_slice_id() &&
                slice_id != slice_assembler.get_flash_slice_id()) 
            {
                if (slice_properties.at("center_x") == 0.0 && slice_properties.at("center_y") == 0.0 && slice_properties.at("center_z") == 0.0) {
                    continue;  // Skip if all center properties are zero
                }
                h_cosmic_completeness->Fill(slice_properties.at("completeness"));
                h_cosmic_purity->Fill(slice_properties.at("purity"));
                h_cosmic_topo->Fill(slice_properties.at("topological_score"));
                h_cosmic_charge->Fill(slice_properties.at("charge"));
                h_cosmic_center_x->Fill(slice_properties.at("center_x"));
                h_cosmic_center_y->Fill(slice_properties.at("center_y"));
                h_cosmic_center_z->Fill(slice_properties.at("center_z"));
                h_cosmic_n_hits->Fill(slice_properties.at("n_hits"));
            }
        }

        double true_center_x = true_slice["center_x"];
        double true_center_y = true_slice["center_y"];
        double true_center_z = true_slice["center_z"];

        for (const auto& [slice_id, slice_properties] : slices)
        {
            if (slice_id != slice_assembler.get_true_slice_id())
            {
                double cosmic_center_x = slice_properties.at("center_x");
                double cosmic_center_y = slice_properties.at("center_y");
                double cosmic_center_z = slice_properties.at("center_z");

                double separation = sqrt(pow(true_center_x - cosmic_center_x, 2) +
                                         pow(true_center_y - cosmic_center_y, 2) +
                                         pow(true_center_z - cosmic_center_z, 2));

                double completeness = slice_properties.at("completeness");
                double purity = slice_properties.at("purity");

                if (completeness == 0.0 || purity == 0.0)
                    continue;

                h_separation_vs_completeness->Fill(separation, completeness);
                h_separation_vs_purity->Fill(separation, purity);
            }
        }
    }

    std::cout << "Signal events: " << sig_count << std::endl;

    // Define a function to draw the plots to avoid repetition
    auto draw_plot = [](TCanvas* canvas, TH1D* h_true, TH1D* h_pandora, TH1D* h_flash, TH1D* h_cosmic, const std::string& save_name, const std::string& title) {
        canvas->SetLogy();
        h_true->SetLineColor(kBlack);          // True data: Black for strong contrast
        h_pandora->SetLineColor(kBlue);        // Pandora reconstruction: Clean blue
        h_flash->SetLineColor(kRed);           // Flash timing: Standard red for visibility
        h_cosmic->SetLineColor(kGreen+2);   

        h_true->SetLineWidth(2);
        h_pandora->SetLineWidth(2);
        h_flash->SetLineWidth(2);
        h_cosmic->SetLineWidth(2);

        h_true->SetStats(0);
        h_pandora->SetStats(0);
        h_flash->SetStats(0);
        h_cosmic->SetStats(0);

        double max_val = std::max({h_true->GetMaximum(), h_pandora->GetMaximum(), h_flash->GetMaximum(), h_cosmic->GetMaximum()});
        h_true->SetMaximum(1.1 * max_val);

        h_true->Draw("HIST E");
        h_pandora->Draw("HIST E SAME");
        h_flash->Draw("HIST E SAME");
        h_cosmic->Draw("HIST E SAME");

        TLegend* legend = new TLegend(0.1, 0.91, 0.9, 0.99); // Adjusted legend position to not overlap with the plot
        legend->SetNColumns(4); // Adjusted for cosmic slices
        legend->SetBorderSize(0);
        legend->AddEntry(h_true, "True Slice", "l");
        legend->AddEntry(h_pandora, "Pandora Slice", "l");
        legend->AddEntry(h_flash, "Flash Slice", "l");
        legend->AddEntry(h_cosmic, "Cosmic Slice", "l"); // Add cosmic slices to legend
        legend->Draw();

        canvas->SaveAs(("./plots/" + save_name + ".pdf").c_str());
    };

    // Draw plots for completeness, purity, and additional properties
    TCanvas* c1 = new TCanvas("c1", "Completeness Comparison", 800, 600);
    draw_plot(c1, h_true_completeness, h_pandora_completeness, h_flash_completeness, h_cosmic_completeness, "slice_completeness_comparison", "Completeness Comparison");

    TCanvas* c2 = new TCanvas("c2", "Purity Comparison", 800, 600);
    draw_plot(c2, h_true_purity, h_pandora_purity, h_flash_purity, h_cosmic_purity, "slice_purity_comparison", "Purity Comparison");

    TCanvas* c3 = new TCanvas("c3", "Topological Score Comparison", 800, 600);
    draw_plot(c3, h_true_topo, h_pandora_topo, h_flash_topo, h_cosmic_topo, "slice_topological_score_comparison", "Topological Score Comparison");

    TCanvas* c4 = new TCanvas("c4", "Charge Comparison", 800, 600);
    draw_plot(c4, h_true_charge, h_pandora_charge, h_flash_charge, h_cosmic_charge, "slice_charge_comparison", "Charge Comparison");

    TCanvas* c5 = new TCanvas("c5", "Center X Comparison", 800, 600);
    draw_plot(c5, h_true_center_x, h_pandora_center_x, h_flash_center_x, h_cosmic_center_x, "slice_center_x_comparison", "Center X Comparison");

    TCanvas* c6 = new TCanvas("c6", "Center Y Comparison", 800, 600);
    draw_plot(c6, h_true_center_y, h_pandora_center_y, h_flash_center_y, h_cosmic_center_y, "slice_center_y_comparison", "Center Y Comparison");

    TCanvas* c7 = new TCanvas("c7", "Center Z Comparison", 800, 600);
    draw_plot(c7, h_true_center_z, h_pandora_center_z, h_flash_center_z, h_cosmic_center_z, "slice_center_z_comparison", "Center Z Comparison");

    TCanvas* c8 = new TCanvas("c8", "Num Hits Comparison", 800, 600);
    draw_plot(c8, h_true_n_hits, h_pandora_n_hits, h_flash_n_hits, h_cosmic_n_hits, "slice_n_hits_comparison", "Num Hits Comparison");

    TCanvas* c9 = new TCanvas("c9", "", 800, 600);
    h_separation_vs_completeness->Draw("COLZ");
    c9->SaveAs("./plots/separation_vs_completeness_2D.pdf");

    TCanvas* c10 = new TCanvas("c10", "", 800, 600);
    h_separation_vs_purity->Draw("COLZ");
    c10->SaveAs("./plots/separation_vs_purity_2D.pdf");

    // Cleanup
    delete h_separation_vs_completeness;
    delete h_separation_vs_purity;
    delete c9;
    delete c10;

    // Cleanup
    delete h_true_completeness;
    delete h_pandora_completeness;
    delete h_flash_completeness;
    delete h_cosmic_completeness;
    delete h_true_purity;
    delete h_pandora_purity;
    delete h_flash_purity;
    delete h_cosmic_purity;
    delete h_true_topo;
    delete h_pandora_topo;
    delete h_flash_topo;
    delete h_cosmic_topo;
    delete h_true_charge;
    delete h_pandora_charge;
    delete h_flash_charge;
    delete h_cosmic_charge;
    delete h_true_center_x;
    delete h_pandora_center_x;
    delete h_flash_center_x;
    delete h_cosmic_center_x;
    delete h_true_center_y;
    delete h_pandora_center_y;
    delete h_flash_center_y;
    delete h_cosmic_center_y;
    delete h_true_center_z;
    delete h_pandora_center_z;
    delete h_flash_center_z;
    delete h_cosmic_center_z;
    delete h_true_n_hits;
    delete h_pandora_n_hits;
    delete h_flash_n_hits;
    delete h_cosmic_n_hits;
    delete c1;
    delete c2;
    delete c3;
    delete c4;
    delete c5;
    delete c6;
    delete c7;
    delete c8;
}
