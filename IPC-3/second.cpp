#include "shared_array.h"
#include <iostream>
#include <unistd.h>

int main()
{
    shared_array array("demo", 10);

    while (true) {
        array.lock();
        std::cout << "[second] read: " << array[0] << std::endl;
        array.unlock();
        sleep(1);
    }
}
