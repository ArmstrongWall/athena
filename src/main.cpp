#include <iostream>
#include "localization/localization.h"
using namespace athena;

int main() {
    std::cout << "ATHENA" << std::endl;

    std::unique_ptr<localization::Localization> localization;
    localization.reset(new localization::Localization());

    localization->init();

    //while (1);

    return 0;
}