/**
 * food.cpp
*/

#include "mylibrary.hpp"

mylibrary::food::Food::Food() {
    this->makesSound = "food";
}

void mylibrary::food::Food::eatMe() {
    std::cout << this->makesSound << ", " << this->makesSound << std::endl;
}
