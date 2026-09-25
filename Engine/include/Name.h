#pragma once
#include <string>

struct Name  {
    std::string name = "GameObject";

    template<class Archive>
    void Reflect(Archive& archive) {
        archive.Property("Name", name);
    }
};