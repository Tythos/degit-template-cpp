/**
 * cookie.cpp
 */

#include "mylibrary.hpp"

mylibrary::cookie::Cookie::Cookie() {
    this->makesSound = "crunch";
}

void mylibrary::cookie::Cookie::eatMe() {
    std::cout << this->makesSound << ", " << this->makesSound << ", " << this->makesSound << "... Can I have some more!?" << std::endl;
}
