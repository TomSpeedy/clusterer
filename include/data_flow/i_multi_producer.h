#include "i_simple_producer.h"
#pragma once
template <typename data_type>
class i_multi_producer :  public i_data_producer<data_type>
{
    protected:
    multi_pipe_writer<data_type> writer_;
    using split_descriptor_type = pipe_descriptor<data_type>;
    public:
    i_multi_producer(split_descriptor_type * split_descriptor) :
    writer_(split_descriptor){};
    i_multi_producer() :
    writer_(new trivial_clustering_descriptor<data_type>()){};
    virtual ~i_multi_producer(){};
    virtual void connect_output(default_pipe<data_type>* pipe) final override
    {
        writer_.add_pipe(pipe);
    }
};