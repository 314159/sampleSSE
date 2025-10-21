#include "app.hpp"
#include "gui/main_frame.hpp"

auto App::OnInit() -> bool
{
    auto* frame = new MainFrame { "C++ Web Server Control Panel" };
    frame->Show(true);
    return true;
}
