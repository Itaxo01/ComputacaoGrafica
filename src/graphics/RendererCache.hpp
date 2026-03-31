/* Guarda algumas informações para caching da lista de draw do display file. Caso alguma informação mude, o caching se torna inválido */
#ifndef RENDERER_CACHE
#define RENDERER_CACHE
#include "imgui.h"
#include "Point.hpp"
#include <cassert>

class RendererCache{
    core::Point l_p0; // window last xmin, ymin (bottom-left)
    core::Point l_p1; // window last xmax, ymax (top-right)
    unsigned long l_obj_count; // last display file object count
    bool initialized = false;
    
    public:
        RendererCache(){};
        
        inline bool cache_changed(core::Point &p0, core::Point &p1, unsigned long obj_count) {
            if(initialized){
                store_cache(p0, p1, obj_count);
                initialized = true;
                return true;
            }
            return (obj_count != l_obj_count || p0 != l_p0 || p1 != l_p1);
        }
        inline void store_cache(core::Point &p0, core::Point &p1, unsigned long obj_count) {
            initialized = true;
            l_obj_count = obj_count;
            l_p0 = p0;
            l_p1 = p1;
        }
};


#endif // RENDERER_CACHE