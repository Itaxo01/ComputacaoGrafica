#ifndef GUI_CONTROLLER_HPP
#define GUI_CONTROLLER_HPP

#include "EntityManager.hpp"
#include "Viewport.hpp"
#include "Window.hpp"
#include "log_app.h"
#include "ObjectCreator.hpp"
//#include "ObjectListener.hpp"
#include "ObjectGUI.hpp"

class GuiController {
    private:
        EntityManager& entityManager;
        Window& window;
        Viewport& viewport;
        ObjectCreator& creator;
        ExampleAppLog& log;
        //ObjectListener& listener;
        ObjectGUI& objGUI;

        void HandleCanvasInteractions();
        
        void HandleLeftClick();
        void HandleRightDragging();
        void HandleScroll();
        void HandleKeyboard();

    public:
        GuiController(EntityManager &em, Window& w, Viewport &vp, ObjectCreator &oc, ExampleAppLog &exl, ObjectGUI &og): \
            entityManager(em), window(w), viewport(vp), creator(oc), log(exl), objGUI(og)
        {}
        void run();
};


#endif // GUI_CONTROLLER_HPP