#ifndef OQTOPUS_JSON_HPP_
#define OQTOPUS_JSON_HPP_

#include "rexapis_common.hpp"

namespace rexapis {

class OQTOPUSJson {
public:
    static const std::string kJobIDKey;
    static const std::string kStatusKey;
    static const std::string kMessageKey;
    static const std::string kPresignedURLKey;
    static const std::string kURLKey;
    static const std::string kFieldsKey;
    static const std::string kKeyKey;
    static const std::string kAWSAccessKeyIdKey;
    static const std::string kAmzSecurityTokenKey;
    static const std::string kPolicyKey;
    static const std::string kSignatureKey;
};

class OQTOPUSSubmitJobResponseParser : public SubmitJobResponseParser {
public:
    OQTOPUSSubmitJobResponseParser() : SubmitJobResponseParser() {}
    ~OQTOPUSSubmitJobResponseParser() {}

    std::variant<SubmitJobValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class OQTOPUSGetJobStatusResponseParser : public GetJobStatusResponseParser {
public:
    OQTOPUSGetJobStatusResponseParser(const std::string& qcJobId) : GetJobStatusResponseParser(qcJobId) {}
    ~OQTOPUSGetJobStatusResponseParser() {}

    std::variant<GetJobStatusValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class OQTOPUSGetJobResultResponseParser : public GetJobResultResponseParser {
public:
    OQTOPUSGetJobResultResponseParser(const std::string& qcJobId) : GetJobResultResponseParser(qcJobId) {}
    ~OQTOPUSGetJobResultResponseParser() {}

    std::variant<GetJobResultValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class OQTOPUSCancelJobResponseParser : public CancelJobResponseParser {
public:
    OQTOPUSCancelJobResponseParser(const std::string& qcJobId) : CancelJobResponseParser(qcJobId) {}
    ~OQTOPUSCancelJobResponseParser() {}

    std::variant<CancelJobValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class OQTOPUSDeleteJobResponseParser : public DeleteJobResponseParser {
public:
    OQTOPUSDeleteJobResponseParser(const std::string& qcJobId) : DeleteJobResponseParser(qcJobId) {}
    ~OQTOPUSDeleteJobResponseParser() {}

    std::variant<DeleteJobValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class OQTOPUSRegisterJobValue {
public:
    OQTOPUSRegisterJobValue(const std::string& qcJobId, const std::string& url,
                            const std::string& key, const std::string& awsAccessKeyId,
                            const std::string& amzSecurityToken, const std::string& policy,
                            const std::string& signature) : qcJobId_(qcJobId), url_(url),
                                                            key_(key), awsAccessKeyId_(awsAccessKeyId),
                                                            amzSecurityToken_(amzSecurityToken),
                                                            policy_(policy), signature_(signature) {}
    ~OQTOPUSRegisterJobValue() {}

    std::string GetQCJobId() const { return qcJobId_; }
    std::string GetUrl() const { return url_; }
    std::string GetKey() const { return key_; }
    std::string GetAwsAccessKeyId() const { return awsAccessKeyId_; }
    std::string GetAmzSecurityToken() const { return amzSecurityToken_; }
    std::string GetPolicy() const { return policy_; }
    std::string GetSignature() const { return signature_; }

private:
    std::string qcJobId_;
    std::string url_;
    std::string key_;
    std::string awsAccessKeyId_;
    std::string amzSecurityToken_;
    std::string policy_;
    std::string signature_;
};

class OQTOPUSRegisterJobResponseParser {
public:
    OQTOPUSRegisterJobResponseParser() {}
    ~OQTOPUSRegisterJobResponseParser() {}

    std::variant<OQTOPUSRegisterJobValue, JsonParseError> Parse(const std::string& jsonStr);
};

}  // namespace rexapis

#endif  // OQTOPUS_JSON_HPP_

