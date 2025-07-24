#include "cnoid_interpreter.hpp"

#include "nlohmann/json.hpp"

#include "xeus/xinterpreter.hpp"
#include "xeus/xsystem.hpp"

#include "pybind11/pybind11.h"
#include "pybind11/functional.h"
#include "pybind11_json/pybind11_json.hpp"

#include "xeus-python/xinterpreter.hpp"
#include "xeus-python/xeus_python_config.hpp"
#include "xeus-python/xtraceback.hpp"
#include "xeus-python/xutils.hpp"

//#include "xcomm.hpp"
//#include "xkernel.hpp"
//#include "xdisplay.hpp"
#include "xinput.hpp"
//#include "xinternal_utils.hpp"
//#include "xstream.hpp"

using namespace cnoid;
using namespace pybind11::literals; // for ""_a

cnoid_interpreter::cnoid_interpreter(bool r_o_e, bool r_d_e) : xpyt::interpreter(r_o_e, r_d_e)
{
}

cnoid_interpreter::~cnoid_interpreter()
{
}

py::object& cnoid_interpreter::python_shell()
{
    return this->m_ipython_shell;
}

nl::json cnoid_interpreter::kernel_info_request_impl()
{
    nl::json result;
    result["implementation"] = "choreonoid(xeus-python)";
    result["implementation_version"] = XPYT_VERSION;

    /* The jupyter-console banner for xeus-python is the following:
       __  _____ _   _ ___
       \ \/ / _ \ | | / __|
       >  <  __/ |_| \__ \
       /_/\_\___|\__,_|___/

       xeus-python: a Jupyter lernel for Python
    */

    std::string banner = ""
    "  __  _____ _   _ ___\n"
    "  \\ \\/ / _ \\ | | / __|\n"
    "   >  <  __/ |_| \\__ \\\n"
    "  /_/\\_\\___|\\__,_|___/\n"
    "\n"
    "  choreonoid(xeus-python): a Jupyter kernel for Choreonoid(Python)\n"
    "  Python ";
    banner.append(PY_VERSION);
    banner.append("\n# start exec(open('/choreonoid_ws/install/share/irsl_choreonoid/sample/irsl_import.py').read()))");

    result["banner"] = banner;
    result["debugger"] = true;

    result["language_info"]["name"] = "python";
    result["language_info"]["version"] = PY_VERSION;
    result["language_info"]["mimetype"] = "text/x-python";
    result["language_info"]["file_extension"] = ".py";
#if 0
    result["help_links"] = nl::json::array();
    result["help_links"][0] = nl::json::object({
            {"text", "Xeus-Python Reference"},
            {"url", "https://xeus-python.readthedocs.io"}
        });
#endif
    result["status"] = "ok";
    return result;
}

void cnoid_interpreter::shutdown_request_impl()
{
    if (!!process) {
        process->shutdown_impl();
    }
}

