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


double calculate_mean(const std::vector<double>& values) {
    double sum = 0.0;
    for (double val : values) {
        sum += val;
    }
    return (values.size() > 0) ? sum / values.size() : 0.0;
}

double calculate_stddev(const std::vector<double>& values, double mean) {
    double sum_sq_diff = 0.0;
    for (double val : values) {
        sum_sq_diff += pow(val - mean, 2);
    }
    return (values.size() > 1) ? sqrt(sum_sq_diff / (values.size() - 1)) : 0.0;
}

void bootstrap_resample(const std::vector<double>& original_data, std::vector<double>& resampled_data, std::mt19937& rng) {
    std::uniform_int_distribution<size_t> dist(0, original_data.size() - 1);
    resampled_data.clear();
    for (size_t i = 0; i < original_data.size(); ++i) {
        resampled_data.push_back(original_data[dist(rng)]);
    }
}

void reconstruction_analyser() 
{
    const int num_bootstrap_samples = 1000; // Number of bootstrap resamples
    std::mt19937 rng;  // Random number generator
    rng.seed(std::random_device{}());

    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/analysis_prod_strange_resample_fhc_run2_fhc_reco2_reco2.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const DisplayAssembler& display_assembler = DisplayAssembler::instance(input_file);

    int total_true_muons = 0;
    int num_events = event_assembler.get_num_events();
    /*for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_has_muon) continue;
        if (!event.mc_is_kshort_decay_pionic) continue;
        total_true_muons++;
    }

    // Vectors to store values for plotting
    std::vector<double> hit_cut_values;
    std::vector<double> avg_purity_values, avg_completeness_values, efficiency_values;
    std::vector<double> purity_errors, completeness_errors, efficiency_errors;
    std::vector<double> hit_cut_widths; // To store x-axis error bars (bin widths)

    // Iterate over hit cuts and compute metrics
    int hit_cut_step = 10;
    for (int hit_cut = 0; hit_cut <= 500; hit_cut += hit_cut_step) {
        double total_purity = 0.0;
        double total_completeness = 0.0;
        int selected_muons = 0;

        std::vector<double> purity_values;
        std::vector<double> completeness_values;

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

                    if (num_hits > max_hits) {
                        max_hits = num_hits;
                        best_match_index = j;
                    }
                }
            }

            if (best_match_index != -1 && max_hits > hit_cut) {
                float muon_purity = event.backtracked_purity->at(best_match_index);
                float muon_completeness = event.backtracked_completeness->at(best_match_index);

                total_purity += muon_purity;
                total_completeness += muon_completeness;
                selected_muons++;

                purity_values.push_back(muon_purity);
                completeness_values.push_back(muon_completeness);
            }
        }

        // Calculate average metrics for this hit cut
        double avg_purity = (selected_muons > 0) ? total_purity / selected_muons : 0.0;
        double avg_completeness = (selected_muons > 0) ? total_completeness / selected_muons : 0.0;
        double efficiency = (total_true_muons > 0) ? double(selected_muons) / total_true_muons : 0.0;

        // Calculate the binomial error for efficiency
        double efficiency_error = (total_true_muons > 0) ? sqrt(efficiency * (1 - efficiency) / total_true_muons) : 0.0;

        // Bootstrapping for error estimation
        std::vector<double> resampled_purity_means;
        std::vector<double> resampled_completeness_means;

        for (int n = 0; n < num_bootstrap_samples; ++n) {
            std::vector<double> purity_resample, completeness_resample;

            // Resample purity and completeness values with replacement
            bootstrap_resample(purity_values, purity_resample, rng);
            bootstrap_resample(completeness_values, completeness_resample, rng);

            double resampled_purity_mean = calculate_mean(purity_resample);
            double resampled_completeness_mean = calculate_mean(completeness_resample);

            resampled_purity_means.push_back(resampled_purity_mean);
            resampled_completeness_means.push_back(resampled_completeness_mean);
        }

        // Compute the standard deviation of the resampled means (bootstrap errors)
        double purity_error = calculate_stddev(resampled_purity_means, avg_purity);
        double completeness_error = calculate_stddev(resampled_completeness_means, avg_completeness);

        // Store values for plotting
        hit_cut_values.push_back(hit_cut);
        avg_purity_values.push_back(avg_purity);
        avg_completeness_values.push_back(avg_completeness);
        efficiency_values.push_back(efficiency);

        purity_errors.push_back(purity_error);
        completeness_errors.push_back(completeness_error);
        efficiency_errors.push_back(efficiency_error);

        // Set the x-axis error bar as half the bin width
        hit_cut_widths.push_back(hit_cut_step / 2.0);
    }

    // Create TGraphErrors for purity, completeness, and efficiency with errors
    TGraphErrors* graph_purity = new TGraphErrors(hit_cut_values.size(), &hit_cut_values[0], &avg_purity_values[0], &hit_cut_widths[0], &purity_errors[0]);
    TGraphErrors* graph_completeness = new TGraphErrors(hit_cut_values.size(), &hit_cut_values[0], &avg_completeness_values[0], &hit_cut_widths[0], &completeness_errors[0]);
    TGraphErrors* graph_efficiency = new TGraphErrors(hit_cut_values.size(), &hit_cut_values[0], &efficiency_values[0], &hit_cut_widths[0], &efficiency_errors[0]);
    gStyle->SetEndErrorSize(0);

    // Customize graphs
    graph_purity->SetLineColor(kRed);
    graph_purity->SetMarkerColor(kRed);
    graph_purity->SetMarkerStyle(21);
    graph_purity->SetMarkerSize(0);
    graph_purity->SetLineWidth(1); 
    graph_purity->SetTitle("");  // Remove title

    graph_completeness->SetLineColor(kBlue);
    graph_completeness->SetMarkerColor(kBlue);
    graph_completeness->SetMarkerStyle(22);
    graph_completeness->SetMarkerSize(0);
    graph_completeness->SetLineWidth(1); 
    graph_completeness->SetTitle("");  // Remove title

    graph_efficiency->SetLineColor(kBlack);
    graph_efficiency->SetMarkerColor(kBlack);
    graph_efficiency->SetMarkerStyle(5);
    graph_efficiency->SetMarkerSize(1);
    graph_efficiency->SetLineWidth(1); 
    graph_efficiency->SetTitle("");  // Remove title

    // Create a canvas for plotting
    TCanvas* canvas = new TCanvas("canvas", "canvas", 800, 600);
    graph_purity->GetYaxis()->SetRangeUser(0, 1);
    graph_purity->GetXaxis()->SetRangeUser(0, 500);

    graph_purity->GetXaxis()->SetTitle("pfnhits Cut");
    graph_purity->GetXaxis()->SetTitleSize(0.04);
    graph_purity->GetYaxis()->SetTitle("True Muon Metric");
    graph_purity->GetYaxis()->SetTitleSize(0.04);

    // Draw graphs with error bars
    graph_purity->Draw("AP");  // Line and points with error bars
    graph_completeness->Draw("P SAME");  // Overlay completeness on the same plot
    graph_efficiency->Draw("P SAME");  // Overlay efficiency on the same plot

    // Add a legend
    TLegend* legend = new TLegend(0.1, 0.91, 0.9, 0.99);
    legend->SetNColumns(3);
    legend->SetBorderSize(0);
    legend->AddEntry(graph_purity, "Purity", "lp");
    legend->AddEntry(graph_completeness, "Completeness", "lp");
    legend->AddEntry(graph_efficiency, "Efficiency", "lp");
    legend->Draw();

    // Save the final plot
    canvas->SaveAs("./plots/muon_reconstruction_hitcut_with_binomial_errors.pdf");

    // Clean up memory
    delete canvas;
    delete graph_purity;
    delete graph_completeness;
    delete graph_efficiency;*/


    TH2D *h2_purity_completeness = new TH2D("h2_purity_completeness", "", 
                                            10, 0, 1, 10, 0, 1);

    // Iterate over events
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_has_muon) continue;
        //if (!event.mc_is_kshort_decay_pionic) continue;

        unsigned int muon_tid = event.mc_muon_tid;

        int max_hits = -1;
        size_t best_match_index = -1;

        // Find the backtracked particle with the most hits
        for (size_t j = 0; j < event.backtracked_tid->size(); ++j) {
            if (event.backtracked_tid->at(j) == muon_tid) {
                int num_hits = event.pfnhits->at(j);

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
        }
    }
    // Create a canvas for plotting the TH2D histogram
    TCanvas* c_purity_completeness = new TCanvas("c_purity_completeness", "Purity vs Completeness", 800, 600);

    gStyle->SetOptStat(0);
    
    // Draw the TH2D histogram
    h2_purity_completeness->GetXaxis()->SetTitle("Purity");
    h2_purity_completeness->GetYaxis()->SetTitle("Completeness");
    h2_purity_completeness->Draw("COLZ");

    // Save the plot
    c_purity_completeness->SaveAs("./plots/muon_purity_completeness_2D.pdf");

    // Clean up memory
    delete c_purity_completeness;
    delete h2_purity_completeness;
}
