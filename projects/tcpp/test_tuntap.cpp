#include "Tuntap.hpp"
#include <iostream>
#include "Interface.hpp"

int main()
{
    std::vector<char> buf;
    buf.reserve(1500);
    // std::cin.get();
    try
    {
        Interface it;
        auto co = it.bind(8000);
        if (co.has_value())
        {
            auto s = co->accept();
            int l = 0;
            if (s.has_value())
            {
                std::string msg{"Hello from cpp!\n"};
                s->write({msg.begin(), msg.end()});
                l = s->read(buf);
                for (int i = 0; i < l; ++i)
                {
                    std::cout << buf[i];
                }
                std::cout << std::endl;
            }
        }
    }
    catch (const std::exception &el)
    {
        std::cout << el.what() << std::endl;
        return -1;
    }
    catch (const char *e)
    {
        std::cout << "Exception e: " << e << std::endl;
    }
    return 0;
}