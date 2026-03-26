#pragma once

#include <esp_http_server.h>

#include <string>

#include "IProvisioningPortal.hpp"

namespace adapters {
class IWifiClient;

class ProvisioningPortal final : public IProvisioningPortal {
   public:
    explicit ProvisioningPortal(IWifiClient& wifiClient);
    ~ProvisioningPortal() override;

    ProvisioningPortal(const ProvisioningPortal&) = delete;
    ProvisioningPortal& operator=(const ProvisioningPortal&) = delete;

    bool start(const common::ProvisioningPortalConfig& cfg,
               common::CredentialsCallback onSubmit) override;
    void stop() override;
    bool isRunning() const override;
    std::string getApIp() const override;

   private:
    static esp_err_t handleRootGet(httpd_req_t* req);
    static esp_err_t handleWifiPost(httpd_req_t* req);

    esp_err_t onRootGet(httpd_req_t* req);
    esp_err_t onWifiPost(httpd_req_t* req);

    bool startWebServer();
    void stopWebServer();

    // HTML form body comes as "application/x-www-form-urlencoded" (key=value&...),
    // so values may include '+' or '%xx' escapes and must be decoded before use.
    static std::string urlDecode(const std::string& src);
    static bool parseCredentialsForm(const std::string& body, common::WifiCredentials& out);

    common::CredentialsCallback mOnSubmit;
    httpd_handle_t mHttpServer;
    IWifiClient& mWifiClient;
    bool mRunning;
};

}  // namespace adapters
