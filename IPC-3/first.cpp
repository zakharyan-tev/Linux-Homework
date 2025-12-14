#include "shared_array.h"
#include <iostream>
#include <unistd.h>

int main()
{
    shared_array array("demo", 10);

    int counter = 0;
    while (true) {
        array.lock();
        array[0] = counter++;
        std::cout << "[first] wrote: " << array[0] << std::endl;
        array.unlock();
        sleep(1);
    }
}
