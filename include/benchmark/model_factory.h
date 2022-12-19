#include "../utils.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include "../data_flow/dataflow_package.h"
#include "i_time_measurable.h"
#include "measuring_clock.h"
#include "../data_nodes/nodes_package.h"
#include "../mm_stream.h"
#pragma once
struct node
{
    std::string type;
    uint32_t id;
    node(const std::string & type, uint32_t id) :
    type(type),
    id(id){}
    bool operator == (const node & other)
    {
        return type == other.type && id == other.id;
    }
};
struct edge
{
    node from;
    node to;
    edge(node from, node to) : 
    from(from),
    to(to){}
};
class architecture_type
{
    
    std::vector<edge> edges_;
    std::vector<node> nodes_;
    std::map<std::string, abstract_node_descriptor*> node_descriptors_; 
    uint32_t tile_size_ = 1;
    double window_size_ = 50000000.;
    bool is_letter(char ch)
    {
        return ch >= 'a' && ch <= 'z';
    }
    bool is_digit(char ch)
    {
        return ch >= '0' && ch <= '9';
    }
    uint32_t read_numbers(const std::string & str, uint32_t & index)
    {
        std::string num;
        while (index < str.size() && is_digit(str[index]))
        {
            
            num.push_back(str[index]);
            ++index;
        } 
        while(index < str.size() && !is_letter(str[index]) && !is_digit(str[index]))
        {
            ++index; //we are skipping non alphanumerical characters
        }
        return std::stoi(num);

    }
    std::string read_letters(const std::string & str, uint32_t & index)
    {
        std::string word;
        while (index < str.size() && is_letter(str[index]))
        {
            word.push_back(str[index]);
            ++index;
        } 
        return word;

    }
    void init_arch(const std::string & arch_string)
    {
        uint32_t i = 0;
        while (i < arch_string.size())
        {
            std::string type = read_letters(arch_string, i);
            uint32_t id = read_numbers(arch_string, i);
            auto from_node = node(type, id);
            type = read_letters(arch_string, i);
            id = read_numbers(arch_string, i);
            auto to_node = node(type, id);
            edges_.push_back(edge(from_node, to_node));
            if(std::find(nodes_.begin(), nodes_.end(), from_node) == nodes_.end())
                nodes_.push_back(from_node);
            if(std::find(nodes_.begin(), nodes_.end(), to_node) == nodes_.end())
                nodes_.push_back(to_node);
        }
    }
    public:
    architecture_type(const std::string & arch_string, uint32_t tile_size = 1, double window_size = 50000000) :
    window_size_(window_size),
    tile_size_(tile_size)
    {
        init_arch(arch_string);
    }

    architecture_type(const std::string & arch_string, std::map<std::string, abstract_node_descriptor*> node_descriptors, uint32_t tile_size = 1, double window_size = 50000000) :
    node_descriptors_(node_descriptors),
    window_size_(window_size),
    tile_size_(tile_size)
    {
        init_arch(arch_string);
    }
    std::map<std::string, abstract_node_descriptor*>& node_descriptors()
    {
        return node_descriptors_;
    } 
    
    std::vector<node> & nodes()
    {
        return nodes_;
    }
    std::vector<edge> & edges()
    {
        return edges_;
    }
    uint32_t tile_size()
    {
        return tile_size_;
    }
    double window_size()
    {
        return window_size_;
    }



};

class model_factory
{
    std::string data_file;
    std::string calib_file;
    using standard_clustering_type = pixel_list_clusterer<cluster>;
    using parallel_clusterer_type = parallel_clusterer<mm_hit, pixel_list_clusterer<cluster>>;
    mm_write_stream * print_stream;
    template <typename arch_type, typename ... args_type>
    i_data_node* create_node(node node, arch_type arch, args_type... args)
    {
        if(node.type == "r")
            return new data_reader<burda_hit>(data_file, 2 << 10);
        else if(node.type == "rr")
            return new repeating_data_reader<burda_hit>{data_file, 2 << 21};  
        else if(node.type == "bm")
        {
            if (arch.node_descriptors().find(node.type + std::to_string(node.id)) != arch.node_descriptors().end())
                return new burda_to_mm_hit_adapter<mm_hit>(
                    dynamic_cast<node_descriptor<burda_hit, mm_hit>*>(arch.node_descriptors()["bm" + std::to_string(node.id)]));
            else
                    return new burda_to_mm_hit_adapter<mm_hit>(calibration(calib_file, current_chip::chip_type::size()));
        }
        else if(node.type == "s")
        {
            if (arch.node_descriptors().find(node.type + std::to_string(node.id)) != arch.node_descriptors().end())
                return new hit_sorter<mm_hit>(
                    dynamic_cast<node_descriptor<mm_hit, mm_hit>*>(arch.node_descriptors()["s" + std::to_string(node.id)]));
            else
                return new hit_sorter<mm_hit>();
        } 
        else if(node.type == "p")
            return new data_printer<cluster<mm_hit>, mm_write_stream>(print_stream);
        else if(node.type == "m")
            return new cluster_merging<mm_hit>(
                dynamic_cast<node_descriptor<cluster<mm_hit>, cluster<mm_hit>>*>(arch.node_descriptors()["m" + std::to_string(node.id)]));
        else if(node.type == "sc")
            return new standard_clustering_type();
        else if(node.type == "ec")
            return new energy_filtering_clusterer<mm_hit>();
        else if(node.type == "ppc")
            return new parallel_clusterer_type(
                dynamic_cast<node_descriptor<cluster<mm_hit>, mm_hit>*>(arch.node_descriptors()["ppc" + std::to_string(node.id)]));
        else if(node.type == "trc")
            return new trigger_clusterer<mm_hit, standard_clustering_type, frequency_diff_trigger<mm_hit>>();
        else if(node.type == "tic")
            return new standard_clustering_type( args...);
        else if(node.type == "bbc")
            return new halo_buffer_clusterer<mm_hit, standard_clustering_type>(args...);
        else if(node.type == "trbbc")
            return new trigger_clusterer<mm_hit, halo_buffer_clusterer<mm_hit, standard_clustering_type>, frequency_diff_trigger<mm_hit>>(
                arch.window_size());
        else if(node.type == "co")
            return new cluster_sorting_combiner<mm_hit>();
        throw std::invalid_argument("node of given type was not recognized");
        
    };
    
    public:
    template <typename ...clustering_args_type>
    void create_model(dataflow_controller * controller, architecture_type arch,
      const std::string& data_file, const std::string& calib_file, clustering_args_type ... cl_args)
    {
        this->data_file = data_file;
        this->calib_file = calib_file;
        this->print_stream = new mm_write_stream("/home/tomas/MFF/DT/clusterer/output/new") ;
        std::vector<i_data_node*> data_nodes;
        for (auto node : arch.nodes())
        {
            data_nodes.emplace_back(create_node(node, arch, cl_args...));
            controller->add_node(data_nodes.back());
        }
        for(auto edge : arch.edges())
        {
            auto from_index = std::find(arch.nodes().begin(), arch.nodes().end(), edge.from) - arch.nodes().begin();
            auto to_index = std::find(arch.nodes().begin(), arch.nodes().end(), edge.to) - arch.nodes().begin();
            controller->connect_nodes(data_nodes[from_index], data_nodes[to_index]);
        }
    }
};