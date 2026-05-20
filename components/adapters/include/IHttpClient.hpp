/**
 * @file IHttpClient.hpp
 * @brief Interface definition for HTTP client operations.
 *
 * This file defines the abstract interface for an HTTP client, supporting
 * both streaming data acquisition and full resource downloads.
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer implementation and interface classes.
 */
namespace adapters {

/**
 * @class IHttpClient
 * @brief Abstract interface for an HTTP client.
 *
 * This interface provides a standardized way to interact with web resources.
 * It supports synchronous downloads and buffered streaming, which is essential
 * for processing large files or media streams without loading them entirely into memory.
 */
class IHttpClient {
   public:
    /** @brief Default timeout for streaming operations in milliseconds (10 seconds). */
    static constexpr uint32_t DefaultStreamTimeoutMs = 10000U;

    /** @brief Default timeout for full resource downloads in milliseconds (10 seconds). */
    static constexpr uint32_t DefaultDownloadTimeoutMs = 10000U;

    /** @brief Default internal buffer size for HTTP transfers in bytes (2KB). */
    static constexpr size_t DefaultBufferSize = 2048U;

    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IHttpClient() = default;

    /**
     * @brief Opens a connection to the specified URL for streaming.
     * @param url The target URL to connect to.
     * @param timeoutMs The connection and read timeout in milliseconds. Defaults to
     * DefaultStreamTimeoutMs.
     * @return true if the stream was successfully opened and the server returned a success code,
     * false otherwise.
     */
    virtual bool openStream(const std::string& url,
                            const uint32_t& timeoutMs = DefaultStreamTimeoutMs) = 0;

    /**
     * @brief Reads a chunk of data from an already opened stream.
     * @param buffer Pointer to the memory buffer where data will be stored.
     * @param size The maximum number of bytes to read into the buffer.
     * @return The actual number of bytes read, or a negative value on error.
     */
    virtual int readStream(uint8_t* buffer, const size_t& size) = 0;

    /**
     * @brief Closes the currently active stream and connection.
     */
    virtual void closeStream() = 0;

    /**
     * @brief Downloads a full resource into a string.
     * @param url The target URL to download.
     * @param result Reference to a string where the response body will be stored.
     * @param timeoutMs The request timeout in milliseconds. Defaults to DefaultDownloadTimeoutMs.
     * @return true if the download completed successfully, false otherwise.
     */
    virtual bool download(const std::string& url, std::string& result,
                          const uint32_t& timeoutMs = DefaultDownloadTimeoutMs) = 0;

    /**
     * @brief Checks if a stream is currently open and active.
     * @return true if a stream connection is established, false otherwise.
     */
    virtual bool isStreamOpen() const = 0;

    /**
     * @brief Retrieves the URL associated with the current or last operation.
     * @return A reference to the URL string.
     */
    virtual const std::string& getUrl() const = 0;

    /**
     * @brief Gets the HTTP status code from the last server response.
     * @return The integer status code (e.g., 200 for OK, 404 for Not Found).
     */
    virtual int getStatusCode() const = 0;
};

}  // namespace adapters
