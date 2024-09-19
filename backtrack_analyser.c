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

    int target_run = 11872;
    int target_subrun = 161;
    int target_event = 8061;
    for (int i = 0; i < num_events; ++i) {
        std::cout << "---------------------------------" << std::endl;
        const AnalysisEvent& event = event_assembler.get_event(i);
        if (!event.mc_has_muon || !event.mc_is_kshort_decay_pionic) continue;
        //if (event.run == target_run && event.subrun == target_subrun && event.event == target_event)
        //{        
            event_assembler.print_event(i);
            display_assembler.plot_event(i);

            auto bt_pdg = *(event.bt_pdg); 
            auto bt_tids = *(event.bt_tids);
            auto bt_energy = *(event.bt_energy);

            std::cout << "Backtrack values:" << std::endl;
            for (size_t j = 0; j < bt_pdg.size(); ++j) {
                std::cout << "  bt_pdg: " << bt_pdg[j] 
                        << ", bt_energy: " << bt_energy[j] 
                        << std::endl;

                std::cout << "  bt_tids: ";
                for (size_t k = 0; k < bt_tids[j].size(); ++k) {
                    std::cout << bt_tids[j][k] << " ";
                }
                std::cout << std::endl;
            }

            std::cout << "Decay pion-plus track identifier " << event.mc_kshrt_piplus_tid << std::endl;
            std::cout << "Decay pion-minus track identifier " << event.mc_kshrt_piminus_tid << std::endl;
        //}
    }
}
