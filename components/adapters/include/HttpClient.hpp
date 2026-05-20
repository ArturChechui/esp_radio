/**
 * @file HttpClient.hpp
 * @brief Implementation of the IHttpClient interface for ESP32 HTTP communication.
 * * This file contains the HttpClient class, which provides high-level wrappers around
 * the ESP-IDF HTTP client for streaming data and downloading files.
 */

#pragma once

#include <esp_http_client.h>

#include <cstdint>
#include <string>

#include "IHttpClient.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class HttpClient
 * @brief Concrete implementation of an HTTP client.
 * * This class handles HTTP connections, allowing for both streaming reads (useful for
 * audio or large data) and full resource downloads into memory strings. It manages
 * the lifecycle of an ESP-IDF HTTP client handle.
 */
class HttpClient : public IHttpClient {
   public:
    /**
     * @brief Constructs a new HttpClient object.
     * @param bufferSize The internal buffer size for HTTP operations. Defaults to
     * DefaultBufferSize.
     */
    HttpClient(const size_t& bufferSize = DefaultBufferSize);

    /**
     * @brief Destroys the HttpClient object and ensures connections are closed.
     */
    ~HttpClient() override;

    /**
     * @brief Opens a connection to a URL for streaming data.
     * @param url The target URL to open.
     * @param timeoutMs Connection and read timeout in milliseconds.
     * @return true if the stream was successfully opened, false otherwise.
     */
    bool openStream(const std::string& url, const uint32_t& timeoutMs) override;

    /**
     * @brief Reads data from an already opened stream.
     * @param buffer Pointer to the memory where data should be stored.
     * @param size The maximum number of bytes to read.
     * @return The number of bytes actually read, or a negative value on error.
     */
    int readStream(uint8_t* buffer, const size_t& size) override;

    /**
     * @brief Closes the currently active stream and connection.
     */
    void closeStream() override;

    /**
     * @brief Performs a full GET request and downloads the response body into a string.
     * @param url The target URL.
     * @param result Reference to a string where the downloaded content will be stored.
     * @param timeoutMs Request timeout in milliseconds.
     * @return true if the download was successful, false otherwise.
     */
    bool download(const std::string& url, std::string& result, const uint32_t& timeoutMs) override;

    /**
     * @brief Checks if a stream is currently open.
     * @return true if the client is connected and streaming, false otherwise.
     */
    bool isStreamOpen() const override;

    /**
     * @brief Retrieves the current or last used URL.
     * @return A reference to the URL string.
     */
    const std::string& getUrl() const override;

    /**
     * @brief Gets the HTTP status code from the last response.
     * @return The integer status code (e.g., 200, 404).
     */
    int getStatusCode() const override;

   private:
    /**
     * @brief Internal helper to open the ESP-IDF HTTP connection.
     * @return true if the connection was successful.
     */
    bool openConnection();

    /**
     * @brief Internal helper to close the ESP-IDF HTTP connection and clean up handles.
     */
    void closeConnection();

    /** @brief The ESP-IDF HTTP client handle. */
    esp_http_client_handle_t mHttpClient;

    /** @brief The target URL for the current operation. */
    std::string mUrl;

    /** @brief Connection and read timeout configuration. */
    uint32_t mTimeoutMs;

    /** @brief The size of the internal buffer. */
    size_t mBufferSize;
};

}  // namespace adapters
