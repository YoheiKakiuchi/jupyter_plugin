#include <xeus-python/xinterpreter.hpp>
#include "PythonProcess.h"

namespace cnoid
{

class cnoid_interpreter : public xpyt::interpreter
{
public:
    cnoid_interpreter(bool redirect_output_enabled=true, bool redirect_display_enabled = true);
    virtual ~cnoid_interpreter();

    PythonProcess *process;

    py::object& python_shell();

    void execute_request_impl_impl(const std::string& code,
                                   nl::json &kernel_results,
                                   xeus::execute_request_config &config,
                                   nl::json &user_expressions);
protected:
    nl::json kernel_info_request_impl() override;
    void shutdown_request_impl() override;

    void execute_request_impl(send_reply_callback cb,
                              int /*execution_count*/,
                              const std::string& code,
                              xeus::execute_request_config config,
                              nl::json user_expressions) override;


};

}
