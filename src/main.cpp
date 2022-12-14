#include "../include/data_nodes/data_reader.h"
#include "../include/data_structs/burda_hit.h"
#include "../include/data_nodes/hit_sorter.h"
#include "../include/data_flow/dataflow_controller.h"
#include "../include/data_nodes/data_printer.h"
#include "../include/data_nodes/clusterer.h"
#include "../include/data_nodes/clusterer_sync.h"
#include "../include/data_nodes/burda_to_mm_hit_adapter.h"
#include "../include/data_nodes/filtering_clusterer.h"
#include "../include/data_structs/cluster.h"
#include "../include/mm_stream.h"
#include "../include/data_nodes/parallel_clusterer.h"
int main(int argc, char** argv)
{
    const std::string filename = "/home/tomas/MFF/DT/measurement_august2022/burda_data/pions_180GeV_bias200V_60deg_alignment_test_F4-W00076_1.txt";
    data_reader<burda_hit>* burda_reader = new data_reader<burda_hit>{filename, 2 << 10};
    auto chip_size = coord(256, 256);
    burda_to_mm_hit_adapter<mm_hit>* converter = new burda_to_mm_hit_adapter<mm_hit>(chip_size, calibration("/home/tomas/MFF/DT/measurement_august2022/calib/F4-W00076/", chip_size));
    hit_sorter<mm_hit>* sorter = new hit_sorter<mm_hit>();
    
    //std::ofstream print_stream("printed_hits.txt");
    //mm_write_stream print_stream("/home/tomas/MFF/DT/clusterer/output/ppppparallel_clustering");
    //mm_write_stream print_stream("/home/tomas/MFF/DT/clusterer/output/filtered_clustering_100keV");
    //data_printer<cluster<mm_hit>, mm_write_stream>* printer = new data_printer<cluster<mm_hit>, mm_write_stream>(&print_stream);
    //pixel_list_clusterer<cluster>* clusterer = new pixel_list_clusterer<cluster>();
    //energy_filtering_clusterer<mm_hit>* clusterer = new energy_filtering_clusterer<mm_hit>();
    parallel_clusterer<mm_hit, sync_pixel_list_clusterer, temporal_clustering_descriptor<mm_hit>>* clusterer = new parallel_clusterer<mm_hit, sync_pixel_list_clusterer, temporal_clustering_descriptor<mm_hit>>(temporal_clustering_descriptor<mm_hit>(4)); 
    dataflow_controller controller;
    controller.add_node(burda_reader);
    controller.add_node(converter);
    controller.add_node(sorter);
    controller.add_node(clusterer);
    //controller.add_node(printer);

    controller.connect_nodes(burda_reader, converter);
    controller.connect_nodes(converter, sorter);
    controller.connect_nodes(sorter, clusterer);
    //controller.connect_nodes(clusterer, printer);
    controller.start_all();
    std::cout << "ALL ENDED" << std::endl;
    //print_stream.close();
    //TODO add virtual destructors
}