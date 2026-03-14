// ============================================================
// session_manager.hpp — Persistent auth session interface.
// Part of: angry::data
//
// Declares local session persistence API:
//   * Loads/saves auth token and username JSON state
//   * Clears persisted session on logout
//   * Exposes login-status and current credentials
//   * Isolates session file path configuration
// ============================================================

#pragma once

#include <string>

namespace angry
{

// Handles local auth session lifecycle and keeps in-memory
// token/username synchronized with on-disk session file.
class SessionManager
{
public:
    explicit SessionManager( std::string filepath = "session.json" );

    void loadSession();
    void saveSession() const;
    void clearSession();

    bool isLoggedIn() const;
    const std::string& token() const;
    const std::string& username() const;

    void setSession( std::string token, std::string username );

private:
    std::string filepath_;
    std::string token_;
    std::string username_;
};

}  // namespace angry
