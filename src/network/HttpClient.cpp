// Include the HttpClient header which contains the class declaration
#include "../../include/network/HttpClient.h"
// Include libcurl library for making HTTP requests
#include <curl/curl.h>
// Include for string stream operations (currently not used)
#include <sstream>

// Network namespace encapsulates all networking-related functionality
namespace Network {

// Static callback function used by libcurl to handle incoming response data
// This function is called by curl whenever it receives data from the server
// Parameters:
//   - contents: pointer to the received data buffer
//   - size: size of each data element
//   - nmemb: number of elements
//   - userp: user-provided pointer (in our case, a string to store the response)
// Returns: the number of bytes processed (size * nmemb), or curl will consider it an error
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    // Calculate total size: size * nmemb gives us the actual data size
    // Cast userp to std::string* and append the received data to it
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    // Return the number of bytes processed - this tells curl we handled all the data
    return size * nmemb;
}

// Constructor: proxy and session cookie
HttpClient::HttpClient(const Utils::Proxy& proxy, const std::string& cookie)
    : proxy(proxy), cookie(cookie), curl(nullptr) { 
    // Initialize curl - creates a new curl easy handle for making requests
    curl = curl_easy_init();
    // Configure curl with default settings (proxy, SSL options, timeouts, etc.)
    setupCurl();
}

// Destructor: Cleans up resources when HttpClient object is destroyed
HttpClient::~HttpClient() {
    // Check if curl handle was successfully initialized
    if (curl) {
        // Clean up and free the curl handle to prevent memory leaks
        curl_easy_cleanup(curl);
    }
}

// Private helper method to configure curl with default settings
// This is called once during construction to set up persistent options
void HttpClient::setupCurl() {
    // Safety check: ensure curl was initialized successfully
    if (!curl) return;

    // Cast void* to CURL* for type safety when calling curl functions
    CURL* curlHandle = static_cast<CURL*>(curl);

    // === PROXY CONFIGURATION ===
    // Get the proxy URL from the proxy configuration object
    std::string proxyUrl = proxy.getProxyUrl();
    // Set the proxy server URL (e.g., "http://127.0.0.1:8888")
    curl_easy_setopt(curlHandle, CURLOPT_PROXY, proxyUrl.c_str());
    // Set proxy type to HTTP proxy (as opposed to SOCKS4/5, etc.)
    curl_easy_setopt(curlHandle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);

    // === SSL/TLS OPTIONS ===
    // Disable SSL certificate verification - allows connections to sites with self-signed certs
    // WARNING: This makes connections less secure and vulnerable to MITM attacks
    curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
    // Disable hostname verification in SSL certificate
    // WARNING: This also reduces security
    curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);

    // === REDIRECT HANDLING ===
    // Enable automatic following of HTTP redirects (301, 302, etc.)
    curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L);

    // === TIMEOUT CONFIGURATION ===
    // Set maximum time in seconds for the entire request operation
    // Request will fail if it takes longer than 30 seconds
    curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, 30L);
}

