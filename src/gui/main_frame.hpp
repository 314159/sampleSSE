#ifndef MAIN_FRAME_HPP
#define MAIN_FRAME_HPP

#include <gsl/gsl> // Include the Guideline Support Library
#include <memory>
#include <wx/event.h>
#include <wx/wx.h>

// Forward declaration
class WebServer;

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    ~MainFrame() override;

    // This is a main window, it should not be copied or moved.
    MainFrame(const MainFrame&) = delete;
    MainFrame& operator=(const MainFrame&) = delete;
    MainFrame(MainFrame&&) = delete;
    MainFrame& operator=(MainFrame&&) = delete;

private:
    auto CreateWidgetsAndLayout() -> void;
    auto Log(const std::string& message) -> void; // Helper to add a log message to the text control
    auto OnAbout(wxCommandEvent& event) -> void;
    auto OnBrowse(wxCommandEvent& event) -> void; // Browse for document root
    auto OnButton(std::string button_name) -> void;
    auto OnButtonA(wxCommandEvent& event) -> void; // New button A event handler
    auto OnButtonB(wxCommandEvent& event) -> void; // New button B event handler
    auto OnClose(wxCloseEvent& event) -> void;
    auto OnExit(wxCommandEvent& event) -> void;
    auto OnLogMessage(wxCommandEvent& event) -> void;
    auto OnStartServer(wxCommandEvent& event) -> void;
    auto OnStopServer(wxCommandEvent& event) -> void;
    auto StopServer() -> void;
    auto UpdateUIForServerState(bool isRunning) -> void; // Helper to manage UI control states

    // Helper to create widgets and return the raw pointer for wxWidgets
    template <typename T, typename... Args>
    auto CreateWidget(Args&&... args) -> T*
    {
        T* widget = new T(std::forward<Args>(args)...);
        return widget;
    }

    // GUI Controls
    wxTextCtrl* m_portText {};
    wxTextCtrl* m_docRootText {};
    wxButton* m_startButton {};
    wxButton* m_stopButton {};
    wxButton* m_quitButton {};
    wxButton* m_buttonA {};
    wxButton* m_browseButton {};
    wxButton* m_buttonB {};
    wxTextCtrl* m_logText {};
    wxStaticText* m_statusLabel {};

    // Server instance
    std::unique_ptr<WebServer> m_server;

    wxDECLARE_EVENT_TABLE();
};

// Custom event for logging from background thread
wxDECLARE_EVENT(wxEVT_LOG_MESSAGE, wxCommandEvent);

#endif // MAIN_FRAME_HPP
