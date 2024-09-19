void inspect_root_file() 
{
    const char* data_dir = getenv("DATA_DIR");
    std::string file_path = std::string(data_dir) + "/prod_strange_resample_fhc_run2_fhc_reco2_reco2_signalfilter_1000_analysis.root";
    const char* file_name = file_path.c_str();

    TFile *file = TFile::Open(file_name, "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Unable to open file " << file_name << std::endl;
        return;
    }

    std::cout << "File contents of: " << file_name << std::endl;
    file->ls();  

    TIter next(file->GetListOfKeys());
    TKey *key;
    while ((key = (TKey*)next())) {
        TObject *obj = key->ReadObj();
        if (obj->InheritsFrom("TDirectory")) {
            std::cout << "Directory: " << obj->GetName() << std::endl;

            TDirectory *dir = (TDirectory*)obj;
            dir->ls();  

            TIter next_tree(dir->GetListOfKeys());
            TKey *tree_key;
            while ((tree_key = (TKey*)next_tree())) {
                TObject *tree_obj = tree_key->ReadObj();
                if (tree_obj->InheritsFrom("TTree")) {
                    std::cout << "  TTree: " << tree_obj->GetName() << std::endl;
                    TTree *tree = (TTree*)tree_obj;

                    TObjArray *branch_list = tree->GetListOfBranches();
                    for (int i = 0; i < branch_list->GetEntries(); ++i) {
                        TBranch *branch = (TBranch*)branch_list->At(i);
                        std::cout << "    Branch: " << branch->GetName() << std::endl;
                    }

                    std::cout << "    Scanning..." << std::endl;
                    tree->Scan(); 
                }
            }
        }
    }

    file->Close();
}