// Returns a map of default HTTP headers that mimic a real browser
// These headers help the client appear as a legitimate Chrome browser to avoid detection
std::map<std::string, std::string> HttpClient::getDefaultHeaders() const {
    return {
        // Tell server we prefer JSON responses
        {"accept", "application/json"},
        // User-Agent: Identifies as Chrome 91 on Windows 10 - makes requests look like a real browser
        {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36"},
        // Client Hints: Part of Chrome's new fingerprinting headers
        {"sec-ch-ua", "\" Not;A Brand\";v=\"99\", \"Google Chrome\";v=\"91\", \"Chromium\";v=\"91\""},
        // Indicates this is not a mobile browser
        {"sec-ch-ua-mobile", "?0"},
        // Fetch metadata: tells server the request is from the same site
        {"sec-fetch-site", "same-site"},
        // Indicates CORS (Cross-Origin Resource Sharing) mode
        {"sec-fetch-mode", "cors"},
        // Destination type for the request (empty means XHR/fetch)
        {"sec-fetch-dest", "empty"},
        // Language preferences: English (US) preferred, fallback to any English
        {"accept-language", "en-US,en;q=0.9"}
    };
}

// Performs an HTTP GET request to the specified URL
// Parameters:
//   - url: The target URL to request
//   - headers: Additional headers to include (will override defaults if same key exists)
// Returns: HttpResponse object containing status code and response body
HttpResponse HttpClient::get(const std::string& url, const std::map<std::string, std::string>& headers) {
    // Create response object to store the result
    HttpResponse response;
    // Safety check: return empty response if curl wasn't initialized
    if (!curl) return response;

    // Cast the void* curl handle to CURL* for type safety
    CURL* curlHandle = static_cast<CURL*>(curl);
    // String buffer to accumulate the response data (passed to WriteCallback)
    std::string readBuffer;

    // === CONFIGURE HTTP METHOD AND URL ===
    // Set the URL to make the request to
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    // Explicitly set HTTP method to GET
    curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1L);
    // Set the callback function that will handle incoming data
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    // Pass our readBuffer as the user pointer to WriteCallback
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &readBuffer);

    // === BUILD HEADER LIST ===
    // Create a linked list to store all HTTP headers (curl uses slist for headers)
    struct curl_slist* headerList = nullptr;
    // Get the default headers (browser-like headers)
    auto defaultHeaders = getDefaultHeaders();

    // Add all default headers to the list
    for (const auto& [key, value] : defaultHeaders) {
        // Format as "key: value"
        std::string header = key + ": " + value;
        // Append to curl's header linked list
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add custom headers (these can override defaults)
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add cookie header if cookie string is provided
    if (!cookie.empty()) {
        std::string cookieHeader = "cookie: " + cookie;
        headerList = curl_slist_append(headerList, cookieHeader.c_str());
    }

    // Apply the complete header list to the request
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerList);

    // === EXECUTE THE REQUEST ===
    // Perform the actual HTTP request (blocking call)
    CURLcode res = curl_easy_perform(curlHandle);

    // === PROCESS THE RESULT ===
    if (res == CURLE_OK) {
        // Request succeeded
        long httpCode = 0;
        // Extract the HTTP status code (200, 404, 500, etc.)
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = httpCode;
        // Copy the accumulated response data
        response.body = readBuffer;
    } else {
        // Request failed at network/curl level (timeout, connection error, etc.)
        response.statusCode = 0;  // 0 indicates curl-level failure
    }

    // Clean up the header list to prevent memory leak
    curl_slist_free_all(headerList);
    return response;
}

// Performs an HTTP POST request to send data to the server
// Parameters:
//   - url: The target URL to request
//   - headers: Additional headers to include (will override defaults)
//   - body: The request body/payload to send (typically JSON or form data)
// Returns: HttpResponse object containing status code and response body
HttpResponse HttpClient::post(const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body) {
    // Create response object to store the result
    HttpResponse response;
    // Safety check: return empty response if curl wasn't initialized
    if (!curl) return response;

    // Cast the void* curl handle to CURL* for type safety
    CURL* curlHandle = static_cast<CURL*>(curl);
    // String buffer to accumulate the response data
    std::string readBuffer;

    // === CONFIGURE HTTP METHOD, URL, AND BODY ===
    // Set the URL to make the request to
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    // Set HTTP method to POST
    curl_easy_setopt(curlHandle, CURLOPT_POST, 1L);
    // Set the POST data/body (curl will copy this data)
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, body.c_str());
    // Set the callback function to handle response data
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    // Pass our readBuffer to the callback
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &readBuffer);

    // === BUILD HEADER LIST ===
    // Create a linked list to store all HTTP headers
    struct curl_slist* headerList = nullptr;
    // Get the default headers
    auto defaultHeaders = getDefaultHeaders();

    // Add all default headers to the list
    for (const auto& [key, value] : defaultHeaders) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add custom headers (can override defaults, e.g., content-type)
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add cookie header for authentication/session
    if (!cookie.empty()) {
        std::string cookieHeader = "cookie: " + cookie;
        headerList = curl_slist_append(headerList, cookieHeader.c_str());
    }

    // Apply headers to the request
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerList);

    // === EXECUTE THE REQUEST ===
    // Perform the HTTP POST request (blocking call)
    CURLcode res = curl_easy_perform(curlHandle);

    // === PROCESS THE RESULT ===
    if (res == CURLE_OK) {
        // Request succeeded
        long httpCode = 0;
        // Get the HTTP status code from the response
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = httpCode;
        // Store the response body
        response.body = readBuffer;
    } else {
        // Request failed (network error, timeout, etc.)
        response.statusCode = 0;
    }

    // Clean up header list to prevent memory leak
    curl_slist_free_all(headerList);
    return response;
}

