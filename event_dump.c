#include "EventAssembler.h"
#include "PlotUtils.h"
#include "TH1D.h"
#include "TFile.h"
#include "TTree.h"

void event_dump() {
    std::string input_file = "neutrino_selection_output.root";

    const EventAssembler& assembler = EventAssembler::Instance(input_file);

    int num_bins = 100;  
    double min_score = -1.0;  
    double max_score = 1.0;  

    TH1D* h_topological_score = new TH1D("", "", num_bins, min_score, max_score);

    int num_events = assembler.get_num_events();
    for (int i = 0; i < num_events; ++i) {
        AnalysisEvent event = assembler.get_event(i);
        h_topological_score->Fill(event.topological_score);
    }

    std::string plot_title = "Topological Score Distribution";
    std::string plot_dir = "./plots";

    plot_utils::draw_histogram(h_topological_score, plot_title, plot_dir);

    delete h_topological_score;
}
