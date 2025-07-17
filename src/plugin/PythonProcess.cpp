#include "PythonProcess.h"

// cnoid
#include <cnoid/UTF8>
#include <cnoid/stdx/filesystem>
// thread
#include <thread>

// xeus
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <xeus/xinterpreter.hpp>
#include <xeus/xhelper.hpp>

#include <xeus-zmq/xserver_zmq_split.hpp>
#include <xeus-zmq/xzmq_context.hpp>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <xeus-python/xinterpreter.hpp>
#include <xeus-python/xinterpreter_raw.hpp>
#include <xeus-python/xdebugger.hpp>
#include <xeus-python/xtraceback.hpp>
#include <xeus-python/xpaths.hpp>
#include <xeus-python/xeus_python_config.hpp>
#include <xeus-python/xutils.hpp>

// HOTFIX
#include <dlfcn.h>

#include "cnoid_interpreter.hpp"

// for shutdown
#include <QCoreApplication>

//#define IRSL_DEBUG
#include "irsl_debug.h"

using namespace cnoid;

namespace filesystem = cnoid::stdx::filesystem;

namespace cnoid {
class PythonProcess::Impl
{
public:
    Impl(PythonProcess *_self);

public:
    PythonProcess *self;

    std::unique_ptr<pybind11::scoped_interpreter> py_interpreter;

    using kernel_ptr = std::unique_ptr<xeus::xkernel>;
    kernel_ptr kernel;

    using interpreter_ptr = std::unique_ptr<xeus::xinterpreter>;
    xeus::xinterpreter *interpreter;

    void *python;
};
}

PythonProcess::Impl::Impl(PythonProcess *_self) : self(_self), interpreter(nullptr), python(nullptr)
{
}

void PythonProcess::onSigOptionsParsed(OptionManager *_om)
{
    DEBUG_PRINT();
    if(_om->count("--jupyter-connection")) {
        auto op = _om->get_option("--jupyter-connection");
        connection_file = op->as<std::string>();
        DEBUG_STREAM(" jupyter-connection:" << connection_file);

        bool res = setupPython();
        std::thread th_kernel(&PythonProcess::kernelThread, this);
        th_kernel.detach();
    } else {
        bool res = setupPython();
        std::thread th_kernel(&PythonProcess::kernelThread, this);
        th_kernel.detach();
    }
}

bool PythonProcess::initialize()
{
    DEBUG_PRINT();

    impl = new Impl(this);

    auto om = OptionManager::instance();
    om->add_option("--jupyter-connection", "connection file for jupyter");
    om->sigOptionsParsed(1).connect(
        [this](OptionManager *_om) { onSigOptionsParsed(_om); } );

    return true;
}

bool PythonProcess::finalize()
{
    DEBUG_PRINT();
    dlclose(impl->python);
    return true;
}

void PythonProcess::procRequest(const std::string &code, bool &exception_occurred, nl::json &kernel_res)
{
    DEBUG_PRINT();
    xeus::execute_request_config config;
    exception_occurred = false;

    try
    {
        //m_ipython_shell.attr("run_cell")(code, "store_history"_a=config.store_history, "silent"_a=config.silent);
    }
    catch(std::runtime_error& e)
    {
        const std::string error_msg = e.what();
        if(!config.silent)
        {
            impl->interpreter->publish_execution_error("RuntimeError", error_msg, std::vector<std::string>());
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
            impl->interpreter->publish_execution_error(error.m_ename, error.m_evalue, error.m_traceback);
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
            impl->interpreter->publish_execution_error("unknown_error", "", std::vector<std::string>());
        }
        kernel_res["ename"] = "UnknownError";
        kernel_res["evalue"] = "";
        exception_occurred = true;
    }
}

void PythonProcess::shutdown_impl()
{
    DEBUG_PRINT();
    QCoreApplication::quit(); // [TODO] exit choreonoid, is it OK?
}

