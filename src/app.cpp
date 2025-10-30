#include "app.hpp"
#include "gui/main_frame.hpp"
#include <gsl/gsl>


auto App::OnInit() -> bool
{
    auto frame = gsl::not_null<MainFrame*>(new MainFrame { "C++ Web Server Control Panel" });
    // The return value of Show() is intentionally ignored. It returns false if the
    // window was already shown, which is not an error in this context.
    (void)frame->Show(true);
    return true;
}
