#ifndef CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H
#define CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H

#include <QObject>
#include <cnoid/OptionManager> //option

#include "nlohmann/json.hpp"
#include "JupyterPlugin.h"
#include <xeus/xinterpreter.hpp>

#include <sys/types.h>

namespace nl = nlohmann;

namespace cnoid {

class PythonProcess : public QObject
{
    Q_OBJECT;
public:
    PythonProcess(JupyterPlugin *_self) : self(_self)
    {
        self = _self;
    }
    std::string connection_file;

#ifdef USE_OLD_OPTION
    void onSigOptionsParsed(boost::program_options::variables_map& variables);
#else
    void onSigOptionsParsed(OptionManager *_om);
#endif
    bool initialize();
    bool finalize();
    void shutdown_impl();

    void proc();

    pid_t getpid();
    pid_t gettid();

public Q_SLOTS:
    void procRequest(const std::string &code, nl::json &kernel_result, xeus::execute_request_config &config, nl::json &user_expressions);
Q_SIGNALS:
    void sendRequest(const std::string &code, nl::json &kernel_result, xeus::execute_request_config &config, nl::json &user_expressions);


private:
    JupyterPlugin *self;
    bool setupPython();
    bool start();
    void kernelThread();

    class Impl;
    Impl *impl;
};

}

#endif
