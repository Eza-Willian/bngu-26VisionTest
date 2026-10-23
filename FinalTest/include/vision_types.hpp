#pragma once
#include <string>

namespace Vision{
    struct Result{
        bool valid = false;
        float x = 0.0f;
        float y = 0.0f;
        double angle = 0.0;
        std::string label = "";
        double confidence = 1.0;
    };
    
}