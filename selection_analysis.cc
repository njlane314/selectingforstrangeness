#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "TChain.h"
#include "TFile.h"
#include "TParameter.h"
#include "TTree.h"
#include "TVector3.h"

#include "EventAssembler.h"
#include "EventCategory.h"
#include "FiducialVolume.h"
#include "Constants.h"

class SelectionAnalysis {

public:

    SelectionAnalysis();
    ~SelectionAnalysis();
};

void process_files(const std::vector<std::string>& input_file_names, const std::string& output_file_name)
{
    TChain events_chain("");
    TChain subruns_chain("");

    for (const auto& file_name : input_file_names)
    {
        events_chain.Add(file_name.c_str());
        subruns_chain.Add(file_name.c_str());
    }

    EventAssembler event_assembler(input_file_names[0]);

    TFile* output_file = new TFile(output_file_name.c_str(), "RECREATE");
    TTree* output_tree = new TTree("", "");

    long events_entry = 0;
    while (true)
    {
        if (events_entry % 1000 == 0) 
        {
            std::cout << " Processing event #" << events_entry << '\n';
        }

        int local_entry = events_chain.LoadTree(events_entry);
        AnalysisEvent event = event_assembler.get_event(local_entry);

        if (local_entry < 0) break;

        
    }

    Long64_t n_entries = events_chain.GetEntries();
    for (Long64_t i = 0; i < n_entries; i++)
    {
        events_chain.GetEntry(i);

        AnalysisEvent event = event_assembler.get_event(i);

    }

    output_tree->Write();
    output_file->Close();

    delete output_file;
}

void run_selection_analysis(const std::string& input_file_name, const std::string& output_file_name)
{
    std::vector<std::string> input_files = { input_file_name };
    process_files(input_files, output_file_name);
}

int main(int argc, char* argv[])
{
    if (argc != 3) 
    {
        std::cout << "Usage: selection_analysis INPUT_FILE OUTPUT_FILE\n";
        return 1;
    }

    std::string input_file_name(argv[1]);
    std::string output_file_name(argv[2]);

    run_selection_analysis(input_file_name, output_file_name);

    return 0;
}
