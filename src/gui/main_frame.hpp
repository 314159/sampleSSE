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
    // Event Handlers
    auto OnStartServer(wxCommandEvent& event) -> void;
    auto OnStopServer(wxCommandEvent& event) -> void;
    auto OnButtonA(wxCommandEvent& event) -> void; // New button A event handler
    auto OnButtonB(wxCommandEvent& event) -> void; // New button B event handler
    auto OnBrowse(wxCommandEvent& event) -> void; // Browse for document root
    auto OnLogMessage(wxCommandEvent& event) -> void;
    auto OnExit(wxCommandEvent& event) -> void;
    auto OnAbout(wxCommandEvent& event) -> void;
    auto OnClose(wxCloseEvent& event) -> void;
    
    // Helper to send SSE event
    auto OnButton(std::string button_name) -> void;

    // Server Control Helper
    auto StopServer() -> void;

    // Helper to add a log message to the text control
    auto Log(const std::string& message) -> void;

    // Helper to manage UI control states
    auto UpdateUIForServerState(bool isRunning) -> void;

    // GUI Controls
    gsl::not_null<wxTextCtrl*> m_portText;
    gsl::not_null<wxTextCtrl*> m_docRootText;
    gsl::not_null<wxButton*> m_startButton;
    gsl::not_null<wxButton*> m_stopButton;
    gsl::not_null<wxButton*> m_quitButton;
    gsl::not_null<wxButton*> m_buttonA; // New button A
    gsl::not_null<wxButton*> m_browseButton;
    gsl::not_null<wxButton*> m_buttonB; // New button B
    gsl::not_null<wxTextCtrl*> m_logText;
    gsl::not_null<wxStaticText*> m_statusLabel;

    // Server instance
    std::unique_ptr<WebServer> m_server;

    wxDECLARE_EVENT_TABLE();
};

// Custom event for logging from background thread
wxDECLARE_EVENT(wxEVT_LOG_MESSAGE, wxCommandEvent);

#endif // MAIN_FRAME_HPP
