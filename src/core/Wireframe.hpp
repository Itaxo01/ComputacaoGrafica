#ifndef WIREFRAME_H
#define WIREFRAME_H

#include <ostream>
#include "Point.hpp"
#include "Shape.hpp"
#include <vector>

namespace core{
    class Wireframe: public Shape{
        public:
        std::vector<core::Point> data;
        Wireframe(std::vector<core::Point> &data): data(data) {
            type = ShapeType::WIREFRAME;
        }


        friend std::ostream &operator<<(std::ostream &os, const Wireframe &l) {
            os <<"[";
            for(int i = 0; i<(int)l.data.size(); i++){
                if(i) os<<" - ";
                os<<l.data[i];
            }
            os<<"]";
            return os;
        }
    };
    
}
#endif // WIREFRAME_H