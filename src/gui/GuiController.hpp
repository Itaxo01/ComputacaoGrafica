#ifndef GUI_CONTROLLER_HPP
#define GUI_CONTROLLER_HPP

#include "EntityManager.hpp"
#include "Viewport.hpp"
#include "Window.hpp"
#include "log_app.h"
#include "ObjectCreator.hpp"

class GuiController {
    private:
        EntityManager& entityManager;
        Window& window;
        Viewport& viewport;
        ObjectCreator& creator;
        ExampleAppLog& log;
    public:
        GuiController(EntityManager &em, Window& w, Viewport &vp, ObjectCreator &oc, ExampleAppLog &exl): \
            entityManager(em), window(w), viewport(vp), creator(oc), log(exl)
        {}
        void run(){
            viewport.DrawWindow();
            creator.DrawWindow();

            ImGui::SetNextWindowPos(ImVec2(876, 361), ImGuiCond_FirstUseEver); // Log window position
            ImGui::SetNextWindowSize(ImVec2(365, 363), ImGuiCond_FirstUseEver); // Log window size
            log.Draw("Log");
        }

        
};


#endif // GUI_CONTROLLER_HPP