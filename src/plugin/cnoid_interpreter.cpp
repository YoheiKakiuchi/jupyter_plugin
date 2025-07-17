#include "cnoid_interpreter.hpp"

using namespace cnoid;

cnoid_interpreter::cnoid_interpreter(bool r_o_e, bool r_d_e) : xpyt::interpreter(r_o_e, r_d_e)
{
}

cnoid_interpreter::~cnoid_interpreter()
{
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
