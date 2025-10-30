#include "main_frame.hpp"
#include "../server/web_server.hpp" // Include the server
#include <gsl/gsl>
#include <wx/config.h>
#include <wx/filename.h>
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
EVT_BUTTON(1003, MainFrame::OnButtonA) // Event for Button A
EVT_BUTTON(1004, MainFrame::OnButtonB) // Event for Button B
EVT_BUTTON(1005, MainFrame::OnBrowse)   // Event for Browse button
EVT_BUTTON(wxID_EXIT, MainFrame::OnExit) // Connect the Quit button
EVT_COMMAND(wxID_ANY, wxEVT_LOG_MESSAGE, MainFrame::OnLogMessage)
EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE();
// clang-format on

MainFrame::MainFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 450))
{
    CreateWidgetsAndLayout();
    (void)CreateStatusBar(); // The returned pointer is managed by the frame.
    SetStatusText("Ready");
}

auto MainFrame::CreateWidgetsAndLayout() -> void
{
    // --- Main Panel and Sizers ---
    auto panel = gsl::not_null<wxPanel*> { new wxPanel { this, wxID_ANY } };
    auto mainSizer = gsl::not_null<wxBoxSizer*> { new wxBoxSizer { wxVERTICAL } };
    auto configSizer = gsl::not_null<wxFlexGridSizer*> { new wxFlexGridSizer { 2, 2, 5, 5 } };

    // --- Create Widgets ---
    // Port Text Control
    auto portValidator = wxIntegerValidator<unsigned short> {};
    portValidator.SetRange(1, 65535);
    auto config = std::make_unique<wxConfig>("WebServerGUI");
    auto portStr = config->Read("/config/port", "8080");
    auto charWidth = panel->GetCharWidth();
    auto portSize = wxSize { charWidth * 8, -1 };
    m_portText = gsl::not_null<wxTextCtrl*> { CreateWidget<wxTextCtrl>(panel, wxID_ANY, portStr, wxDefaultPosition, portSize, 0, portValidator) };

    // Document Root Text Control
    // Get the directory containing the executable in a platform-independent way.
    auto defaultDocRootPath = wxFileName::DirName(wxStandardPaths::Get().GetExecutablePath());
    if (!defaultDocRootPath.AppendDir("www")) {
        throw std::runtime_error("Failed to append 'www' to default document root path.");
    }
    auto docRootPath = wxFileName(config->Read("/config/doc_root", defaultDocRootPath.GetFullPath()));
    if (!docRootPath.Normalize(
            wxPATH_NORM_ABSOLUTE | wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | wxPATH_NORM_CASE)) {
        throw std::runtime_error("Failed to normalize document root path.");
    }
    m_docRootText = CreateWidget<wxTextCtrl>(panel, wxID_ANY, docRootPath.GetFullPath());
    m_browseButton = CreateWidget<wxButton>(panel, 1005, "Browse...");

    // Control Buttons
    m_startButton = CreateWidget<wxButton>(panel, 1001, "Start Server");
    m_stopButton = CreateWidget<wxButton>(panel, 1002, "Stop Server");
    m_buttonA = CreateWidget<wxButton>(panel, 1003, "Button A");
    m_buttonB = CreateWidget<wxButton>(panel, 1004, "Button B");
    m_quitButton = CreateWidget<wxButton>(panel, wxID_EXIT, "Quit");

    // Status and Logging
    m_statusLabel = CreateWidget<wxStaticText>(panel, wxID_ANY, "Status: Stopped");
    m_logText = CreateWidget<wxTextCtrl>(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);

    // --- Configure Widgets ---
    m_stopButton->Enable(false);

    // --- Layout Widgets ---
    // Config Section
    configSizer->Add(new wxStaticText(panel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    configSizer->Add(m_portText, 0, wxALIGN_LEFT);
    auto docRootSizer = gsl::not_null<wxBoxSizer*> { new wxBoxSizer { wxHORIZONTAL } };
    docRootSizer->Add(m_docRootText, 1, wxEXPAND | wxRIGHT, 5);
    docRootSizer->Add(m_browseButton, 0, wxALIGN_CENTER_VERTICAL);
    configSizer->Add(new wxStaticText(panel, wxID_ANY, "Document Root:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    configSizer->Add(docRootSizer, 1, wxEXPAND);
    configSizer->AddGrowableCol(1, 1);

    // Button Section
    auto buttonSizer = gsl::not_null<wxBoxSizer*>{new wxBoxSizer{wxHORIZONTAL}};
    buttonSizer->Add(m_startButton, 0, wxALL, 5);
    buttonSizer->Add(m_stopButton, 0, wxALL, 5);
    buttonSizer->Add(m_buttonA, 0, wxALL, 5);
    buttonSizer->Add(m_buttonB, 0, wxALL, 5);
    buttonSizer->Add(m_quitButton, 0, wxALL, 5);

    // Main Layout
    mainSizer->Add(configSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);
    mainSizer->Add(m_statusLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    mainSizer->Add(new wxStaticText(panel, wxID_ANY, "Log:"), 0, wxLEFT | wxRIGHT, 10);
    mainSizer->Add(m_logText, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // --- Menu Bar ---
    auto menuFile = gsl::not_null<wxMenu*>{new wxMenu};
    menuFile->Append(wxID_ABOUT);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    auto menuBar = gsl::not_null<wxMenuBar*>(new wxMenuBar); // wxWidgets manages the memory of the menu bar.
    menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);

    // --- Finalize Layout ---
    panel->SetSizerAndFit(mainSizer);
    SetMinSize(panel->GetSize());
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

    auto port = 0ul;
    m_portText->GetValue().ToULong(&port);
    auto doc_root_str = m_docRootText->GetValue();
    auto doc_root_path = wxFileName { doc_root_str };
    if (!doc_root_path.IsAbsolute()) {
        // If the path is relative, make it absolute relative to the executable's directory.
        doc_root_path.MakeAbsolute(wxStandardPaths::Get().GetExecutablePath());
    }
    const auto doc_root = doc_root_path.GetFullPath().ToStdString();
    const auto address = std::string { "::" };

    Log("Starting server with document root: " + doc_root);

    try {
        m_server = std::make_unique<WebServer>(address, static_cast<unsigned short>(port), doc_root, 4);

        // This is the key for thread-safe logging from server to GUI
        m_server->set_logger([this](const std::string& msg) {
            // Use gsl::owner to explicitly transfer ownership of the event
            // to the wxWidgets event queue.
            auto logEvent = gsl::not_null<wxCommandEvent*>(new wxCommandEvent(wxEVT_LOG_MESSAGE, GetId()));
            logEvent->SetString(wxString(msg));
            wxQueueEvent(this, gsl::owner<wxCommandEvent*>(logEvent));
        });

        m_server->run(); // This starts the server threads and returns immediately

        Log("Server starting on " + address + ":" + std::to_string(port));
        UpdateUIForServerState(true);
    } catch (const std::exception& e) {
        Log("Error starting server: " + std::string(e.what()));
        m_server.reset(); // Clean up failed server instance
    }
}

auto MainFrame::OnStopServer(wxCommandEvent& event) -> void { StopServer(); }

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
    UpdateUIForServerState(false);
}

auto MainFrame::OnLogMessage(wxCommandEvent& event) -> void { Log(event.GetString().ToStdString()); }
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

auto MainFrame::OnExit(wxCommandEvent& event) -> void { Close(true); }
auto MainFrame::OnAbout(wxCommandEvent& event) -> void
{
    (void)wxMessageBox("This is a C++20 Web Server using Boost.Beast and wxWidgets.",
        "About Web Server", wxOK | wxICON_INFORMATION);
}

auto MainFrame::OnButton(std::string button_name) -> void
{
    if (m_server) {
        if (m_server->get_sse_service()) {
            Log("Button '" + button_name + "' pressed. Sending SSE event 'button_press' with data '" + button_name + "'.");
            m_server->get_sse_service()->send_event_to_all("button_press", button_name);
        } else {
            Log("SSE service not available. Button " + button_name + "press not sent.");
        }
    } else {
        Log("Server not running. Cannot send SSE event.");
    }
}

auto MainFrame::OnButtonA(wxCommandEvent& event) -> void { OnButton("A"); }
auto MainFrame::OnButtonB(wxCommandEvent& event) -> void { OnButton("B"); }
auto MainFrame::OnBrowse(wxCommandEvent& event) -> void
{
    auto dirDialog = wxDirDialog(this, "Choose a directory for the document root", m_docRootText->GetValue(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

    if (dirDialog.ShowModal() == wxID_OK) {
        m_docRootText->SetValue(dirDialog.GetPath());
        Log("Document root changed to: " + dirDialog.GetPath().ToStdString());
    }
}

auto MainFrame::Log(const std::string& message) -> void
{
    auto now = wxDateTime::Now();
    *m_logText << now.FormatISOCombined() << ": " << message << "\n";
}

auto MainFrame::UpdateUIForServerState(bool isRunning) -> void
{
    m_statusLabel->SetLabel(isRunning ? "Status: Running" : "Status: Stopped");
    m_startButton->Enable(!isRunning);
    m_stopButton->Enable(isRunning);
    m_portText->Enable(!isRunning);
    m_docRootText->Enable(!isRunning);
    m_browseButton->Enable(!isRunning);
    m_buttonA->Enable(isRunning);
    m_buttonB->Enable(isRunning);
}