// Performs an HTTP PUT request to update/replace a resource on the server
// PUT is typically used for updating existing resources or creating at a specific URL
// Parameters:
//   - url: The target URL to request
//   - headers: Additional headers to include
//   - body: The request body/payload to send (resource data)
// Returns: HttpResponse object containing status code and response body
HttpResponse HttpClient::put(const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body) {
    // Create response object to store the result
    HttpResponse response;
    // Safety check: return empty response if curl wasn't initialized
    if (!curl) return response;

    // Cast the void* curl handle to CURL* for type safety
    CURL* curlHandle = static_cast<CURL*>(curl);
    // String buffer to accumulate the response data
    std::string readBuffer;

    // === CONFIGURE HTTP METHOD, URL, AND BODY ===
    // Set the URL to make the request to
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    // Set HTTP method to PUT using custom request
    // (CUSTOMREQUEST allows us to use any HTTP verb)
    curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "PUT");
    // Set the PUT data/body (uses POSTFIELDS for PUT as well)
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, body.c_str());
    // Set the callback function to handle response data
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    // Pass our readBuffer to the callback
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &readBuffer);

    // === BUILD HEADER LIST ===
    // Create a linked list to store all HTTP headers
    struct curl_slist* headerList = nullptr;
    // Get the default headers
    auto defaultHeaders = getDefaultHeaders();

    // Add all default headers to the list
    for (const auto& [key, value] : defaultHeaders) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add custom headers
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add cookie header for authentication/session
    if (!cookie.empty()) {
        std::string cookieHeader = "cookie: " + cookie;
        headerList = curl_slist_append(headerList, cookieHeader.c_str());
    }

    // Apply headers to the request
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerList);

    // === EXECUTE THE REQUEST ===
    // Perform the HTTP PUT request (blocking call)
    CURLcode res = curl_easy_perform(curlHandle);

    // === PROCESS THE RESULT ===
    if (res == CURLE_OK) {
        // Request succeeded
        long httpCode = 0;
        // Get the HTTP status code from the response
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = httpCode;
        // Store the response body
        response.body = readBuffer;
    } else {
        // Request failed (network error, timeout, etc.)
        response.statusCode = 0;
    }

    // Clean up header list to prevent memory leak
    curl_slist_free_all(headerList);
    return response;
}

// Performs an HTTP DELETE request to remove a resource from the server
// DELETE is used to request the removal of a resource at the specified URL
// Parameters:
//   - url: The target URL to request (resource to delete)
//   - headers: Additional headers to include
// Returns: HttpResponse object containing status code and response body
HttpResponse HttpClient::del(const std::string& url, const std::map<std::string, std::string>& headers) {
    // Create response object to store the result
    HttpResponse response;
    // Safety check: return empty response if curl wasn't initialized
    if (!curl) return response;

    // Cast the void* curl handle to CURL* for type safety
    CURL* curlHandle = static_cast<CURL*>(curl);
    // String buffer to accumulate the response data
    std::string readBuffer;

    // === CONFIGURE HTTP METHOD AND URL ===
    // Set the URL to make the request to
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    // Set HTTP method to DELETE using custom request
    curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
    // Set the callback function to handle response data
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    // Pass our readBuffer to the callback
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &readBuffer);

    // === BUILD HEADER LIST ===
    // Create a linked list to store all HTTP headers
    struct curl_slist* headerList = nullptr;
    // Get the default headers
    auto defaultHeaders = getDefaultHeaders();

    // Add all default headers to the list
    for (const auto& [key, value] : defaultHeaders) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add custom headers
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add cookie header for authentication/session
    if (!cookie.empty()) {
        std::string cookieHeader = "cookie: " + cookie;
        headerList = curl_slist_append(headerList, cookieHeader.c_str());
    }

    // Apply headers to the request
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerList);

    // === EXECUTE THE REQUEST ===
    // Perform the HTTP DELETE request (blocking call)
    CURLcode res = curl_easy_perform(curlHandle);

    // === PROCESS THE RESULT ===
    if (res == CURLE_OK) {
        // Request succeeded
        long httpCode = 0;
        // Get the HTTP status code from the response (typically 200, 204, or 404)
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = httpCode;
        // Store the response body (may be empty for DELETE)
        response.body = readBuffer;
    } else {
        // Request failed (network error, timeout, etc.)
        response.statusCode = 0;
    }

    // Clean up header list to prevent memory leak
    curl_slist_free_all(headerList);
    return response;
}

} // namespace Network
