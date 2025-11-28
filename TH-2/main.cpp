#include <iostream>
#include <thread>
#include "lib_sch.h"

int a_count = 0;
int b_count = 0;
int c_count = 0;

void taskA(int n) {
    while (n-- > 0) a_count++;
    std::cout << a_count << "\n";
}

void taskB(int n) {
    while (n-- > 0) b_count++;
    std::cout << b_count << "\n";
}

void taskC(int n) {
    while (n-- > 0) c_count++;
    std::cout << c_count << "\n";
}

int main() {
    parallel_scheduler scheduler(2); 

    scheduler.run(taskA, 50000);
    scheduler.run(taskB, 50000);
    scheduler.run(taskC, 50000);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
