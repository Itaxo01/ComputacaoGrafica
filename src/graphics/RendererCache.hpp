/* Guarda algumas informações para caching da lista de draw do display file. Caso alguma informação mude, o caching se torna inválido */
#ifndef RENDERER_CACHE
#define RENDERER_CACHE
#include "Point.hpp"

class RendererCache{
    core::Point l_p0; // window last xmin, ymin (bottom-left)
    core::Point l_p1; // window last xmax, ymax (top-right)
    unsigned long l_obj_count; // last display file object count
    
    public:
        RendererCache(core::Point l_p0, core::Point l_p1, unsigned long l_obj_count): l_p0(l_p0), l_p1(l_p1), l_obj_count(l_obj_count){};
        RendererCache(){};
        
        inline bool cache_changed(core::Point &p0, core::Point &p1, unsigned long obj_count) {
            return (obj_count != l_obj_count || p0 != l_p0 || p1 != l_p1);
        }
        inline void store_cache(core::Point &p0, core::Point &p1, unsigned long obj_count) {
            l_obj_count = obj_count;
            l_p0 = p0;
            l_p1 = p1;
        }
};


#endif // RENDERER_CACHE