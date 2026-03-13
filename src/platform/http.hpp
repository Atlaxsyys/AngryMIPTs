#pragma once

#include <string>
#include <utility>
#include <vector>

#ifndef __EMSCRIPTEN__
#include <cpr/cpr.h>
#endif

namespace platform::http
{

using Header = std::pair<std::string, std::string>;
using QueryParam = std::pair<std::string, std::string>;
using Headers = std::vector<Header>;
using QueryParams = std::vector<QueryParam>;

struct Response
{
    int status_code = 0;
    std::string body;
    bool network_error = false;
    std::string error_message;
};

inline bool is_http_ok( const Response& response )
{
    return response.status_code >= 200 && response.status_code < 300;
}

#ifndef __EMSCRIPTEN__

inline Response post( const std::string& url,
                      const std::string& body,
                      const Headers& headers,
                      int timeout_ms )
{
    cpr::Header cpr_headers;
    for ( const auto& [key, value] : headers )
    {
        cpr_headers[key] = value;
    }

    const cpr::Response raw = cpr::Post(
        cpr::Url {url},
        cpr_headers,
        cpr::Body {body},
        cpr::Timeout {timeout_ms} );

    Response response;
    response.status_code = static_cast<int> ( raw.status_code );
    response.body = raw.text;
    response.network_error = raw.error.code != cpr::ErrorCode::OK;
    if ( response.network_error )
    {
        response.error_message = raw.error.message;
    }
    return response;
}

inline Response get( const std::string& url,
                     const QueryParams& query_params,
                     const Headers& headers,
                     int timeout_ms )
{
    cpr::Parameters cpr_query;
    for ( const auto& [key, value] : query_params )
    {
        cpr_query.Add( cpr::Parameter {key, value} );
    }

    cpr::Header cpr_headers;
    for ( const auto& [key, value] : headers )
    {
        cpr_headers[key] = value;
    }

    const cpr::Response raw = cpr::Get(
        cpr::Url {url},
        cpr_query,
        cpr_headers,
        cpr::Timeout {timeout_ms} );

    Response response;
    response.status_code = static_cast<int> ( raw.status_code );
    response.body = raw.text;
    response.network_error = raw.error.code != cpr::ErrorCode::OK;
    if ( response.network_error )
    {
        response.error_message = raw.error.message;
    }
    return response;
}

#else

// Web stub for Phase 5.
// We intentionally return a deterministic network error while we migrate to
// emscripten_fetch callback-based flow.
inline Response post( const std::string& /*url*/,
                      const std::string& /*body*/,
                      const Headers& /*headers*/,
                      int /*timeout_ms*/ )
{
    Response response;
    response.network_error = true;
    response.error_message = "http unavailable in web build (phase5 stub)";
    return response;
}

inline Response get( const std::string& /*url*/,
                     const QueryParams& /*query_params*/,
                     const Headers& /*headers*/,
                     int /*timeout_ms*/ )
{
    Response response;
    response.network_error = true;
    response.error_message = "http unavailable in web build (phase5 stub)";
    return response;
}

#endif

}  // namespace platform::http

