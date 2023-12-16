/**
 * mylibrary.hpp
 */

#pragma once

#include <iostream>
#include <string>

namespace mylibrary {
    namespace food {
        class Food {
            private:

            protected:
                std::string makesSound;

            public:
                Food();
                void eatMe();
        };
    }

    namespace cookie {
        class Cookie : public mylibrary::food::Food {
            private:

            protected:
            
            public:
                Cookie();
                void eatMe();
        };
    }
}