bool PythonProcess::setupPython()
{
    std::string ver(Py_GetVersion());
    std::cout << "ver: " << ver << std::endl;

    PyStatus status;
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    const std::wstring pname(L"choreonoid");
    const std::wstring phome(L"/tmp");
    config.program_name = const_cast<wchar_t*>(pname.c_str());
    config.home = const_cast<wchar_t*>(phome.c_str());
    int argc = 0;
    char **argv = nullptr;
    PyConfig_SetBytesArgv(&config, argc, argv);

    using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;
    history_manager_ptr hist = xeus::make_in_memory_history_manager();

    nl::json debugger_config;
    debugger_config["python"] = "choreonoid";

    impl->python = dlopen("/usr/lib/x86_64-linux-gnu/libpython3.8.so", RTLD_NOW | RTLD_GLOBAL);
    impl->py_interpreter.reset(new pybind11::scoped_interpreter());

    if (!connection_file.empty()) {
        std::unique_ptr<xeus::xcontext> context = xeus::make_zmq_context();
        Impl::interpreter_ptr interpreter_ = Impl::interpreter_ptr(new cnoid_interpreter());
        impl->interpreter = interpreter_.get();
        dynamic_cast<cnoid_interpreter *>(impl->interpreter)->process = this;
        xeus::xconfiguration config = xeus::load_configuration(connection_file);
        impl->kernel = Impl::kernel_ptr(new xeus::xkernel(config,
                                                          xeus::get_user_name(),
                                                          std::move(context),
                                                          std::move(interpreter_),
                                                          xeus::make_xserver_shell_main,
                                                          std::move(hist),
                                                          xeus::make_console_logger(xeus::xlogger::msg_type,
                                                                                    xeus::make_file_logger(xeus::xlogger::content, "xeus.log")),
                                                          xpyt::make_python_debugger,
                                                          debugger_config));
    } else {
        std::unique_ptr<xeus::xcontext> context = xeus::make_zmq_context();

        std::cout << "Interpreter" << std::endl;
        Impl::interpreter_ptr interpreter_ = Impl::interpreter_ptr(new cnoid_interpreter());
        impl->interpreter = interpreter_.get();
        dynamic_cast<cnoid_interpreter *>(impl->interpreter)->process = this;
        std::cout << "Instantiating kernel" << std::endl;
        impl->kernel = Impl::kernel_ptr(new xeus::xkernel(xeus::get_user_name(),
                                                          std::move(context),
                                                          std::move(interpreter_),
                                                          xeus::make_xserver_shell_main,
                                                          std::move(hist),
                                                          nullptr,
                                                          xpyt::make_python_debugger,
                                                          debugger_config));

        std::cout << "Getting config" << std::endl;
        const auto& config = impl->kernel->get_config();
        std::cout <<
            "Starting xeus-python kernel...\n\n"
            "If you want to connect to this kernel from an other client, just copy"
            " and paste the following content inside of a `kernel.json` file. And then run for example:\n\n"
            "# jupyter console --existing kernel.json\n\n"
            "kernel.json\n```\n{\n"
            "    \"transport\": \"" + config.m_transport + "\",\n"
            "    \"ip\": \"" + config.m_ip + "\",\n"
            "    \"control_port\": " + config.m_control_port + ",\n"
            "    \"shell_port\": " + config.m_shell_port + ",\n"
            "    \"stdin_port\": " + config.m_stdin_port + ",\n"
            "    \"iopub_port\": " + config.m_iopub_port + ",\n"
            "    \"hb_port\": " + config.m_hb_port + ",\n"
            "    \"signature_scheme\": \"" + config.m_signature_scheme + "\",\n"
            "    \"key\": \"" + config.m_key + "\"\n"
            "}\n```"
            << std::endl;
    }

    connect(this, &PythonProcess::sendRequest,
            this, &PythonProcess::procRequest,
            Qt::BlockingQueuedConnection);  //Qt::DirectConnection);

    return true;
}

void PythonProcess::kernelThread()
{
    impl->kernel->start();
}

