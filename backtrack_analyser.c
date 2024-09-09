#include "AnalysisEvent.h"
#include "EventAssembler.h"
#include "DisplayAssembler.h"
#include "PlotFunctions.h"

#include "FiducialVolumeSelector.h"

#include "TH1D.h"
#include "TFile.h"
#include "TTree.h"

void backtrack_analyser() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/analysis_prod_strange_resample_fhc_run2_fhc_reco2_reco2.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const DisplayAssembler& display_assembler = DisplayAssembler::instance(input_file);

    FiducialVolumeSelector fv_selector(FiducialVolumeSelector::kWirecell);

    int num_events = event_assembler.get_num_events();

    int target_run = 9033;
    int target_subrun = 356;
    int target_event = 17802;
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        
        if (event.run == target_run && event.subrun == target_subrun && event.event == target_event)
        {
            for (size_t j = 0; j < event.backtracked_tid->size(); ++j) 
            {
                std::cout << event.backtracked_pdg->at(j) << std::endl;
            }
        }
    }
}
