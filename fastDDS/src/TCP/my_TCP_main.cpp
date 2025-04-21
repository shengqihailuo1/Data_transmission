#include "hk_image_Publisher.h"
#include "hk_image_Subscriber.h"
#include <fastrtps/Domain.h>
#include <fastrtps/log/Log.h>
#include <string>
#include <optionparser.hpp>
#include <thread>


namespace option = eprosima::option;

//定义了 Arg 结构体来处理不同的参数验证。
struct Arg : public option::Arg
{
    static void print_error(
            const char* msg1,
            const option::Option& opt,
            const char* msg2)
    {
        fprintf(stderr, "%s", msg1);
        fwrite(opt.name, opt.namelen, 1, stderr);
        fprintf(stderr, "%s", msg2);
    }

    static option::ArgStatus Unknown(
            const option::Option& option,
            bool msg)
    {
        if (msg)
        {
            print_error("Unknown option '", option, "'\n");
        }
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Required(
            const option::Option& option,
            bool msg)
    {
        if (option.arg != 0 && option.arg[0] != 0)
        {
            return option::ARG_OK;
        }

        if (msg)
        {
            print_error("Option '", option, "' requires an argument\n");
        }
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Numeric(
            const option::Option& option,
            bool msg)
    {
        char* endptr = 0;
        if (option.arg != 0 && strtol(option.arg, &endptr, 10))
        {
        }
        if (endptr != option.arg && *endptr == 0)
        {
            return option::ARG_OK;
        }

        if (msg)
        {
            print_error("Option '", option, "' requires a numeric argument\n");
        }
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus String(
            const option::Option& option,
            bool msg)
    {
        if (option.arg != 0)
        {
            return option::ARG_OK;
        }
        if (msg)
        {
            print_error("Option '", option, "' requires a numeric argument\n");
        }
        return option::ARG_ILLEGAL;
    }

};


//optionIndex 枚举列出了所有支持的命令行选项。
enum  optionIndex
{
    UNKNOWN_OPT,
    HELP,
    IP,
    PORT,
    TLS,
    WHITELIST
};

 /*
       "至少需要一个参数来指定它是作为 'publisher' 还是 'subscriber' 运行。"   ;
    
       "publisher 将作为 TCP 服务器运行，如果测试通过 NAT 进行，则必须在 wan_ip 参数中设置其公共 IP。" 
       "可选参数包括：publisher  [wan_ip] [port]"   ;
       "\t- wap_ip: 发布者的公共 IP 地址。"   ;
       "\t- port: 物理端口，用于监听传入连接；如果测试使用 WAN IP，则必须在发布者的路由器中允许此端口。" 

       "subscriber 将作为 TCP 客户端运行。如果测试通过 NAT 进行，" 
       "server_ip 必须具有发布者的 WAN IP；如果测试在 LAN 上进行，"   ;
       "则必须具有发布者的 LAN IP。"   ;
       "可选参数包括：subscriber [server_ip] [port]"   ;
       "\t- server_ip: 发布者的 IP 地址。"   ;
       "\t- port: 发布者用于接收连接的物理端口。"    ;
 */

//usage 数组定义了每个命令行选项的详细信息，包括短名称、长名称、参数类型等。
const option::Descriptor usage[] = {
    { UNKNOWN_OPT, 0, "", "",                Arg::None,
      "Usage: HelloWorldExampleTCP <publisher|subscriber>\n\nGeneral options:" },
    { HELP,    0, "h", "help",               Arg::None,      "  -h \t--help  \tProduce help message." },
    { TLS, 0, "t", "tls",          Arg::None,      "  -t \t--tls \tUse TLS." },
    { WHITELIST, 0, "w", "whitelist",       Arg::String,    "  -w \t--whitelist \tUse Whitelist." },

    { UNKNOWN_OPT, 0, "", "",                Arg::None,      "\nPublisher options:"},
    { IP, 0, "a", "address",                   Arg::String,
      "  -a <address>, \t--address=<address> \tPublic IP Address of the publisher (Default: None)." },
    { PORT, 0, "p", "port",                 Arg::Numeric,
      "  -p <num>, \t--port=<num>  \tPhysical Port to listening incoming connections (Default: 5100)." },

    { UNKNOWN_OPT, 0, "", "",                Arg::None,      "\nSubscriber options:"},
    { IP, 0, "a", "address",                   Arg::String,
      "  -a <address>, \t--address=<address> \tIP Address of the publisher (Default: 127.0.0.1)." },
    { PORT, 0, "p", "port",                 Arg::Numeric,
      "  -p <num>, \t--port=<num>  \tPhysical Port where the publisher is listening for connections (Default: 5100)." },

    { 0, 0, 0, 0, 0, 0 }
};

using namespace eprosima;
using namespace fastrtps;
using namespace rtps;
int main(
        int argc,
        char** argv)
{
    int columns;

#if defined(_WIN32)
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "COLUMNS") == 0 && buf != nullptr)
    {
        columns = strtol(buf, nullptr, 10);
        free(buf);
    }
    else
    {
        columns = 80;
    }
#else
    columns = getenv("COLUMNS") ? atoi(getenv("COLUMNS")) : 80;
#endif // if defined(_WIN32)

    int type = 1;
    std::string wan_ip;
    int port = 5100;
    bool use_tls = false;
    std::vector<std::string> whitelist;

    if (argc > 1)
    {
        if (strcmp(argv[1], "publisher") == 0)
        {
            type = 1;
        }
        else if (strcmp(argv[1], "subscriber") == 0)
        {
            type = 2;
        }

        argc -= (argc > 0);
        argv += (argc > 0); // skip program name argv[0] if present
        --argc; ++argv; // skip pub/sub argument
        option::Stats stats(usage, argc, argv);
        std::vector<option::Option> options(stats.options_max);
        std::vector<option::Option> buffer(stats.buffer_max);
        option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

        if (parse.error())
        {
            return 1;
        }

        if (options[HELP])
        {
            option::printUsage(fwrite, stdout, usage, columns);
            return 0;
        }

        for (int i = 0; i < parse.optionsCount(); ++i)
        {
            option::Option& opt = buffer[i];
            switch (opt.index())
            {
                case IP:
                {
                    if (opt.arg != nullptr)
                    {
                        wan_ip = std::string(opt.arg);
                    }
                    else
                    {
                        option::printUsage(fwrite, stdout, usage, columns);
                        return 0;
                    }
                    break;
                }

                case PORT:
                    port = strtol(opt.arg, nullptr, 10);
                    break;

                case TLS:
                    use_tls = true;
                    break;

                case WHITELIST:
                    whitelist.emplace_back(opt.arg);
                    break;

                case UNKNOWN_OPT:
                    option::printUsage(fwrite, stdout, usage, columns);
                    return 0;
                    break;
            }
        }
    }
    else
    {
        option::printUsage(fwrite, stdout, usage, columns);
        return 0;
    }


    switch (type)
    {
        case 1:
        {
            hk_image_Publisher mypub;
            if (mypub.init(wan_ip, static_cast<uint16_t>(port), use_tls, whitelist))
            {
                std::cout << "Started Publisher..."<<std::endl;
                mypub.start_publish();
            }
            break;
        }
        case 2:
        {
            hk_image_Subscriber mysub;
            if (mysub.init(wan_ip, static_cast<uint16_t>(port), use_tls, whitelist))
            {
                std::cout<<"Started Subscriber..."<<std::endl;
                while(1){
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
                //主程序在这里死循环，它接收到消息后会自动回调函数on_data_available
            }
            break;
        }
    }
    Domain::stopAll();
    return 0;
}
