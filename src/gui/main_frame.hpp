#ifndef MAIN_FRAME_HPP
#define MAIN_FRAME_HPP

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
    auto OnLogMessage(wxCommandEvent& event) -> void;
    auto OnExit(wxCommandEvent& event) -> void;
    auto OnAbout(wxCommandEvent& event) -> void;
    auto OnClose(wxCloseEvent& event) -> void;

    // Server Control Helper
    auto StopServer() -> void;

    // Helper to add a log message to the text control
    auto Log(const std::string& message) -> void;

    // GUI Controls
    wxTextCtrl* m_portText;
    wxTextCtrl* m_docRootText;
    wxButton* m_startButton;
    wxButton* m_stopButton;
    wxButton* m_quitButton;
    wxTextCtrl* m_logText;
    wxStaticText* m_statusLabel;

    // Server instance
    std::unique_ptr<WebServer> m_server;

    wxDECLARE_EVENT_TABLE();
};

// Custom event for logging from background thread
wxDECLARE_EVENT(wxEVT_LOG_MESSAGE, wxCommandEvent);

#endif // MAIN_FRAME_HPP
