all: selection_analysis

selection_analysis: selection_analysis.cc
	$(CXX) $(shell root-config --cflags --libs) -O3 -o $@ $^

.PHONY: clean

clean:
	$(RM) selection_analysis strang_root_dict.o strang_root_dict_rdict.pcm