#if 1
using namespace xpyt;
/* original implementation (copied) */
void cnoid_interpreter::execute_request_impl(send_reply_callback cb,
                                       int /*execution_count*/,
                                       const std::string& code,
                                       xeus::execute_request_config config,
                                       nl::json user_expressions)
{
    std::cout << "exec(req) pid: " << process->getpid() << ", tid: " << process->gettid() << std::endl;
        py::gil_scoped_acquire acquire;
        nl::json kernel_res;

        // Reset traceback
        m_ipython_shell.attr("last_error") = py::none();

        // Scope guard performing the temporary monkey patching of input and
        // getpass with a function sending input_request messages.
        auto input_guard = input_redirection(config.allow_stdin);

        bool exception_occurred = false;
        try
        {
            m_ipython_shell.attr("run_cell")(code, "store_history"_a=config.store_history, "silent"_a=config.silent);
        }
        catch(std::runtime_error& e)
        {
            const std::string error_msg = e.what();
            if(!config.silent)
            {
                publish_execution_error("RuntimeError", error_msg, std::vector<std::string>());
            }
            kernel_res["ename"] = "std::runtime_error";
            kernel_res["evalue"] = error_msg;
            exception_occurred = true;
        }
        catch (py::error_already_set& e)
        {
            xerror error = extract_already_set_error(e);
            if (!config.silent)
            {
                publish_execution_error(error.m_ename, error.m_evalue, error.m_traceback);
            }

            kernel_res["status"] = "error";
            kernel_res["ename"] = error.m_ename;
            kernel_res["evalue"] = error.m_evalue;
            kernel_res["traceback"] = error.m_traceback;
            exception_occurred = true;
        }
        catch(...)
        {
            if(!config.silent)
            {
                publish_execution_error("unknown_error", "", std::vector<std::string>());
            }
            kernel_res["ename"] = "UnknownError";
            kernel_res["evalue"] = "";
            exception_occurred = true;
        }

        // Get payload
        kernel_res["payload"] = m_ipython_shell.attr("payload_manager").attr("read_payload")();
        m_ipython_shell.attr("payload_manager").attr("clear_payload")();

        if(exception_occurred)
        {
            kernel_res["status"] = "error";
            kernel_res["traceback"] = std::vector<std::string>();
            cb(kernel_res);
            return;
        }

        if (m_ipython_shell.attr("last_error").is_none())
        {
            kernel_res["status"] = "ok";
            kernel_res["user_expressions"] = m_ipython_shell.attr("user_expressions")(user_expressions);
        }
        else
        {
            py::list pyerror = m_ipython_shell.attr("last_error");

            xerror error = extract_error(pyerror);

            if (!config.silent)
            {
                publish_execution_error(error.m_ename, error.m_evalue, error.m_traceback);
            }

            kernel_res["status"] = "error";
            kernel_res["ename"] = error.m_ename;
            kernel_res["evalue"] = error.m_evalue;
            kernel_res["traceback"] = error.m_traceback;
        }
        cb(kernel_res);
}
#else
void cnoid_interpreter::execute_request_impl(send_reply_callback cb,
                                       int /*execution_count*/,
                                       const std::string& code,
                                       xeus::execute_request_config config,
                                       nl::json user_expressions)
{
    nl::json kernel_res;
    std::cout << "exec(req) pid: " << process->getpid() << ", tid: " << process->gettid() << std::endl;
    process->sendRequest(code, kernel_res, config, user_expressions);
    cb(kernel_res);
}
#endif
void cnoid_interpreter::execute_request_impl_impl(const std::string& code,
                                                  nl::json &kernel_res,
                                                  xeus::execute_request_config &config,
                                                  nl::json &user_expressions)
{
    std::cout << "exec(impl) pid: " << process->getpid() << ", tid: " << process->gettid() << std::endl;
    py::gil_scoped_acquire acquire;

    // Reset traceback
    m_ipython_shell.attr("last_error") = py::none();

    // Scope guard performing the temporary monkey patching of input and
    // getpass with a function sending input_request messages.
    auto input_guard = xpyt::input_redirection(config.allow_stdin);

    bool exception_occurred = false;

    try
    {
        //m_ipython_shell.attr("run_cell")(code, "store_history"_a=config.store_history, "silent"_a=config.silent);
        m_ipython_shell.attr("run_cell")(code, py::arg("store_history") = config.store_history, py::arg("silent") = config.silent);
    }
    catch(std::runtime_error& e)
    {
        const std::string error_msg = e.what();
        if(!config.silent)
        {
            publish_execution_error("RuntimeError", error_msg, std::vector<std::string>());
        }
        kernel_res["ename"] = "std::runtime_error";
        kernel_res["evalue"] = error_msg;
        exception_occurred = true;
    }
    catch (py::error_already_set& e)
    {
        xpyt::xerror error = xpyt::extract_already_set_error(e);
        if (!config.silent)
        {
            publish_execution_error(error.m_ename, error.m_evalue, error.m_traceback);
        }

        kernel_res["status"] = "error";
        kernel_res["ename"] = error.m_ename;
        kernel_res["evalue"] = error.m_evalue;
        kernel_res["traceback"] = error.m_traceback;
        exception_occurred = true;
    }
    catch(...)
    {
        if(!config.silent)
        {
            publish_execution_error("unknown_error", "", std::vector<std::string>());
        }
        kernel_res["ename"] = "UnknownError";
        kernel_res["evalue"] = "";
        exception_occurred = true;
    }

    // Get payload
    kernel_res["payload"] = m_ipython_shell.attr("payload_manager").attr("read_payload")();
    m_ipython_shell.attr("payload_manager").attr("clear_payload")();

    if(exception_occurred)
    {
        kernel_res["status"] = "error";
        kernel_res["traceback"] = std::vector<std::string>();
        //cb(kernel_res);
        return;
    }

    if (m_ipython_shell.attr("last_error").is_none())
    {
        kernel_res["status"] = "ok";
        kernel_res["user_expressions"] = m_ipython_shell.attr("user_expressions")(user_expressions);
    }
    else
    {
        py::list pyerror = m_ipython_shell.attr("last_error");

        xpyt::xerror error = xpyt::extract_error(pyerror);

        if (!config.silent)
        {
            publish_execution_error(error.m_ename, error.m_evalue, error.m_traceback);
        }

        kernel_res["status"] = "error";
        kernel_res["ename"] = error.m_ename;
        kernel_res["evalue"] = error.m_evalue;
        kernel_res["traceback"] = error.m_traceback;
    }
    //cb(kernel_res);
}

