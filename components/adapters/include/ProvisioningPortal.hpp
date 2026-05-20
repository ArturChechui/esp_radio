/**
 * @file ProvisioningPortal.hpp
 * @brief Implementation of the IProvisioningPortal interface for Wi-Fi configuration.
 *
 * This file contains the ProvisioningPortal class, which manages a local HTTP server
 * and a DNS captive portal to facilitate Wi-Fi credential entry.
 */

#pragma once

#include <esp_http_server.h>

#include <string>

#include "IProvisioningPortal.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer implementation classes.
 */
namespace adapters {

class IWifiClient;

/**
 * @class ProvisioningPortal
 * @brief Concrete implementation of a Wi-Fi provisioning portal using ESP-IDF's HTTP server.
 *
 * This class hosts a simple web application that allows users to connect to the device's
 * Access Point and submit their local Wi-Fi SSID and password via a web form.
 * It handles URL decoding of form data and dispatches credentials via a callback.
 */
class ProvisioningPortal final : public IProvisioningPortal {
   public:
    /**
     * @brief Constructs a new ProvisioningPortal object.
     * @param wifiClient Reference to the Wi-Fi client used to manage the Access Point state.
     */
    explicit ProvisioningPortal(IWifiClient& wifiClient);

    /**
     * @brief Destroys the ProvisioningPortal object and ensures the web server is stopped.
     */
    ~ProvisioningPortal() override;

    /** @brief Deleted copy constructor to prevent unintended copying. */
    ProvisioningPortal(const ProvisioningPortal&) = delete;
    /** @brief Deleted assignment operator to prevent unintended copying. */
    ProvisioningPortal& operator=(const ProvisioningPortal&) = delete;

    /**
     * @brief Starts the Access Point and the internal web server.
     * @param cfg Configuration for the Access Point (SSID, password, etc.).
     * @param onSubmit Callback triggered when valid credentials are submitted via the portal.
     * @return true if both the AP and web server started successfully, false otherwise.
     */
    bool start(const common::ProvisioningPortalConfig& cfg,
               common::CredentialsCallback onSubmit) override;

    /**
     * @brief Stops the web server and the Access Point.
     */
    void stop() override;

    /**
     * @brief Checks if the provisioning portal is currently active.
     * @return true if the web server and AP are running, false otherwise.
     */
    bool isRunning() const override;

    /**
     * @brief Retrieves the IP address of the device's Access Point.
     * @return A string representing the gateway IP (usually "192.168.4.1").
     */
    std::string getApIp() const override;

   private:
    /**
     * @brief Static URI handler for GET requests to the root path ("/").
     * @param req The HTTP request handle.
     * @return ESP_OK on success.
     */
    static esp_err_t handleRootGet(httpd_req_t* req);

    /**
     * @brief Static URI handler for POST requests containing Wi-Fi credentials ("/wifi").
     * @param req The HTTP request handle.
     * @return ESP_OK on success.
     */
    static esp_err_t handleWifiPost(httpd_req_t* req);

    /**
     * @brief Instance-level handler for rendering the provisioning HTML page.
     * @param req The HTTP request handle.
     * @return ESP_OK on success.
     */
    esp_err_t onRootGet(httpd_req_t* req);

    /**
     * @brief Instance-level handler for processing submitted Wi-Fi credentials.
     * @param req The HTTP request handle.
     * @return ESP_OK on success.
     */
    esp_err_t onWifiPost(httpd_req_t* req);

    /**
     * @brief Configures and starts the ESP-IDF HTTP server instance.
     * @return true if the server started successfully.
     */
    bool startWebServer();

    /**
     * @brief Shuts down the ESP-IDF HTTP server instance.
     */
    void stopWebServer();

    /**
     * @brief Decodes a URL-encoded string (converts '+' to ' ' and '%xx' to characters).
     * @param src The encoded source string.
     * @return The decoded string.
     */
    static std::string urlDecode(const std::string& src);

    /**
     * @brief Parses the raw HTTP POST body into a WifiCredentials structure.
     * @param body The raw "application/x-www-form-urlencoded" body.
     * @param out Reference to the structure where parsed SSID and password will be stored.
     * @return true if both SSID and password were found and parsed, false otherwise.
     */
    static bool parseCredentialsForm(const std::string& body, common::WifiCredentials& out);

    /** @brief The application callback invoked when credentials are received. */
    common::CredentialsCallback mOnSubmit;

    /** @brief Handle for the underlying ESP-IDF HTTP server. */
    httpd_handle_t mHttpServer;

    /** @brief Reference to the Wi-Fi client interface. */
    IWifiClient& mWifiClient;

    /** @brief Internal flag tracking if the portal is currently active. */
    bool mRunning;
};

}  // namespace adapters
