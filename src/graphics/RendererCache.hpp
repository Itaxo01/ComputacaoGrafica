/* Guarda algumas informações para caching da lista de draw do display file. Caso alguma informação mude, o caching se torna inválido */
#ifndef RENDERER_CACHE
#define RENDERER_CACHE
#include "Point.hpp"
#include "Window.hpp"

class RendererCache{
    WindowAttributes l_w;
    unsigned long l_obj_count; // last display file object count
    
    public:
        RendererCache(const WindowAttributes &l_w, unsigned long l_obj_count): l_w(l_w), l_obj_count(l_obj_count){};
        RendererCache(){};
        
        inline bool cache_changed(const WindowAttributes &w, unsigned long obj_count) {
            return (obj_count != l_obj_count || l_w != w);
        }
        inline void store_cache(const WindowAttributes &w, unsigned long obj_count) {
            l_obj_count = obj_count;
            l_w = w;
        }
};


#endif // RENDERER_CACHE