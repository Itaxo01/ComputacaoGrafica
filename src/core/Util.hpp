#include <iomanip>
#include <string>

inline std::string format(float x, int precision){
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << x;
    return stream.str();
}