#include "pipe_reader.h"
#include "pipe_writer.h"
#include "burda_hit.h"
#include "i_data_producer.h"
#include "i_data_consumer.h"

template <typename data_type, typename stream_type>
class data_printer : public i_data_consumer<data_type>, public i_data_producer<data_type>
{
    pipe_reader<data_type> reader_;
    pipe_writer<data_type> writer_;
    stream_type* out_stream_;
    public:
    data_printer(stream_type* print_stream) :
    out_stream_(print_stream)
    {
        //out_stream_ = std::move(std::make_unique<std::ostream>(print_stream));
    }
    virtual void start() override
    {
        bool start = true;
        data_type hit;
        while(!reader_.read(hit));
        while(hit.is_valid())
        {
            writer_.write(std::move(hit));
            (*out_stream_) << hit;
            while(!reader_.read(hit));

        }
        std::cout << "PRINTER ENDED ----------------" << std::endl;
    }
    virtual void connect_input(pipe<data_type>* in_pipe) override
    {
        reader_ = pipe_reader<data_type>(in_pipe);
    }
    virtual void connect_output(pipe<data_type>* out_pipe) override
    {
        writer_ = pipe_writer<data_type>(out_pipe);
    }
};