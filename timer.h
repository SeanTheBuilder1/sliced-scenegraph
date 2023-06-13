#pragma once

#include <chrono>
#include <iostream>
namespace slib{


    struct Timer{
        std::chrono::high_resolution_clock::time_point t0;
        bool doPrint;

        Timer(bool print_result = false):t0(std::chrono::high_resolution_clock::now()),doPrint(print_result){}
        
        ~Timer(){
            auto  t1 = std::chrono::high_resolution_clock::now();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
            if(doPrint)
                std::cout << (double) nanos/1000000 << "ms\n";
        }
        void start(){
    		t0 = std::chrono::high_resolution_clock::now();
        }
        double stop(){
        	auto  t1 = std::chrono::high_resolution_clock::now();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
            if(doPrint)
                std::cout << (double) nanos/1000000 << "ms\n";
            return (double)nanos /1000000;
        }
    };
}