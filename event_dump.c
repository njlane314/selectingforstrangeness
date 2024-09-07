#include "AnalysisEvent.h"
#include "EventAssembler.h"
#include "DisplayAssembler.h"
#include "PlotFunctions.h"

#include "TH1D.h"
#include "TFile.h"
#include "TTree.h"

void event_dump() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/analysis_prod_strange_resample_fhc_run2_fhc_reco2_reco2.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const DisplayAssembler& display_assembler = DisplayAssembler::instance(input_file);

    int num_bins = 100;  
    double min_score = -1.0;  
    double max_score = 1.0;  

    TH1D* h_topological_score = new TH1D("", "", num_bins, min_score, max_score);

    int num_events = event_assembler.get_num_events();
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        h_topological_score->Fill(event.topological_score);

        display_assembler.plot_event(i);
    }

    std::string plot_title = "top_score_dist";
    std::string plot_dir = "./plots";

    plot_functions::draw_histogram(h_topological_score, plot_title, plot_dir);

    delete h_topological_score;
}