#include <iostream>
#include <string>
#include "Pic.hpp"
#include "Video.hpp"
#include "Capture.hpp"

int main(){
    std::string mode;
    std::cout << "Choose your mode: " << std::flush;
    std::getline(std::cin, mode);
    if(mode == "pic" || mode == "Pic"){
        pic();
        return 0;
    }else if(mode == "video" || mode == "Video"){
        video();
        return 0;
    }else if(mode == "capture" || mode == "Capture"){
        capture();
        return 0;
    }else{
        std::cerr << "Input error!" << std::endl;
    }
    return 0;
    
}
