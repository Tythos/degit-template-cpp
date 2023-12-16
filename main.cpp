/**
 * main.cpp
 */

#include "mylibrary.hpp"

int main(int nArgs, char** vArgs) {
    mylibrary::food::Food f;
    mylibrary::cookie::Cookie c;
    f.eatMe();
    c.eatMe();
    return 0;
}
