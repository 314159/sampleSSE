#include "main_frame.hpp"
#include "../server/web_server.hpp" // Include the server
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/valnum.h> // For numeric validator

// Define the custom event type
wxDEFINE_EVENT(wxEVT_LOG_MESSAGE, wxCommandEvent);

// Event table
// clang-format off
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(wxID_EXIT, MainFrame::OnExit)
EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
EVT_BUTTON(1001, MainFrame::OnStartServer)
EVT_BUTTON(1002, MainFrame::OnStopServer)
EVT_BUTTON(wxID_EXIT, MainFrame::OnExit) // Connect the Quit button
EVT_COMMAND(wxID_ANY, wxEVT_LOG_MESSAGE, MainFrame::OnLogMessage)
EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE();
// clang-format on

MainFrame::MainFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 450))
{
    // --- Menu Bar ---
    auto* menuFile = new wxMenu;
    menuFile->Append(wxID_ABOUT);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    auto* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);

    // --- Main Panel and Sizers ---
    auto* panel = new wxPanel(this, wxID_ANY);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* configSizer = new wxFlexGridSizer(2, 2, 5, 5);

    // --- Configuration Controls ---
    // Load configuration
    auto config = std::make_unique<wxConfig>("WebServerGUI");
    wxString portStr = config->Read("/config/port", "8080");
    wxString docRootStr = config->Read("/config/doc_root", ".");

    auto portValidator = wxIntegerValidator<unsigned short> {};
    portValidator.SetRange(1, 65535);

    m_portText = new wxTextCtrl { panel, wxID_ANY, portStr, wxDefaultPosition, wxDefaultSize, 0, portValidator };
    m_docRootText = new wxTextCtrl { panel, wxID_ANY, docRootStr };

    configSizer->Add(new wxStaticText(panel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    configSizer->Add(m_portText, 1, wxEXPAND);
    configSizer->Add(new wxStaticText(panel, wxID_ANY, "Document Root:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    configSizer->Add(m_docRootText, 1, wxEXPAND);
    configSizer->AddGrowableCol(1, 1);

    // --- Control Buttons ---
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_startButton = new wxButton { panel, 1001, "Start Server" };
    m_stopButton = new wxButton { panel, 1002, "Stop Server" };
    m_quitButton = new wxButton { panel, wxID_EXIT, "Quit" }; // Use standard ID
    m_stopButton->Enable(false);
    buttonSizer->Add(m_startButton, 0, wxALL, 5);
    buttonSizer->Add(m_stopButton, 0, wxALL, 5);
    buttonSizer->Add(m_quitButton, 0, wxALL, 5);

    // --- Status and Logging ---
    m_statusLabel = new wxStaticText(panel, wxID_ANY, "Status: Stopped");
    m_logText = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);

    // --- Layout ---
    mainSizer->Add(configSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);
    mainSizer->Add(m_statusLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    mainSizer->Add(new wxStaticText(panel, wxID_ANY, "Log:"), 0, wxLEFT | wxRIGHT, 10);
    mainSizer->Add(m_logText, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    panel->SetSizerAndFit(mainSizer);
    CreateStatusBar();
    SetStatusText("Ready");
}

MainFrame::~MainFrame() = default;
// The unique_ptr for m_server is correctly handled here,
// as its destructor is defined after web_server.hpp is included.

auto MainFrame::OnStartServer(wxCommandEvent& event) -> void
{
    if (m_server) {
        Log("Server is already running.");
        return;
    }

    unsigned long port;
    m_portText->GetValue().ToULong(&port);
    auto doc_root = m_docRootText->GetValue().ToStdString();
    auto address = std::string { "0.0.0.0" };

    try {
        m_server = std::make_unique<WebServer>(address, static_cast<unsigned short>(port), doc_root, 4);

        // This is the key for thread-safe logging from server to GUI
        m_server->set_logger([this](const std::string& msg) { //
            wxCommandEvent* logEvent = new wxCommandEvent(wxEVT_LOG_MESSAGE, GetId());
            logEvent->SetString(wxString(msg));
            wxQueueEvent(this, logEvent);
        });

        m_server->run(); // This starts the server threads and returns immediately

        Log("Server starting on " + address + ":" + std::to_string(port));
        m_statusLabel->SetLabel("Status: Running");
        m_startButton->Enable(false);
        m_stopButton->Enable(true);
        m_portText->Enable(false);
        m_docRootText->Enable(false);
    } catch (const std::exception& e) {
        Log("Error starting server: " + std::string(e.what()));
        m_server.reset(); // Clean up failed server instance
    }
}

auto MainFrame::OnStopServer(wxCommandEvent& event) -> void
{
    StopServer();
}

auto MainFrame::StopServer() -> void
{
    if (!m_server) {
        Log("Server is not running.");
        return;
    }

    Log("Stopping server...");
    m_server->stop();
    m_server.reset();

    Log("Server stopped.");
    m_statusLabel->SetLabel("Status: Stopped");
    m_startButton->Enable(true);
    m_stopButton->Enable(false);
    m_portText->Enable(true);
    m_docRootText->Enable(true);
}

auto MainFrame::OnLogMessage(wxCommandEvent& event) -> void
{
    Log(event.GetString().ToStdString());
}

auto MainFrame::OnClose(wxCloseEvent& event) -> void
{
    // Save configuration
    auto config = std::make_unique<wxConfig>("WebServerGUI");
    config->Write("/config/port", m_portText->GetValue());
    config->Write("/config/doc_root", m_docRootText->GetValue());

    if (m_server) {
        StopServer(); // Gracefully stop the server
    }
    Destroy();
}

auto MainFrame::OnExit(wxCommandEvent& event) -> void
{
    Close(true);
}

auto MainFrame::OnAbout(wxCommandEvent& event) -> void
{
    wxMessageBox("This is a C++20 Web Server using Boost.Beast and wxWidgets.",
        "About Web Server", wxOK | wxICON_INFORMATION);
}

auto MainFrame::Log(const std::string& message) -> void
{
    auto now = wxDateTime::Now();
    *m_logText << now.FormatISOTime() << ": " << message << "\n";
}
