#include "AnalysisEvent.h"
#include "EventAssembler.h"
#include "DisplayAssembler.h"
#include "PlotFunctions.h"

#include "FiducialVolumeSelector.h"

#include "TH1D.h"
#include "TFile.h"
#include "TTree.h"

void selection_analyser() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string input_file = std::string(data_dir) + "/analysis_prod_strange_resample_fhc_run2_fhc_reco2_reco2.root";

    const EventAssembler& event_assembler = EventAssembler::instance(input_file);
    const DisplayAssembler& display_assembler = DisplayAssembler::instance(input_file);

    FiducialVolumeSelector fv_selector(FiducialVolumeSelector::kWirecell);

    int num_events = event_assembler.get_num_events();
    int disp_count = 0;
    for (int i = 0; i < num_events; ++i) {
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (event.mc_has_muon && event.mc_is_kshort_decay_pionic && disp_count < 30)
        {
            display_assembler.plot_event(i);
            disp_count++;
        }

        bool fv_pass = fv_selector.pass_selection(event);
    }
}
