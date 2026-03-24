#ifndef GUI_CONTROLLER_HPP
#define GUI_CONTROLLER_HPP

#include "EntityManager.hpp"
#include "Viewport.hpp"
#include "Window.hpp"
#include "log_app.h"
#include "ObjectCreator.hpp"
#include "ObjectListener.hpp"

class GuiController {
    private:
        EntityManager& entityManager;
        Window& window;
        Viewport& viewport;
        ObjectCreator& creator;
        ExampleAppLog& log;
        ObjectListener& listener;

        void HandleCanvasInteractions();
        
        void HandleLeftClick();
        void HandleRightDragging();
        void HandleScroll();

    public:
        GuiController(EntityManager &em, Window& w, Viewport &vp, ObjectCreator &oc, ExampleAppLog &exl, ObjectListener &obl): \
            entityManager(em), window(w), viewport(vp), creator(oc), log(exl), listener(obl)
        {}
        void run();
};


#endif // GUI_CONTROLLER_HPP